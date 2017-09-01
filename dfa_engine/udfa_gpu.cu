/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * udfa_gpu.cu
 */

#include "common.h"
#include "udfa_gpu.h"

__global__ void udfa_kernel(
				state_t *input_dfa_state_tables,
				symboln *input,
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
	
	//jump to the right input string
	input += (pkt_size * blockIdx.x/fetch_bytes); 

	unsigned int accum_dfa_state_table_length = accum_dfa_state_table_lengths[dfa_id];
	
	state_t current_state = 0;

	//loop over payload
	for(unsigned int p=0; p<cur_pkt_size; p+=fetch_bytes, input++){
		symboln Input_ = *input;//fetch 4 bytes from the input string
		for (unsigned int byt = 0; byt < fetch_bytes; byt++) {
			unsigned int Input = Input_ & 0xFF;//extract 1 byte
			Input_  = Input_ >> 8;//Input_ right-shifted by 8 bits
		
			//query the state table on the input symbol for the next state
			current_state = input_dfa_state_tables [current_state * CSIZE + Input + accum_dfa_state_table_length];
			
			if (current_state < 0) {//Added for matching operation: check if the dst state is an accepting state
				current_state = -current_state;
				//match_offset[match_vec_size*blockIdx.x + shr_match_count + dfa_id*match_vec_size*nstreams] = p;
				//match_states[match_vec_size*blockIdx.x + shr_match_count + dfa_id*match_vec_size*nstreams] = current_state;
				tmp_match.off  = p + byt;
				tmp_match.stat = current_state;
				match_array[shr_match_count + match_vec_size*(blockIdx.x + dfa_id*gridDim.x)] = tmp_match;
				
				shr_match_count = shr_match_count + 1;
			}		
		}
	}
	match_count[blockIdx.x + dfa_id*gridDim.x] = shr_match_count;
}
