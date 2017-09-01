#ifndef __comp_NFA_H      
         
#define __comp_NFA_H

#include "nfa.h"
#include "stdinc.h"
#include "linked_set.h"
#include "sorted_tx_list.h"

using namespace std;
 
/*
 * Implements a compressed NFA (compression through alphabet reduction)
 */

class compNFA {

public:
	
 	/* size (number of states) of the compressed NFA */
	unsigned size;
    
    /* transitions (a sorted list for each state) */
    sorted_tx_list **tx;
    
    /* for each state, set of accepted rules (empty-set if the state is non-accepting) */
    linked_set **accepted_rules;
    
    /* reduced alphabet size */
    unsigned alphabet_size;	
    
    /* alphabet translation table */
    symbol_t *alphabet_tx;
    
    /* marking - for algorithmic purposes */
    bool *marking;
    
public:        
        
	/* instantiates a new compNFA from the corresponding full NFA */
	compNFA(NFA *nfa);
	
	/* deallocates the compNFA */
	~compNFA();
	
	/* mark the current state */
	void mark(state_t);
	
	/* reset the marking in the whole compressed NFA */
	void reset_marking();
	
private:	
	
	/* reduce the alphabet */
	void alphabet_reduction();
};

inline void compNFA::mark(state_t s){marking[s]=true;}

#endif /*__comp_NFA_H*/
