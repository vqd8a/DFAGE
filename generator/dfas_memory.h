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
 * File:   dfas_memory.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 *
 * Description: implements the memory layout for a multiple DFAs
 * 
 */

#ifndef DFAS_MEMORY_H_
#define DFAS_MEMORY_H_

#include "stdinc.h"
#include "dfa.h"
#include "cache.h"
#include "fa_memory.h"

class dfas_memory{
	
	//threshold on number of outgoing transition per states leading to full representation
	int threshold;
	
	//memory layout		
	layout_t layout;
	
	//number of associated DFAs
	unsigned num_dfas;	
		
	//associated DFAs
	DFA **dfas;
	
	//number of states
	unsigned num_states;
	
	//for each DFA, number of states in memory before it (0 for DFA0, size(DFA0) for DFA1, and so on...)
	state_t *state_offset; 
		
	//for each DFA, number of fully represented states
	state_t *base_states;  
		
	//for each DFA, the address in memory it starts at
	long *memory_base; 
	  
	//for each DFA, the offset at which the first non-fully-specified state starts
	long *memory_offset; 
	
	//for each state, its address
	long *state_address; 
	
	//for each state, the number of labeled transitions
	int *num_labeled_tr; 
	
	//for each state, its first level bitmap
	int *bitmap; 
	
	//associated cache, if any
	cache *_cache;
	
	//size of on-chip memory
	int _onchip_size;
	
public:

	//constructor
	dfas_memory(unsigned num_dfas_in, DFA **dfas_in, layout_t layout_in, int threshold_in);
	
	//de-allocator
	~dfas_memory();
	
	//initializes the memory
	void init(long *data=NULL); 

	//returns an array of fa_memory addresses assiciated with querying the specified DFA state w/ the given input character
	int *get_dfa_accesses(unsigned dfa_nr, state_t state,int c, int *size);
	
	//returns the associated DFAs
	DFA **get_dfas();
	
	//returns the number of DFAs
	unsigned get_num_dfas();
	
	//returns the total number of states
	unsigned get_num_states();
	
	//returns the state index of the given state in the specified DFA
	unsigned get_state_index(unsigned dfa_nr, state_t s);
	
	//returns whether the given state uses default transitions
	bool allow_def_tx(unsigned dfa_nr, state_t s);
	
	//sets the cache
	void set_cache(cache_size_t,line_size_t,assoc_t);

	//returns the fa_memory access type associated with the given address
	mem_access_t read(int address);
	
	//returns the cache
	cache *get_cache();

	//sets the on-chip size
	void set_onchip_size(int size);

	//returns the on-chip size
	int get_onchip_size();
	
};

inline unsigned dfas_memory::get_num_dfas(){return num_dfas;}

inline unsigned dfas_memory::get_num_states(){return num_states;}

inline unsigned dfas_memory::get_state_index(unsigned dfa_nr, state_t s){return s+state_offset[dfa_nr];}

inline DFA **dfas_memory::get_dfas(){return dfas;}

inline bool dfas_memory::allow_def_tx(unsigned dfa_nr, state_t s){return (state_address[s+state_offset[dfa_nr]]>=memory_offset[dfa_nr]);} //MB 

inline cache *dfas_memory::get_cache(){return _cache;}

inline void dfas_memory::set_onchip_size(int size){_onchip_size=size;}

inline int dfas_memory::get_onchip_size(){return _onchip_size;}

#endif /*DFAS_MEMORY_H_*/
