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
 */

/*
 * File:   nfa.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 *
 * Descriprion:
 *
 * 		Implements a NFA.
 * 
 * 		Each NFA state is represented through a NFA object.
 * 
 *		NFA consists of:
 * 			- a set of labeled transitions, in the form <symbol_t, next NFA state>. 
 *   		  Note that several transitions leading to different states on the same input character may exist.
 *		    - a (possibly empty) set of epsilon transitions.
 * 
 * 		Additionally, each state has a unique identifier and a depth, the latter expressing the
 * 		minimum distance from the entry state.
 * 
 * 		This class provides:
 * 		- methods to build a NFA by adding transitions, concatenating and or-ing NFAs, copying NFAs, etc.
 * 		- function to transform a NFA into the equivalent without epsilon transitions
 * 		- function to reduce a NFA by collapsing equivalent sub-NFAs
 *		- function to transform a NFA into DFA (subset construction procedure)  
 * 		- code to export the NFA into format suitable for dot program (http://www.graphviz.org/)
 */
 
#ifndef __NFA_H_
#define __NFA_H_

#include "stdinc.h"
#include "linked_set.h"
#include "dfa.h"
#include <list>
#include <set>
#include <map>
#include <utility>
#include <vector>

using namespace std;

class NFA;

/* redefinitions used for convenience */

typedef set<NFA*> nfa_set; 				//set of NFA states
typedef set<state_t> state_set; 				//set of NFA states
typedef list<NFA*> nfa_list;			//list of NFA states
typedef set<pair<symbol_t,NFA*>*> pair_set;	//set of labeled transitions

//iterator on given set of labeled transitions
#define FOREACH_PAIRSET(pairset,it) \
	for(pair_set::iterator it=pairset->begin();it!=pairset->end();++it)

//iterator on labeled transitions
#define FOREACH_TRANSITION(it) FOREACH_PAIRSET(transitions,it)

//iterator on set of NFA states	
#define FOREACH_SET(set_id,it) \
	for(nfa_set::iterator it=set_id->begin();it!=set_id->end();++it)

//iterator on list of NFA states
#define FOREACH_LIST(list_id,it) \
	for(nfa_list::iterator it=list_id->begin();it!=list_id->end();++it)

//set membership
#define SET_MBR(_set,item) (_set->find(item) != _set->end()) 

//set deletion	
#define SET_DELETE(_set,item) _set->erase(_set->find(item))

/* identifier of the last  NFA state instantiated */
static state_t nstate_nr=0;  

char *get_line(FILE *file);

class NFA{
	
	/* state identifier */
	state_t id;
	
	/* labeled transitions */
	pair_set *transitions;
	
	/* epsilon transitions */
	nfa_list *epsilon;
		
	/* emptyset for non-accepting states, set of rule-numbers for accepting states */
	linked_set *accepting;
	
	/* depth of the state in the graph */
	unsigned int depth;
	
	/* marking (used for algorithmical purposes) */
	int marked;
	
	/* to avoid recursion during deletion*/
	int delete_only_me;
	
	/* pointer to first state in the finite state machine the NFA-state belongs to */
	NFA *first;
	
	/* pointer to last state in the finite state machine the NFA-state belongs to */
	NFA *last;
	
public:

	/* indicate whether the regular expression has the ignore case option or not */
	static bool ignore_case;

	/* number of rules */
	static int num_rules;	
	
public:

	/* constructor */
	NFA();
	
	/* (recursive) de-allocator */
	~NFA();
	
	
	/*
	 * FUNCTIONS TO QUERY A NFA STATE 
	 */
	
	/* size in terms of number of states */
	unsigned int size();
	
	/* returns state identifier */
	state_t get_id();
	
	/* returns transitions on character c (set of next NFA states) */
	nfa_set *get_transitions(symbol_t c);
	
	/* returns transitions on character c (set of <char, next NFA state>) */
	pair_set *get_transition_pairs(symbol_t c);
	
	/* returns all the labeled transitions */
	pair_set *get_transitions();
	
	/* is there a transition to nfa on character c?  */
	bool has_transition(symbol_t c, NFA *nfa);
	
	/* returns epsilon transitions */
	nfa_list *get_epsilon();
	
	/* returns the set of accepted regular expressions */
	linked_set *get_accepting();
	
	/* returns the first state of the current NFA */
	NFA *get_first();
	
	/* returns the last state of the current NFA */
	NFA *get_last();
	
	/* returns the depth of the current NFA state */
	unsigned get_depth();
	
	/* returns true if the current NFA state has at least an outgoing (labeled or epsilon) transition */
	bool connected();
	
	/*
	 * FUNCTIONS TO MODIFY A NFA STATE 
	 */
	
	/* adds transition to nfa on character c */
	void add_transition(symbol_t c, NFA *nfa);
		
	/* adds a transition on symbol c and returns the last state of the new NFA */
	NFA *add_transition(symbol_t c);
	
	/* adds a range of transitions (from symbol from_s to symbol to_s to a given state or a new NFA) and returns the last state of the new NFA */
	NFA *add_transition(symbol_t from_s, symbol_t to_s, NFA *nfa=NULL);

