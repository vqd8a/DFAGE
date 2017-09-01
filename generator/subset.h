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
 * File:   subset.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: 
 * 		Data structure used in order to store the mapping between a DFA/NFA state-ID and the 
 * 		corresponding set of NFA state-IDs. 
 * 
 * 		This data structure is used:
 * 		1) during the subset construction operation (that is, when transforming a NFA into its DFA counterpart). 
 * 		2) when reducing a NFA by collapsing common prefixes.
 */

#ifndef __SUBSET_H_
#define __SUBSET_H_

#include "stdinc.h"
#include "dfa.h"
#include "nfa.h"
#include "dheap.h"
#include <set>

using namespace std;
 
class subset{
    state_t nfa_id; //the ID of the NFA state the set of HORIZONTALLY TRAVERSED subsets corresponds to - for NFA reduction
	state_t dfa_id; //the ID of the DFA state the set of HORIZONTALLY TRAVERSED subsets corresponds to - for subset construction
	NFA *nfa_obj;   // NFA object in case of NFA reduction
	subset *next_o; //the next subset in the _HORIZONTAL_ dimension
	subset *next_v; //the next subset in the _VERTICAL_ dimension	
	
public:

	//instantiates a new subset with the given state id
	subset(state_t id, NFA *nfa=NULL);
	
	//(recursively) deallocates the subset
	~subset();
	
	// For subset construction:
	// - queries a subset
	// - if the subset is not present, then:
	//	 (1) creates a new state in the DFA 
	//   (2) creates a new subset (updating the whole data structure), and associates it to the newly created DFA state 
	// nfaid_set: set of nfa_id to be queried
	// dfa: dfa to be updated, in case a new state has to be created
	// dfaid: pointer to the variable containing the value of the (new) dfa_id
	// returns true if the subset (for nfaid_sets) was preesisting, and false if not. In the latter
	// case, a new DFA state is created
	bool lookup(set <state_t> *nfaid_set, DFA *dfa, state_t *dfa_id);
	
	// For NFA reduction:
	// - queries a subset
	// - if the subset is not present, then:
	//	 (1) creates a new NFA state 
	//   (2) creates a new subset (updating the whole data structure), and associates it to the newly created NFA state 
	// nfaid_set: set of nfa_id to be queried
	// nfa: pointer to the variable containing the value of the (new) NFA state
	// returns true if the subset (for nfaid_sets) was preesisting, and false if not. In the latter
	// case, a new NFA state is created
	bool lookup(set <state_t> *nfaid_set, NFA **nfa);
	
	//dumps the subset (for logging purposes)
	void dump(FILE *file=stdout);
	
private:	
	
	//analyze the subsets and populates the heap with the number of
	//NFA/DFA states corresponding to a subset w/ different size 
	void analyze(dheap *heap,int size=1);
	
};

#endif /*__SUBSET_H_*/
