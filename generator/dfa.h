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
 * File:   dfa.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 *
 * Description:
 * 
 * 		Implements a DFA.
 * 
 * 		State transitions are represented through a bidimensional array (state table).
 * 
 * 		The class contains:
 * 		- getter and setter methods to access important DFA information
 * 		- O(nlogn) minimization algorithm
 * 		- code to export and import DFA from textual representation
 * 		- code to export the DFA in format suitable for dot program (http://www.graphviz.org/)
 * 		- code to compress the DFA through default transitions. Different algorithms are provided. 
 * 
 */

#ifndef __DFA_H               
#define __DFA_H

#include "stdinc.h"
#include "int_set.h"
#include "linked_set.h"
#include "wgraph.h"
#include "partition.h"
#include <map>
#include <set>
#include <list>

using namespace std;

class DFA;

typedef set<DFA*> dfa_set; 
typedef pair<symbol_t, state_t> tx_t;
typedef list<tx_t> tx_list;

//iterator on set of DFA	
#define FOREACH_DFASET(set_id,it) \
	for(dfa_set::iterator it=set_id->begin();it!=set_id->end();++it)

#define FOREACH_TXLIST(txlist,it) \
	for(tx_list::iterator it=txlist->begin();it!=txlist->end();++it)


class DFA {

		/*number of states in the DFA*/
        unsigned _size;

		/* state_table[old_state][symbol]=new_state*/
	    state_t **state_table;

		/* default transitions (one per state) */
        state_t *default_tx;
        
        /* labeled transitions (NO_STATE when the corresponding transition is represented by
         * the default transition, next state otherwise) */
        tx_list **labeled_tx;

		/* for each state, set of accepted rules (empty-set if the state is non-accepting) */
        linked_set **accepted_rules;
 
        /* marking used for algorithmical purposed */
        int *marking;
        
        /* for each state, its depth (minimum "distance" from the entry state 0) */
        unsigned int *depth;
        
        /* state involving no progression */
        state_t dead_state;
             
private:

	/* number of entry allocated (for dynamic allocation) */
    unsigned int entry_allocated;
		
	/* builds the default and the labeled transitions for a D2FA starting from
	 * the forest, the partition data structure and the distance vector 
	 */	
	void d2fa_graph_to_default_tx(wgraph *T,partition *P,int *d);
	
public:	

	/* instantiates a new DFA */
	DFA(unsigned=50);
	
	/* deallocates the DFA */
	~DFA();
	
	/* returns the number of states in the DFA */
	unsigned int size();

	/* sets the number of states in the DFA */
	void set_size(unsigned int n);
	
	/* equality operator */
	bool equal(DFA *);

	/* Adds a state and returns its identifier. Allocates the necessary data structures */
	state_t add_state();

	/* adds a transition from old_state to new_state on symbol c */
	void add_transition(state_t old_state, symbol_t c, state_t new_state);

	/* minimizes the DFA 
	 * Implementation of Hopcroft's O(n log n) minimization algorithm, follows
   	 * description by D. Gries.
   	 *
     * Time complexity: O(n log n)
     * Space complexity: O(c n), size < 4*(5*c*n + 13*n + 3*c) byte
     */
	void minimize();

	/* returns the next_state from state on symbol c */
	state_t get_next_state(state_t state, symbol_t c);
	
	/* returns the (possibly empty) list of accepted rules on a state */
	linked_set *accepts(state_t state);
	
	/* returns the transition table */
	state_t **get_state_table();
	
	/* returns the array of acceptstate pointers */
	linked_set **get_accepted_rules();
	
	/* returns the array of default transitions */
	state_t *get_default_tx();
	
	/* returns the labeled transitions table */
	tx_list **get_labeled_tx();
	
	/* marks the current state as visited (for algorithmical purposed) */
	void mark(state_t);

	/* returns true if a given state is marked */
	bool marked(state_t);

	/* resets the marking (algorithmical purposes) */
	void reset_marking();
	
	/* the given state become the dead state */
	void set_dead_state(state_t state);
	
	/* returns the dead state */
	state_t get_dead_state();
	
	/* dumps the DFA */
	void dump(FILE *log=stdout);
	
	/* exports the DFA to DOT format 
	 * download tool to produce visual representation from: http://www.graphviz.org/
	 */
	void to_dot(FILE *file, const char *title);

	void to_file(FILE *file, const char *title);
	
	/* dumps the DFA into file for later import */
	void put(FILE *file, char *comment=NULL);
	
	/* imports the DFA from file */
	void get(FILE *file);
	
	/* sets the state depth (minimum "distance") from the entry state 0 */
	void set_depth();
	
	/* gets the array of state depth */
	unsigned int *get_depth();
	
	/* 
	 * Computes the minimized alphabet for the given DFA using the algorithm in
	 * 
	 * Becchi M. and Crowley P., "An improved algorithm to accelerate regular expression evaluation", ANCS 2007.
	 * 
	 * Time complexity: O(n^2)
	 * Space complexity: O(n)
	 * 
	 * Returns the size of the reduced alphabet.
	 * Parameters:
	 * - alph_tx: alphabet translation table (alph_tx[c]=reduced symbol c)
	 * - init: true if the alphabet translation table must be initialized (default)
	 */ 
	unsigned alphabet_reduction(symbol_t *alph_tx,bool init=true);
	
	/* analyzes the character classes */
	void analyze_classes(symbol_t *classes, unsigned num_classes);
	
	/* computes next state using default and labeled transitions */
	state_t lookup(state_t s, symbol_t c);
	
	/* computes next state using default and labeled transitions;
	 * updates two statistics, indicating the number of traversals per state and the total number
	 * of state traversals.
	 */
	state_t lookup(state_t s, symbol_t c, unsigned **state_traversals, unsigned *total_traversals);
	
	/* returns the number of state traversals to perform a state lookup on given character */
	int traversals_per_lookup(state_t s, symbol_t c);
	
	/* checks that the default and labeled transitions are functionally equivalent to full-DFA */
	void crosscheck_default_tx();
		
	/* analyzes default transitions on current DFA */	
	void analyze_default_tx(short *classes, short  num_classes, int offset=0, FILE *file=stdout);
	
	
	/* Algorithms to build default transitions */
	
	/* Default transition compression algorithm described in the following paper:
	 * 
	 * Becchi M. and Crowley P., "An improved algorithm to accelerate regular expression evaluation", ANCS 2007.
	 * 
	 * - Time complexity: O(n^2)
	 * - Space complexity: O(n)
	 * - Resulting time complexity to process an input text of length N: (k+1/k)N
	 * 
	 * Input parameters:
	 * - k: the default transition from a state at depth d must fall to a state with depth <=d-k
	 * - bound: if > -1, gives an upper bound to the default path length.
	 * 
	 */
	void fast_compression_algorithm(int k=1,int bound=-1);
	
	/* D2FA compression algorithm, as described in the following paper:
	 * 
	 * S. Kumar et alt., "Algorithms to Accelerate Multiple Regular Expressions Matching for Deep Packet Inspection", in SIGCOMM 2006
	 * 
	 * - Time complexity: O(n^2logn)
	 * - Space complexity: O(n^2)
	 * - Resulting time complexity to process an input text of length N: (diameter/2+1)N
	 * 
	 * Input parameters: 
	 * - diameter: diameter bound
	 * - refined: see Kumar's paper
	 */
	void D2FA(int diameter, bool refined);
	
	/* Algorithm described in paper
	 * S. Kumar et alt., "Advanced Algorithms for Fast and Scalable Deep Packet Inspection," ANCS 2006
	 * 
	 * Returns the resulting number of trees.
	 */
	unsigned CD2FA();
	
};

inline void DFA::add_transition(state_t old_state, symbol_t c, state_t new_state){ state_table[old_state][c]=new_state;}	

inline bool DFA::marked(state_t state){return marking[state];}

inline void DFA::mark(state_t state){marking[state]=1;}

inline void DFA::reset_marking(){for (unsigned int i=0;i<_size;i++) marking[i]=0;}

inline state_t DFA::get_next_state(state_t state, symbol_t c) { return state_table[state][c]; }

inline linked_set *DFA::accepts(state_t state) { return accepted_rules[state]; }

inline unsigned int DFA::size(){ return _size;}

inline void DFA::set_size(unsigned int n){ _size = n;}

inline state_t **DFA::get_state_table(){return state_table;}

inline state_t *DFA::get_default_tx(){return default_tx;}

inline tx_list **DFA::get_labeled_tx(){return labeled_tx;}

inline linked_set **DFA::get_accepted_rules(){return accepted_rules;}

inline unsigned int *DFA::get_depth(){return depth;}

inline void DFA::set_dead_state(state_t state){dead_state=state;}

inline state_t DFA::get_dead_state(){return dead_state;}

#endif /*__DFA_H*/