	/* adds a collection of transitions (as specified in int_set) and returns the last state of the new NFA.*/
	NFA *add_transition(int_set *);	
	
	/* adds a wildcard (to a given state or a new NFA) and returns the last state of the new NFA */
	NFA *add_any(NFA *nfa=NULL);
	
	/* adds an epsilon transition (to a given state or to a new state if nfa is not specified) and returns the new state */
	NFA *add_epsilon(NFA *nfa=NULL);
	
	/* returns the last state of the NFA obtained by concatenating the current one to tail */
	NFA *link(NFA *tail);
	
	/* makes a duplicate of the current NFA and returns it */
	NFA *make_dup();
	
	/* returns the state machine representing from lb to ub repetitions of the current one. The upper bound
	 * may be INFINITY */
	NFA *make_rep(int lb,int ub);
		
	/* returns the last state of the NFA obtained by or-ing the current one to alt */
	NFA *make_or(NFA *alt);
	
	/* sets the current NFA-state as accepting (and increment the rule number) */
	void accept();
	
	/* adds the given set of rule-numbers to the accepting set */ 
	void add_accepting(linked_set *numbers);
	
	/*
	 * FUNCTIONS TO OBTAIN EQUIVALENT NFAs 
	 */
	
	/* remove the epsilon transition from the NFA (building an equivalent NFA) */
	void remove_epsilon();
	
	/* reduce the current NFA (building an equivalent NFA) by removing equivalent sub-NFAs */
	void transform();
	
	/* reduce the current NFA (building an equivalent NFA) by removing equivalent sub-NFAs */
	void reduce();
	
	/* builds an equivalent NFA characterized by the fact that no NFA state has more than one 
	 * transition on a given character. 
	 * This is done by replacing a state with several outgoing transitions on the same character
	 * with a set of states connected through epsilon transitions.
	 */  
	void split_states();

	/*
	 * CONVERSION FUNCTIONS
	 */
	
	/* subset construction operation - converts the NFA into DFA and returns the latter */
	DFA *nfa2dfa();
	 
	/* exports the NFA into format suitable for dot program (http://www.graphviz.org/) */
	void to_dot(FILE *file, const char *title);
	
	/* exports the NFA into format suitable for automated parsing */
	void to_file(FILE *file, const char *title);

	
	/*
	 * MISC
	 */

	/* sets the depth information on the whole NFA */
	void set_depth(unsigned d=0);

	/* builds a set containing all the NFA states */
	void traverse(nfa_set *queue);

	/* builds a list containing all the NFA states in breath-first traversal order */
	void traverse(nfa_list *queue);
	
	/* reset the state identifier and reassigns them to the NFA states in breath-first traversal order (staring from first_id) */
	void reset_state_id(state_t first_id=0);
	
	/* returns the epsilon closure of the current NFA state */
	nfa_set *epsilon_closure();
	
	/* avoids recursion while deleting a NFA state */
	void restrict_deletion();
	
	/* marks current state */
	void mark();
	
	/* is the state marked ? */
	bool is_marked();
	
	/* resets marking (algorithmical purposed) on the whole NFA */
	void reset_marking();
	
	/* for debugging purposes - prints out the NFA */
	void analyze(FILE *file=stdout);
	
	/* recursive call to dot-file generation */
	void to_dot(FILE *file, bool blue=false);
	void to_file(FILE *file, bool blue=false);
	
	static NFA *from_file(FILE *file);

private:
	
	/* returns the set of identifiers corresponding to the given set of NFA states
	 * (convenient method used during subset construction) */
	set<state_t> *set_NFA2ids(nfa_set *fas);
	
	/* for debugging purposes: prints out the NFAs in the set */
	void debug_set(nfa_set *s);
	
	/* compares the transitions of nfa1 and nfa2 */
	bool same_transitions(NFA *nfa1, NFA *nfa2);

	/* function used to duplicate a NFA */
	void dup_state(NFA *nfa, NFA *copy,std::map <state_t,NFA *> *mapping);
	
	/* makes times linked copies of the current NFA */
	NFA *make_mdup(int times);

	/* remove the duplicate states from the NFA (building an equivalent NFA) - called by reduce() */
	void remove_duplicates();
	
	void set_first_last(NFA *father);
	

};

inline void NFA::accept(){this->accepting->insert(++num_rules);}

inline state_t NFA::get_id(){ return id;}

inline void NFA::add_accepting(linked_set *numbers){accepting->add(numbers);}

inline linked_set *NFA::get_accepting(){return accepting;}

inline bool NFA::is_marked(){return marked;}

inline void NFA::mark(){marked=1;}

inline void NFA::restrict_deletion(){delete_only_me=1;} 

inline bool NFA::connected(){return !(transitions->empty() && epsilon->empty());}

inline nfa_list *NFA::get_epsilon() { return epsilon; }

inline pair_set *NFA::get_transitions() { return transitions; }

inline unsigned NFA::get_depth() {return depth;}

#endif /*__NFA_H_*/
