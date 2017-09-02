/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * udfa_host.cu
 */

#include <cstdlib>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <libgen.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "packets.h"
#include "common.h"
#include "mem_controller.h"
#include "udfa_host.h"
#include "udfa_gpu.h"
				
using namespace std;

extern CommonConfigs cfg;

/*--------------------------------------------------------------------------------------------------*/
#ifdef TEXTURE_MEM_USE //Texture memory: DFA STATE TABLE
texture<state_t, cudaTextureType1D, cudaReadModeElementType> tex_dfa_state_tables;
__global__ void udfa_kernel_texture(symboln *input,
									unsigned int *pkt_size_vec, unsigned int pkt_size,
									unsigned int *match_count, match_type *match_array, unsigned int match_vec_size,
									unsigned int *accum_dfa_state_table_lengths, unsigned int n_subsets);
#endif
/*--------------------------------------------------------------------------------------------------*/
void GPUMemInfo() {
   size_t free_byte ;
   size_t total_byte ;
   cudaError_t cuda_status = cudaMemGetInfo( &free_byte, &total_byte ) ;
   if ( cudaSuccess != cuda_status ){   
      printf("Error: cudaMemGetInfo fails, %s \n", cudaGetErrorString(cuda_status) );   
      exit(1);   
   }
   double free_db  = (double)free_byte ;
   double total_db = (double)total_byte ;
   double used_db  = total_db - free_db ;

   printf("GPU memory usage: used = %lf MB, free = %lf MB, total = %f MB\n", used_db/1024.0/1024.0, free_db/1024.0/1024.0, total_db/1024.0/1024.0);
}
/*--------------------------------------------------------------------------------------------------*/
unsigned int udfa_run(std::vector<FiniteAutomaton *> fa, Packets &packets, unsigned int n_subsets, unsigned int packet_size, int *rulestartvec, double *t_alloc, double *t_kernel, double *t_collect, double *t_free, int *blocksize, int blksiz_tuning){

    struct timeval c0, c1, c2, c3, c4;
    long seconds, useconds;
    unsigned int *h_match_count, *d_match_count;
    match_type   *h_match_array, *d_match_array;
   
    ofstream fp_report;
    char filename[200], bufftmp[10];
   
    state_t *d_dfa_state_tables;
    symbol *d_input;
    unsigned int *d_pkt_size;
    size_t max_shmem=0;
    unsigned int   *accum_dfa_state_table_lengths;//Note: arrays contain accumulated values
    unsigned int *d_accum_dfa_state_table_lengths;
   
	//cout << "------------- Preparing to launch kernel ---------------" << endl;
	//cout << "Packets (Streams or Number of CUDA blocks in x-dimension): " << packets.get_payload_sizes().size() << endl;
	
	//cout << "Accumulated number of symbol per packet (stream): ";
	//for (int i = 0; i < packets.get_payload_sizes().size(); i++)
	//	cout << packets.get_payload_sizes()[i] << " ";
	//cout << endl;
    
	//cout << "Threads per block: " << cfg.get_threads_per_block() << endl;
		
	for (unsigned int i = 0; i < n_subsets; ++i) {
		cout << "Graph (DFA) " << i+1 << endl;
		cout << "   + State count: " << cfg.get_state_count(i) << endl;
		cout << endl;
	}
    
	gettimeofday(&c0, NULL);

	unsigned int tmp_avg_count = packets.get_payload_sizes()[packets.get_payload_sizes().size()-1]/packets.get_payload_sizes().size()*60/n_subsets;//just for now, size of each match array for each packet//????????
	
	cout << "Maximum matches allowed:  " << (tmp_avg_count*packets.get_payload_sizes().size()*n_subsets) << endl;
	
	h_match_array         = (match_type*)malloc ((tmp_avg_count*packets.get_payload_sizes().size()) * n_subsets * sizeof(match_type));//just for now
    h_match_count         = (unsigned int*)malloc ((              packets.get_payload_sizes().size()) * n_subsets * sizeof(unsigned int));//just for now  
    accum_dfa_state_table_lengths = (unsigned int*)malloc (n_subsets * sizeof(unsigned int));
			
	cudaMalloc( (void **) &d_match_array,  (tmp_avg_count*packets.get_payload_sizes().size()) * n_subsets * sizeof(match_type));//just for now
    cudaMalloc( (void **) &d_match_count,  (              packets.get_payload_sizes().size()) * n_subsets * sizeof(unsigned int));//just for now
	cudaMalloc( (void **) &d_accum_dfa_state_table_lengths, n_subsets * sizeof(unsigned int));
    	
	size_t tmp_dfa_state_table_total_size=0, tmp_curr_dfa_state_table_size=0, tmp_accum_prev_dfa_state_table_size=0;//in bytes
	for (unsigned int i = 0; i < n_subsets; i++) {//Find total size (in bytes) of each data structure
		tmp_dfa_state_table_total_size +=  fa[i]->get_dfa_state_table_size();
	}
	
	//Allocate device memory
	cudaMalloc((void **) &d_dfa_state_tables, tmp_dfa_state_table_total_size);
    cudaMalloc((void **) &d_input, packets.get_payloads().size() * sizeof(*d_input));
    cudaMalloc((void **) &d_pkt_size, packets.get_payload_sizes().size() * sizeof(*d_pkt_size));
	
	for (unsigned int i = 0; i < n_subsets; i++){//Copy to device memory
		cudaError_t retval3;
		tmp_curr_dfa_state_table_size =  fa[i]->get_dfa_state_table_size();
				
		if (i==0){
			retval3 = cudaMemcpy( d_dfa_state_tables, fa[i]->get_dfa_state_table(), tmp_curr_dfa_state_table_size, cudaMemcpyHostToDevice);
		}
		else{
			tmp_accum_prev_dfa_state_table_size +=  fa[i-1]->get_dfa_state_table_size();
			retval3 = cudaMemcpy( &d_dfa_state_tables[tmp_accum_prev_dfa_state_table_size/sizeof(state_t)], fa[i]->get_dfa_state_table(), tmp_curr_dfa_state_table_size, cudaMemcpyHostToDevice);
		}
		accum_dfa_state_table_lengths[i] = tmp_accum_prev_dfa_state_table_size/sizeof(state_t);
	
		if (retval3 != cudaSuccess) cout << "Error while copying dfa state table to device memory" << endl;
	}

    cudaError_t retval = cudaMemcpy(d_input, &(packets.get_payloads()[0]), packets.get_payloads().size() * sizeof(*d_input), cudaMemcpyHostToDevice);
    if (retval != cudaSuccess) cout << "Error while copying payload to device memory" << endl;
	
    retval = cudaMemcpy(d_pkt_size, &(packets.get_payload_sizes()[0]), packets.get_payload_sizes().size() * sizeof(*d_pkt_size), cudaMemcpyHostToDevice);
	if (retval != cudaSuccess) cout << "Error while copying packet sizes to device memory" << endl;
	
	cudaMemcpy( d_accum_dfa_state_table_lengths, accum_dfa_state_table_lengths,    n_subsets * sizeof(unsigned int), cudaMemcpyHostToDevice);
			
	GPUMemInfo();
	
	//Tuning blocksize
	int device;
	cudaDeviceProp props;
	cudaGetDevice(&device);
	cudaGetDeviceProperties(&props, device); printf("multiProcessorCount = %d\n", props.multiProcessorCount);
	
	int blockSize_, maxActiveBlocks;
	float occupancy;
	int n_packets = packets.get_payload_sizes().size();
	
	blockSize_ = 1;
	cudaOccupancyMaxActiveBlocksPerMultiprocessor( &maxActiveBlocks, udfa_kernel, blockSize_, max_shmem);
	printf("Required computational workload: subsets = %d, packets = %d, subsets*packets = %d\n", n_subsets, n_packets, n_packets * n_subsets);
	//while ( (  blockSize_*maxActiveBlocks*props.multiProcessorCount <= packets.get_payload_sizes().size()*n_subsets ) && (blockSize_ < n_subsets) && (blockSize_ < 1024) ){	//not very correct
	while ( (  maxActiveBlocks*props.multiProcessorCount <= n_packets * (n_subsets/blockSize_ + 1) ) && (blockSize_ < n_subsets) && (blockSize_ < 1024) ){
		occupancy = (maxActiveBlocks * blockSize_ / props.warpSize) / (float)(props.maxThreadsPerMultiProcessor / props.warpSize)*100;
		printf("Inter. theoretical GPU launch info: blockSize_ = %d, maxActiveBlocks = %d, maxActiveBlocks*multiProcessorCount = %d, grid.x=%d, grid.y=%d, occupancy: %f\n", blockSize_, maxActiveBlocks, maxActiveBlocks*props.multiProcessorCount, n_packets, n_subsets/blockSize_ + 1, occupancy);
		blockSize_++;
		cudaOccupancyMaxActiveBlocksPerMultiprocessor( &maxActiveBlocks, udfa_kernel, blockSize_, max_shmem);
	}
	*blocksize = blockSize_;
	cudaOccupancyMaxActiveBlocksPerMultiprocessor( &maxActiveBlocks, udfa_kernel, *blocksize, max_shmem);
	occupancy = (maxActiveBlocks * (*blocksize) / props.warpSize) / (float)(props.maxThreadsPerMultiProcessor / props.warpSize)*100;	
	printf("Final theoretical GPU launch info: blocksize = %d, maxActiveBlocks = %d, maxActiveBlocks*multiProcessorCount = %d, grid.x=%d, grid.y=%d, occupancy: %f\n", *blocksize, maxActiveBlocks, maxActiveBlocks*props.multiProcessorCount, n_packets, n_subsets/(*blocksize) + 1, occupancy);

#ifdef TEXTURE_MEM_USE
	// bind textures to d_dfa_state_tables
    cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<state_t>();
    cudaBindTexture(0, tex_dfa_state_tables, d_dfa_state_tables, channelDesc, tmp_dfa_state_table_total_size);
    printf("Texture memory usage: %lf MB\n", tmp_dfa_state_table_total_size/1024.0/1024.0);
#endif	
	gettimeofday(&c1, NULL);
	
	// Launch kernel (asynchronously)
	//printf("Size of symbol = %d, Size of unsigned char = %d\n",sizeof(symbol), sizeof(unsigned char));
	printf("U-DFA kernel\n");
	dim3 block(cfg.get_threads_per_block(),1);
	dim3 grid (packets.get_payload_sizes().size(),n_subsets/cfg.get_threads_per_block() + 1);
	
	cudaOccupancyMaxActiveBlocksPerMultiprocessor( &maxActiveBlocks, udfa_kernel, cfg.get_threads_per_block(), max_shmem);
	occupancy = (maxActiveBlocks * cfg.get_threads_per_block() / props.warpSize) / (float)(props.maxThreadsPerMultiProcessor / props.warpSize)*100;	
	printf("Manual GPU launch info: blocksize = %d, maxActiveBlocks = %d, maxActiveBlocks*multiProcessorCount = %d, grid.x=%d, grid.y=%d, occupancy: %f\n", cfg.get_threads_per_block(), maxActiveBlocks, maxActiveBlocks*props.multiProcessorCount, grid.x, grid.y, occupancy);
	
	if (blksiz_tuning == 1) {
		block.x = *blocksize;
		grid.x = packets.get_payload_sizes().size();
		grid.y = n_subsets/(*blocksize) + 1;
		printf("Blocksize tuning is being used!\n");
	}
	cout << "GPU launch info: block.x = " << block.x << ", grid.x = " << grid.x << ", grid.y = " << grid.y << endl;

#ifdef TEXTURE_MEM_USE
    printf("Store DFA STATE TABLE in texture memory!\n");
    udfa_kernel_texture<<<grid, block>>>(
                                        (symboln*)d_input,
                                        d_pkt_size, packet_size,
                                        d_match_count, d_match_array, tmp_avg_count,
                                        d_accum_dfa_state_table_lengths, n_subsets);
#else
	printf("Store DFA STATE TABLE in global memory!\n");
    udfa_kernel<<<grid, block>>>(d_dfa_state_tables,
                                (symboln*)d_input,
                                d_pkt_size, packet_size,
                                d_match_count, d_match_array, tmp_avg_count,
                                d_accum_dfa_state_table_lengths, n_subsets);							    
#endif
				
	cudaThreadSynchronize();
	
	gettimeofday(&c2, NULL);

#ifdef TEXTURE_MEM_USE
    // unbind textures from d_nfa_state_tables, d_ptr_state_tables
    cudaUnbindTexture(tex_dfa_state_tables);
#endif
	
	seconds  = c2.tv_sec  - c1.tv_sec;
	useconds = c2.tv_usec - c1.tv_usec;
    *t_kernel= ((double)seconds * 1000 + (double)useconds/1000.0);
	printf("host_functions.cu: t_kernel= %lf(ms)\n", *t_kernel);
	
	cudaMemcpy( h_match_count, d_match_count,                packets.get_payload_sizes().size()  * n_subsets * sizeof(unsigned int), cudaMemcpyDeviceToHost);
	cudaMemcpy( h_match_array, d_match_array, (tmp_avg_count*packets.get_payload_sizes().size()) * n_subsets * sizeof(match_type), cudaMemcpyDeviceToHost);
	
	// Collect results
	//Temporarily comment the following FOR loop
	unsigned int total_matches=0;
	for (unsigned int i = 0; i < n_subsets; i++) {
#ifdef TEXTURE_MEM_USE
        strcpy (filename,"../bin/data/Report_tex_");
#else
        strcpy (filename,"../bin/data/Report_global_");
#endif
		snprintf(bufftmp, sizeof(bufftmp),"%d",n_subsets);
		strcat (filename,bufftmp);
		strcat (filename,"_");
		snprintf(bufftmp, sizeof(bufftmp),"%d",i+1);
		strcat (filename,bufftmp);
		strcat (filename,".txt");
		fp_report.open (filename); //cout << "Report filename:" << filename << endl;
		fa[i]->mapping_states2rules(&h_match_count[packets.get_payload_sizes().size()*i], &h_match_array[tmp_avg_count*packets.get_payload_sizes().size()*i], 
		                            tmp_avg_count, packets.get_payload_sizes(), packets.get_padded_sizes(), fp_report, rulestartvec, i);
		fp_report.close();
		for (unsigned int j = 0; j < packets.get_payload_sizes().size(); j++)
			total_matches += h_match_count[j + packets.get_payload_sizes().size()*i];		
	}
	printf("Host - Total number of matches %d\n", total_matches);

    gettimeofday(&c3, NULL);
	
	// Free some memory
	cudaFree(d_match_count);
	cudaFree(d_match_array);
	cudaFree(d_accum_dfa_state_table_lengths);
	cudaFree(d_dfa_state_tables);
	cudaFree(d_input);
    cudaFree(d_pkt_size);

	free(h_match_count);
    free(h_match_array);
	free(accum_dfa_state_table_lengths);
		
	gettimeofday(&c4, NULL);
	
	seconds  = c1.tv_sec  - c0.tv_sec;
	useconds = c1.tv_usec - c0.tv_usec;
    *t_alloc = ((double)seconds * 1000 + (double)useconds/1000.0);
	
	seconds  = c2.tv_sec  - c1.tv_sec;
	useconds = c2.tv_usec - c1.tv_usec;
    *t_kernel= ((double)seconds * 1000 + (double)useconds/1000.0);
	
	seconds    = c3.tv_sec  - c2.tv_sec;
	useconds   = c3.tv_usec - c2.tv_usec;
    *t_collect = ((double)seconds * 1000 + (double)useconds/1000.0);

	seconds  = c4.tv_sec  - c3.tv_sec;
	useconds = c4.tv_usec - c3.tv_usec;
    *t_free  = ((double)seconds * 1000 + (double)useconds/1000.0);
	
	return 0;
}
/*--------------------------------------------------------------------------------------------------*/
#ifdef TEXTURE_MEM_USE
__global__ void udfa_kernel_texture(symboln *input,
									unsigned int *pkt_size_vec, unsigned int pkt_size,
									unsigned int *match_count, match_type *match_array, unsigned int match_vec_size,
									unsigned int *accum_dfa_state_table_lengths, unsigned int n_subsets){					
	
	unsigned int dfa_id = threadIdx.x + blockIdx.y * blockDim.x;
	match_type tmp_match;
	
	if (dfa_id >= n_subsets)
		return;
	
	unsigned int shr_match_count = 0;
	
	//cur_pkt_size is the input string length of the packet
	unsigned int cur_pkt_size = pkt_size_vec[blockIdx.x];

	//skip to the right input string
	input += (pkt_size * blockIdx.x/fetch_bytes); 

	unsigned int accum_dfa_state_table_length = accum_dfa_state_table_lengths[dfa_id];
	
	state_t current_state = 0;

	//Payload loop
	for(unsigned int p=0; p<cur_pkt_size; p+=fetch_bytes, input++){
		symboln Input_ = *input;//fetch 4 bytes from the input string
		for (unsigned int byt = 0; byt < fetch_bytes; byt++) {
			unsigned int Input = Input_ & 0xFF;//extract 1 byte
			Input_  = Input_ >> 8;//Input_ right-shifted by 8 bits
				
			//Query the state table on the input symbol for the next state
			current_state = tex1Dfetch(tex_dfa_state_tables, current_state * CSIZE + Input + accum_dfa_state_table_length);
			
			if (current_state < 0) {//Added for matching operation: check if the dst state is an accepting state
				current_state = -current_state;
				//match_offset[match_vec_size*blockIdx.x + shr_match_count + dfa_id*match_vec_size*gridDim.x] = p;
				//match_states[match_vec_size*blockIdx.x + shr_match_count + dfa_id*match_vec_size*gridDim.x] = current_state;
				tmp_match.off  = p + byt;
				tmp_match.stat = current_state;
				match_array[shr_match_count + match_vec_size*(blockIdx.x + dfa_id*gridDim.x)] = tmp_match;
				
				shr_match_count = shr_match_count + 1;			
			}
		}
	}
	match_count[blockIdx.x + dfa_id*gridDim.x] = shr_match_count;
}
#endif