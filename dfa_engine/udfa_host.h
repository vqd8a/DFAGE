/*
 * Vinh Dang
 * vqd8a@virginia.edu
 *
 * UDFA host Object
 */

#ifndef UDFA_HOST_H
#define UDFA_HOST_H

#include <fstream>
#include <set>
#include <vector>
#include <stdio.h>

#include "common.h"
#include "common_configs.h"
#include "finite_automaton.h"

class Packets;
 
unsigned int udfa_run(std::vector<FiniteAutomaton *> fa, Packets &packets, unsigned int n_subsets, unsigned int packet_size, int *rulestartvec, double *t_alloc, double *t_kernel, double *t_collect, double *t_free, int *blocksize, int blksiz_tuning);
 
#endif
