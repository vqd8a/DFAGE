/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * UDFA Object
 */

#ifndef UDFA_GPU_H
#define UDFA_GPU_H

#include <device_functions.h>
#include "common.h"

__global__ void udfa_kernel(
				state_t *input_dfa_state_tables,
				symboln *input,
				unsigned int *pkt_size_vec, unsigned int pkt_size,
				unsigned int *match_count, match_type *match_array, unsigned int match_vec_size,
				unsigned int *accum_dfa_state_table_lengths, unsigned int n_subsets);
#endif
