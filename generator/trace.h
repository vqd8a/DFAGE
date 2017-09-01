/*
 * Copyright (c) 2007 Michela Becchi and Washington University in St. Louis.
 * All rights reserved
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. The name of the author or Washington University may not be used
 *       to endorse or promote products derived from this source code
 *       without specific prior written permission.
 *    4. Conditions of any other entities that contributed to this are also
 *       met. If a copyright notice is present from another entity, it must
 *       be maintained in redistributions of the source code.
 *
 * THIS INTELLECTUAL PROPERTY (WHICH MAY INCLUDE BUT IS NOT LIMITED TO SOFTWARE,
 * FIRMWARE, VHDL, etc) IS PROVIDED BY  THE AUTHOR AND WASHINGTON UNIVERSITY
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR WASHINGTON UNIVERSITY
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS INTELLECTUAL PROPERTY, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * */

/*
 * File:   trace.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 *
 * Description: 
 * 
 * This class is used to test the automaton traversal.
 * Each trace has associated the file representing the input text.
 * Traversal functions are provided.
 */

#ifndef TRACE_H_
#define TRACE_H_

#include "dfa.h"
#include "nfa.h"
#include "hybrid_fa.h"
#include "fa_memory.h"
#include "dfas_memory.h"

#define MIN_INPUTS 50000   //minimum size of the input, in case of probabilistic traversal
#define MAX_INPUTS 1000000 //maximum size of the input, in case of probabilistic traversal

#define CACHE_HIT_DELAY 1
#define CACHE_MISS_DELAY 30
#define MAX_MEM_REF CSIZE+1+CSIZE/4

class trace{
	
	//name of the trace file
	char *tracename;
	
	//trace file
	FILE *tracefile;
	
	//seed used in probabilistic traversal
	int seed;
	
public:

	/* constructor */
	trace(char *filename=NULL);
	
	/* de-allocator */
	~trace();
	
	/* sets the trace */
	void set_trace(char *filename);
	
	/* returns the trace file */
	FILE *get_trace();
	
	/* traverses the given nfa and prints statistics to stream
	 * Note: assume that split-state was not called (epsilon auto-loop meaningless) */
	void traverse(NFA *nfa, FILE *stream=stdout);
	
	/* traverses the given dfa and prints statistics to stream */ 
	void traverse(DFA *dfa, FILE *stream=stdout);
	
	/* traverses the given compressed dfa and prints statistics to stream */
	void traverse_compressed(DFA *dfa, FILE *stream=stdout);
	
	/* traverses the given hybrid-fa and prints statistics to stream */ 
	void traverse(HybridFA *hfa, FILE *stream=stdout);
	
	/* generates "bad" traffic for the given DFA (from the given state s) 
	 * => returns: the next input character 
	 */
	int bad_traffic(DFA *dfa, state_t s);
	
	/* generates "average" traffic for the given DFA (from the given state s) 
	 * => returns: the next input character 
	 */
	int avg_traffic(DFA *dfa, state_t s);
	
	/* generates synthetic traffic for the given DFA (from the given state s) 
	 * - p_fw is the probability of moving forward 
	 * => returns: the next input character
	 */
	int syn_traffic(DFA *dfa, state_t s, float p_fw);
	
	/* generates synthetic traffic for the given NFA (from the given set of active states) 
	 * - p_fw is the probability of moving forward 
	 * - forward is true if we want to move forward, false if we want to maximize the active set size 
	 * => returns: the next input character 
	 */
	int syn_traffic(nfa_set *active_set, float p_fw, bool forward);
	
	/* generates a traffic trace for the given NFA  
	   - in_seed is the initial seed of the probabilistic traversal
	   - p_fw is the probability of moving forward 
	   - forward is true if we want to move forward, false if we want to maximize the active set size
	   - trace_name is the name of the trace file to be generated 
	   => returns: the generated trace file
	   */
	FILE *generate_trace(NFA *nfa,int in_seed, float p_fw, bool forward, char *trace_name);
	
	/* traverse the memory populated by a single DFA/NFA - store the traversal statistics in data */
	void traverse(fa_memory   *memory, double *data, int seed=0, float p_fw=1, bool forward=true);
		
	/* traverse the memory populated by DFAs - store the traversal statistics in data */
	void traverse(dfas_memory *memory, double *data);
		
	
private:

	//traverse a DFA-memory
	void traverse_dfa(fa_memory *, double *data, int seed=0, float p_fw=1);
	
	//traverse a NFA-memory
	void traverse_nfa(fa_memory *, double *data, int seed=0, float p_fw=1, bool forward=true);
	
	//returns the next set of active states
	nfa_set *get_next(nfa_set *active_set, int c, bool forward=false);
	
	//returns the next deepest state reachable from the given active set on the indicated input character
	NFA *get_next_forward(nfa_set *active_set, int c);
	
};

inline FILE *trace::get_trace(){return tracefile;}

#endif /*TRACE_H_*/
