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
 * File:   hybrid_fa.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: Implements a hybrid-FA consisting of a head-DFA and several tail-automata.
 * 				The subset construction algorithm is guided by MAX_TX and MAX_HEAD_SIZE setting and special() function.
 * 
 */

#ifndef __HYBRID_FA_H_
#define __HYBRID_FA_H_

#include "stdinc.h"
#include "dfa.h"
#include "nfa.h"
#include <list>
#include <set>
#include <map>
#include <utility>

using namespace std;

typedef map <state_t,nfa_set*>::iterator border_it;

class HybridFA{
	
	/* orginal NFA (where to get the tail information from */
	NFA *nfa;
	
	/* head-DFA */
	DFA *head;
	
	/* border: mapping between DFA state and corresponding set of NFA states */ 
	map <state_t, nfa_set*> *border;
	
	/* non-special states (for construction: to avoid that a state classified as non-special at the beginning
	 * of the construction becomes special later on) */
	nfa_set *non_special;
	nfa_set *is_special;
	
public:

	/* constructor */
	HybridFA(NFA *_nfa);
	
	/* de-allocator */
	~HybridFA();

	/* returns underlying NFA */
	NFA *get_nfa();

	/* returns the head */
	DFA *get_head();
	
	/* returns the border */
	map <state_t, nfa_set*> *get_border();
	
	/* return the number of tail-automata */
	unsigned get_num_tails();
	
	/* return the tail size */
	unsigned get_tail_size();

	/* performs DFA minimization on the head-DFA*/
	void minimize();
	
	/* exports the head of the Hybrid-FA into format suitable for dot program (http://www.graphviz.org/) */
	void to_dot(FILE *file, const char *title);
	
	void to_file(FILE *file, const char *title);
	
private:

	// builds the hybrid-FA for the current NFA.
	void build();

	// determines when a NFA state is special (that is, it must not be expanded).
	bool special(NFA *nfa);
	
	// modified nfa so that its more suitable for Hybrid-FA creation
	void optimize_nfa_for_hfa(NFA *nfa, unsigned depth);
	
};

inline NFA *HybridFA::get_nfa(){return nfa;}

	/* returns the head */
inline DFA *HybridFA::get_head(){return head;}
	
	/* returns the border */
inline map <state_t, nfa_set*> *HybridFA::get_border(){return border;}

#endif /*__HYBRID_FA_H_*/

