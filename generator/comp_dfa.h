#ifndef __comp_DFA_H      
         
#define __comp_DFA_H

#include "dfa.h"
#include "stdinc.h"
#include "linked_set.h"
#include "sorted_tx_list.h"

using namespace std;
 
/*
 * Implements a compressed DFA (compression happened through alphabet reduction and default transition compression.
 */

class compDFA {

public:
	
 	/* size (number of states) of the compressed DFA */
	unsigned size;
	
	/* default transitions (one per state) */
    state_t *default_tx;
    
    /* labeled transitions (for each state, set of them) */
    sorted_tx_list **labeled_tx;
    
    /* for each state, set of accepted rules (empty-set if the state is non-accepting) */
    linked_set **accepted_rules;
              
    /* alphabet translation table */
    symbol_t *alphabet_tx;
    
    /* reduced alphabet size */
    unsigned alphabet_size;	

    /* marking - for algorithmic purposes */
    bool *marking;
    
    /* dead state */
    state_t dead_state;
    
public:        
        
	/* instantiates a new compDFA from the corresponding full DFA */
	compDFA(DFA *dfa, unsigned alphabet, symbol_t *alph_tx);
	
	/* deallocates the compDFA */
	~compDFA();
	
	state_t next_state(state_t,unsigned);
	
	void mark(state_t);
	
	void reset_marking();
	
	state_t get_dead_state();
};

inline void compDFA::mark(state_t s){marking[s]=true;}

inline state_t compDFA::get_dead_state(){return dead_state;}

#endif /*__comp_DFA_H*/
