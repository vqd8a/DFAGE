#include "comp_dfa.h"

compDFA::compDFA(DFA *dfa,unsigned alphabet,symbol_t *alph_tx){
	size=dfa->size();
	default_tx=new state_t[size];
	labeled_tx=(sorted_tx_list **)allocate_array(sizeof(sorted_tx_list *),size);
	accepted_rules = (linked_set **) allocate_array(sizeof(linked_set *),size);
	dead_state = dfa->get_dead_state();
	marking = new bool[size];
	alphabet_size = alphabet;
	alphabet_tx=alph_tx;
	if (dfa->get_default_tx()==NULL) dfa->fast_compression_algorithm();
	for (state_t s=0;s<size;s++){
		default_tx[s]=dfa->get_default_tx()[s];
		labeled_tx[s]=new sorted_tx_list();
		FOREACH_TXLIST(dfa->get_labeled_tx()[s],it) labeled_tx[s]->insert(alphabet_tx[(*it).first],(*it).second);
		accepted_rules[s]=new linked_set();
		accepted_rules[s]->add(dfa->get_accepted_rules()[s]);
		marking[s]=false;
	
	}
	if (DEBUG){
		unsigned tx=0;
		for (state_t s=0;s<size;s++) tx+=labeled_tx[s]->size();
		printf("compressed DFA::\n - states=%ld\n",size);
		printf(" - (labeled) tx=%ld\n",tx);
		printf(" - alphabet=%ld\n",alphabet_size);
	}
}

compDFA::~compDFA(){
	delete [] default_tx;
	for (state_t s=0;s<size;s++){
		delete labeled_tx[s];
		delete accepted_rules[s];
	}
	free(labeled_tx);
	free(accepted_rules);
	delete [] marking;
}

state_t compDFA::next_state(state_t s,unsigned c){
	sorted_tx_list *tx=labeled_tx[s];
	while (tx!=NULL && !tx->empty()){
		if (tx->c()==c) return tx->state();
		tx=tx->next();
	}
	return next_state(default_tx[s],c);
}

void compDFA::reset_marking(){
	for (state_t s=0;s<size;s++) marking[s]=false;	
}
