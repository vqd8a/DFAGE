/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * udfa_main.cu
 */

#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "packets.h"
#include "udfa_host.h"

using namespace std;

size_t getFilesize(const char* filename);
void Usage(void);
bool ParseCommandLine(int argc, char *argv[]);

const char *base_name=NULL;

#ifdef DEBUG
const char *timing_filename = NULL;
const char *blksiz_filename = NULL;
#endif

int total_rules=0;
int blksiz_tuning = 0;
int automata_format = 0;

CommonConfigs cfg;

int main(int argc, char* argv[]){

    unsigned int retval;
    std::vector<FiniteAutomaton *> dfa_vec;
	    
	char char_temp;
    char filename[1500], bufftmp[10];

	struct timeval c1, c2, c3, c4, c5;
	long seconds, useconds;
	double t_alloc, t_kernel, t_collect, t_free, t_DFAload, t_in;
	
#ifdef DEBUG
	double t_exec;
	ofstream fp_timing;
	ofstream fp_blksiz;
#endif	
	
	int rulespergroup, *rulestartvec;
	int blockSize;
	
	// Load DFAs from files and stores in arrays of internal data structure
	gettimeofday(&c1, NULL);
	
	retval = ParseCommandLine(argc, argv);
    
	if(!retval)
		return 0;
	
    unsigned int total_bytes = getFilesize(cfg.get_input_file_name());

	cout<< "-----------------User input info--------------------" << endl;
	cout<< "Total number of rules: " << total_rules << endl;
	cout<< "Total input bytes: "   << total_bytes << endl;	
	unsigned int n_subsets   = cfg.get_groups();	
	unsigned int n_packets   = cfg.get_packets();
	unsigned int packet_size = ((total_bytes%n_packets)==0)?(total_bytes/n_packets):(total_bytes/n_packets+1);
	cout<< "Subgraph(s) (or DFA(s)) combined: "   << n_subsets << endl;
	cout<< "Packet(s): "   << n_packets << endl;
    cout<< "Packet size (bytes): " << packet_size << endl;
    if (automata_format ==0)
        cout << "Automata in binary format" << endl;
    else
        cout << "Automata in MNRL format" << endl;
    if (blksiz_tuning ==0)
        cout << "Blocksize tuning is not enabled" << endl;
    else
        cout << "Blocksize tuning is enabled" << endl;
	
	rulestartvec = (int*)malloc (n_subsets * sizeof(int));

    if ((total_rules%n_subsets)==0)
		rulespergroup = total_rules/n_subsets;
	else
		rulespergroup = total_rules/n_subsets + 1;
	//printf("rulespergroup=%d\n",rulespergroup);
	for (unsigned int i=0; i<n_subsets; i++) {
		rulestartvec[i]=i*rulespergroup;
		//printf("rulestartvec[%d]=%d\n",i,rulestartvec[i]);
	}

	cout << endl;
	cout<< "-----------------Loading DFA(s) from file(s)--------------------" << endl;
	cout << "Loading..." << endl;

	for (unsigned int i = 0; i < n_subsets; i++) {		
				
		strcpy (filename,base_name);
		
		strcat (filename,"_");
		snprintf(bufftmp, sizeof(bufftmp),"%d",n_subsets);
		strcat (filename,bufftmp);
		strcat (filename,"/");	
		snprintf(bufftmp, sizeof(bufftmp),"%d",i+1);
		strcat (filename,bufftmp); //cout<< "filename " << i + 1 << ":"<< filename << endl;
		
		FiniteAutomaton *dfa_tmp = NULL;//cout <<"TEST sizeof(*FiniteAutomaton) = " << sizeof(*dfa_tmp)<<endl;

		dfa_tmp = load_dfa_file(filename, i, automata_format);
		
		if(!dfa_tmp){
			printf("Error while loading DFA on the device\n");
			return 0;
		}
		dfa_vec.push_back(dfa_tmp);
	}

	cout << "\nDFA loading done!!!\n\n";
	
	for (unsigned int i = 0; i < n_subsets; i++) {
		if (i!=n_subsets-1) cout << "Sub-ruleset "<< i + 1 << ": Rules: " << rulestartvec[i+1] - rulestartvec[i] <<", States: "<< cfg.get_state_count(i) << endl;	
	    else cout << "Sub-ruleset "<< i + 1 << ": Rules: " << total_rules - rulestartvec[i] <<", States: "<< cfg.get_state_count(i) << endl;	
	}    
	gettimeofday(&c2, NULL);
		
	printf("-----------------Starting dfa execution--------------------\n");
    	
	// open input stream file    
	ifstream fp(cfg.get_input_file_name());
#ifdef DEBUG
	if (timing_filename != NULL)//and timing file
		fp_timing.open(timing_filename,ios::binary | ios::out);
	if (blksiz_filename != NULL)//and timing file
		fp_blksiz.open(blksiz_filename,ios::binary | ios::out);
#endif
		
	unsigned int processed_packets = 0;
{	
	gettimeofday(&c3, NULL);
	
	Packets packets;
    vector<unsigned char> payload;
    vector<unsigned int> payload_count;

	// Read input stream file		
	//cout << "Fixed-size packet processing" << endl;
	unsigned int cnt2=0;
	unsigned int cnt=0;
	if (fp){								
		while ( fp.get(char_temp) ){
			cnt2++;
			payload.push_back(char_temp);
			cnt++;					
			if (cnt==packet_size){				
				//version 2 -- note: padding to each packet if packet_size is not evenly divided by fetch_bytes (e.g. 4, 8)
				if ( (cnt%fetch_bytes) != 0 ) {
					for (unsigned int i = 0; i < (fetch_bytes-(cnt%fetch_bytes)); i++)
						payload.push_back(0);
					packets.set_padded_bytes(fetch_bytes-(cnt%fetch_bytes));
				}				
				packets.add_packet(payload);
				payload_count.push_back(payload.size());//cout << payload.size() << endl;
				processed_packets++;
				payload.clear();
				cnt=0;
			}
		}
		if ((cnt>0)&&(cnt<packet_size)){
			//version 2 -- note: padding to each packet if packet_size is not evenly divided by fetch_bytes (e.g. 4, 8)
			if ( (cnt%fetch_bytes) != 0 ) {
				for (unsigned int i = 0; i < (fetch_bytes-(cnt%fetch_bytes)); i++)
					payload.push_back(0);
				packets.set_padded_bytes(fetch_bytes-(cnt%fetch_bytes));
			}
			packets.add_packet(payload);
			payload_count.push_back(payload.size());//cout << payload.size() << endl;
			processed_packets++;
			payload.clear();
			cnt=0;
		}
	}
	else{
		cout<< "Cannot open input file" << endl;				
	}
	cout << "Number of processed packets: "<< processed_packets << " and total number of bytes: "<< cnt2 << endl;
	for (unsigned int i = 0; i < processed_packets; i++){
		cout << "Packet "<< i << ": " << payload_count[i] << " bytes (padding included)"<< endl;				
	}			
	// End of Fixed-size packet processing
	
	//ofstream myfile2 ("./data/packet_cnts.txt");			
	//for (unsigned int i = 0; i < processed_packets; i++){
	//	myfile2 << payload_count[i] << endl;
	//}
	//myfile2.close();
		
	gettimeofday(&c4, NULL);
		
	//cout << "UDFA!!!" << endl;
	retval = udfa_run(dfa_vec, packets, n_subsets, packet_size, rulestartvec, &t_alloc, &t_kernel, &t_collect, &t_free, &blockSize, blksiz_tuning);	
					
	gettimeofday(&c5, NULL);
}
	cout << "----------------- Kernel execution done -----------------" << endl;

	seconds  = c2.tv_sec  - c1.tv_sec;
	useconds = c2.tv_usec - c1.tv_usec;
    t_DFAload   = ((double)seconds * 1000 + (double)useconds/1000.0);
	
	seconds  = c4.tv_sec  - c3.tv_sec;
	useconds = c4.tv_usec - c3.tv_usec;
    t_in     = ((double)seconds * 1000 + (double)useconds/1000.0);
#ifdef DEBUG	
	seconds  = c5.tv_sec  - c4.tv_sec;
	useconds = c5.tv_usec - c4.tv_usec;
    t_exec   = ((double)seconds * 1000 + (double)useconds/1000.0);
	
    printf("udfa.cu: t_exec= %lf(ms)\n", t_exec);
#endif	

    printf("Execution times: DFA loading (from text): %lf(ms), Input stream loading: %lf(ms), GPU mem alloc: %lf(ms), GPU kernel execution: %lf(ms), Result collecting: %lf(ms), GPU mem release: %lf(ms)\n", t_DFAload, t_in, t_alloc, t_kernel, t_collect, t_free);
	
#ifdef DEBUG	
	//Write timing result to file
	double t_DFAs[7];
	t_DFAs[0] = t_alloc;
	t_DFAs[1] = t_kernel;
	t_DFAs[2] = t_collect;
	t_DFAs[3] = t_free;
	t_DFAs[4] = t_DFAload;
	t_DFAs[5] = t_in;
	t_DFAs[6] = t_exec;
	
	if (timing_filename != NULL)
		fp_timing.write((char *)t_DFAs, 7*sizeof(double));
	if (blksiz_filename != NULL)
		fp_blksiz.write((char *)&blockSize, sizeof(int));
#endif
	
	// close the file	
	fp.close();

#ifdef DEBUG	
	if (timing_filename != NULL)
		fp_timing.close();
	if (blksiz_filename != NULL)
		fp_blksiz.close();
#endif
	
    for (unsigned int i = 0; i < n_subsets; i++) {
		delete dfa_vec[i];
	}
	
	cfg.get_controller().dealloc_host_all();
	
	cudaDeviceReset();//Explicitly destroys and cleans up all resources associated with the current device in the current process. Note that this function will reset the device immediately. It is the caller's responsibility to ensure that the device is not being accessed by any other host threads from the process when this function is called.
	//To prevent strange memory leak in some machines (or drivers)
	
	free(rulestartvec);
	
	return 0;
}

