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
 * File:   fa_memory.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 *
 * Description: implements the fa_memory layout for a DFA/NFA
 * 
 */

#ifndef FA_MEMORY_H_
#define FA_MEMORY_H_

#include "stdinc.h"
#include "dfa.h"
#include "nfa.h"
#include "cache.h"

//invalid address
#define NO_ADDRESS 0xFFFFFFFF

//type of layout (linear, bitmapped, indirect addressed)
typedef enum {LINEAR=0, BITMAP, INDIRECT_ADDR} layout_t;

//type of fa_memory access (cache hit, cache miss, direct fa_memory access)
typedef enum {CACHE_HIT, CACHE_MISS, DIRECT} mem_access_t;


class fa_memory{
	
	//threshold on number of outgoing transition per states leading to full representation
	int threshold;
	
	//fa_memory layout
	layout_t layout;
		
	//associated DFA
	DFA *dfa;
	
	//associated NFA
	NFA *nfa;
	
	//for each DFA/NFA state, index in fa_memory
	long *state_index;
	
	//for each DFA/NFA state, address in fa_memory
	long *state_address;
	
	/* 
	 * DFA: 0-fa_memory_offset: fully specified states (CSIZE entries)
	 *      fa_memory_offset- : compressed states w/ default transitions
	 * NFA: 0-fa_memory_offset                  : fully specified w/ epsilon transition
	 * 		fa_memory_offset-fa_memory_offset1  : fully specified w/o epsilon transition
	 * 		fa_memory_offset1-fa_memory_offset2 : w/ epsilon transition
	 * 		fa_memory_offset2-				    : w/o epsilon transition
	 */
	long fa_memory_offset; 
	long fa_memory_offset1; 
	long fa_memory_offset2; 
	
	//number of fully specified states
	long base_states;
	
	// for each state, number of outgoing labeled transitions
	int *num_labeled_tr;
	
	// for each state, number of epsilon transitions
	int *epsilon;
	
	//for each state, first level bitmap
	int *bitmap; 
	
	//associated cache (if any)
	cache *_cache;
	
	//on_chip size
	int _onchip_size;
	
public:

	//constructor w/ DFA
	fa_memory(DFA *, layout_t, int threshold);
	
	//constructor w/ NFA
	fa_memory(NFA *, layout_t, int threshold);
	
	//de-allocator
	~fa_memory();
	
	//initialize the fa_memory
	void init(long *data=NULL); 

	//returns an array of fa_memory addresses assiciated with querying the specified DFA state w/ the given input character
	int *get_dfa_accesses(state_t state,int c, int *size);

	//returns an array of fa_memory addresses assiciated with querying the specified NFA state w/ the given input character
	int *get_nfa_accesses(NFA *nfa_state,int c, int *size);
	
	//returns the DFA
	DFA *get_dfa();
	
	//returns the NFA
	NFA *get_nfa();
	
	//returns whether state s uses default transitions
	bool allow_def_tx(state_t s);
	
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

private:
	
	//initialize the fa_memory with the given DFA
	void init_dfa(long *data);
	
	//initialize the fa_memory with the given NFA
	void init_nfa(long *data);

};

inline DFA *fa_memory::get_dfa(){return dfa;}

inline NFA *fa_memory::get_nfa(){return nfa;}

inline bool fa_memory::allow_def_tx(state_t s){return (state_address[s]>=fa_memory_offset);} 

inline cache *fa_memory::get_cache(){return _cache;}

inline void fa_memory::set_onchip_size(int size){_onchip_size=size;}

inline int fa_memory::get_onchip_size(){return _onchip_size;}

#endif /*FA_MEMORY_H_*/