bool ParseCommandLine(int argc, char *argv[])
{
	int CurrentItem = 1;
	int retVal;

	while (CurrentItem < argc)
	{

		if (strcmp(argv[CurrentItem], "-a") == 0)
		{
			CurrentItem++;
			base_name=argv[CurrentItem];
			CurrentItem++;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-i") == 0)
		{
			CurrentItem++;
			char *input_filename = NULL;
			input_filename=argv[CurrentItem];
			cfg.set_input_file_name(input_filename);
			CurrentItem++;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-p") == 0)
		{
			CurrentItem++;
			unsigned int parallel_packets;
			retVal = sscanf(argv[CurrentItem],"%d", &parallel_packets);
			cfg.set_packets(parallel_packets);
			if(retVal!=1 || parallel_packets < 1 ){
				printf("Invalid parallel_packets number: %s\n", argv[CurrentItem]);
				return false;
			}
			CurrentItem++;
			continue;
		}
		
		if (strcmp(argv[CurrentItem], "-T") == 0)
		{
			CurrentItem++;
			unsigned int threads_per_block;
			retVal = sscanf(argv[CurrentItem],"%d", &threads_per_block);
			cfg.set_threads_per_block(threads_per_block);
			if(retVal!=1 || threads_per_block < 1 ){
				printf("Invalid THREADS_PER_BLOCK number: %s\n", argv[CurrentItem]);
				return false;
			}
			CurrentItem++;
			continue;
		}
#ifdef DEBUG		
		if (strcmp(argv[CurrentItem], "-f") == 0)
		{
			CurrentItem++;
			timing_filename=argv[CurrentItem];
			CurrentItem++;
			continue;
		}
		
		if (strcmp(argv[CurrentItem], "-ft") == 0)
		{
			CurrentItem++;
			blksiz_filename=argv[CurrentItem];
			CurrentItem++;
			continue;
		}		
#endif
        if (strcmp(argv[CurrentItem], "-g") == 0)
		{
			CurrentItem++;
			unsigned int groups;
			retVal = sscanf(argv[CurrentItem],"%d", &groups);
			cfg.set_groups(groups);
			if(retVal!=1 || groups < 1 ){
				printf("Invalid SUB-RULESETS number: %s\n", argv[CurrentItem]);
				return false;
			}
			CurrentItem++;
			continue;
		}
		
		if (strcmp(argv[CurrentItem], "-N") == 0)
			{
				CurrentItem++;
				retVal = sscanf(argv[CurrentItem],"%d", &total_rules);
				if(retVal!=1 || total_rules < 1 ){
					printf("Invalid TOTAL_RULES number: %s\n", argv[CurrentItem]);
					return false;
				}
				CurrentItem++;
				continue;
		}

		if (strcmp(argv[CurrentItem], "-O") == 0)
			{
				CurrentItem++;
				retVal = sscanf(argv[CurrentItem],"%d", &blksiz_tuning);
				if(retVal!=1 || blksiz_tuning > 1 ){
					printf("Invalid blksiz_tuning param: %s\n", argv[CurrentItem]);
					return false;
				}
				CurrentItem++;
				continue;
		}

		if (strcmp(argv[CurrentItem], "-m") == 0)
			{
				CurrentItem++;
				retVal = sscanf(argv[CurrentItem],"%d", &automata_format);
				if(retVal!=1 || automata_format > 1 ){
					printf("Invalid automata_format param: %s\n", argv[CurrentItem]);
					return false;
				}
				CurrentItem++;
				continue;
		}
		
		/*if (strcmp(argv[CurrentItem], "-P") == 0)
		{
			CurrentItem++;
			print_parameters = 1;
			continue;
		}*/

		if (strcmp(argv[CurrentItem], "-h") == 0 ||
				strcmp(argv[CurrentItem], "-?") == 0)
		{
			CurrentItem++;
			Usage();
			return false;
		}
	}


	return true;
}

void Usage(void) {
    char string[]= "USAGE: ./dfa_engine [OPTIONS] \n" \
					 "\t-a <file> :   automata name (must NOT contain the file extension)\n" \
					 "\t-i <file> :   input file (with file extension)\n"  \
					 "\t-T <n>    :   number of threads per block (overwritten if block size tuning feature is used)\n" \
					 "\t-g <n>    :   number of graphs (or DFAs) to be executed (default: 1)\n" \
					 "\t-p <n>    :   number of parallel packets to be examined (default: 1)\n"\
					 "\t-N <n>    :   total number of rules (subgraphs)\n" \
					 "\t-O <n>    :   0 - block size tuning not enabled; 1 - block size tuned (optional, default: 0 - not tuned)\n" \
					 "\t-m <n>    :   0 - automata in binary format; 1 - automata in MNRL format (optional, default: 0 - binary)\n" \
#ifdef DEBUG
					 "\t-f <name> :   timing result filename (optional, default: empty)\n" \
					 "\t-ft <name>:   blocksize filename (optional, default: empty)\n" \
#endif
					 "\t-h        :   prints this message\n" \
					 "Ex:\t./dfa_engine -a ./data/simple -i ./data/simple.input -T 1 -g 1 -p 1 -N 3\n" \
					 "\t./dfa_engine -a ./data/simple -i ./data/simple.input -T 1 -g 1 -p 1 -N 3 -m 1\n" \
					 "\t./dfa_engine -a ./data/simpletwo -i ./data/simpletwo.input -T 1 -g 2 -p 1 -N 6 -m 1\n" \
					 "\t./dfa_engine -a ./data/simpletwo -i ./data/simpletwo.input -T 2 -g 2 -p 1 -N 6 -m 1 -O 1\n";
    fprintf(stderr, "%s", string);
}

/**
 * Get the size of a file.
 * @return The filesize, or 0 if the file does not exist.
 */
 size_t getFilesize(const char* filename) {
    struct stat st;
    if(stat(filename, &st) != 0) {
        return 0;
    }
    return st.st_size;   
}