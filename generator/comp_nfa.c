#include "comp_nfa.h"

compNFA::compNFA(NFA *nfa){
	nfa->reset_state_id();
	size=nfa->size();
	tx=(sorted_tx_list **)allocate_array(sizeof(sorted_tx_list *),size);
	accepted_rules = (linked_set **) allocate_array(sizeof(linked_set *),size);
	marking = new bool[size];
	alphabet_size = CSIZE;
	alphabet_tx = new symbol_t[CSIZE];
	for (symbol_t c=0;c<CSIZE;c++) alphabet_tx[c]=c;
	nfa_list *queue=new nfa_list();
	nfa->traverse(queue);
	while (!queue->empty()){
		NFA *state=queue->front(); queue->pop_front();
		tx[state->get_id()] = new sorted_tx_list();
		FOREACH_PAIRSET(state->get_transitions(),it){
			tx[state->get_id()]->insert((*it)->first,(*it)->second->get_id());
		}
		accepted_rules[state->get_id()]=new linked_set();
		accepted_rules[state->get_id()]->add(state->get_accepting());
		marking[state->get_id()]=0;
	}
	delete queue;
#if 0
//	alphabet_reduction();
	for (state_t s=0;s<size;s++){
		if (!tx[s]->empty()){
			sorted_tx_list *new_tx=new sorted_tx_list();
			FOREACH_SORTEDLIST(tx[s],tl) new_tx->insert(alphabet_tx[tl->c()],tl->state());
			delete tx[s];
			tx[s]=new_tx;
		}
	}
#endif
}

compNFA::~compNFA(){
	for (state_t s=0;s<size;s++){
		delete tx[s];
		delete accepted_rules[s];
	}
	free(tx);
	free(accepted_rules);
	delete [] alphabet_tx;
	delete [] marking;
}

void compNFA::reset_marking(){
	for (state_t s=0;s<size;s++) marking[s]=0;	
}

bool equal(state_set *s, state_set *t){
	if (s->size()!=t->size()) return false;
	for(state_set::iterator it=s->begin();it!=s->end();++it)
		if (t->find(*it)==t->end()) return false;
	return true;
}

void compNFA::alphabet_reduction(){
	//initializations
	alphabet_size=0;
	for (symbol_t c=0;c<CSIZE;c++) alphabet_tx[c]=0;
	
	//utility vector
	unsigned char_covered[CSIZE];
	unsigned class_covered[CSIZE];
	unsigned remap[CSIZE];
	
	//reduction algorithm
	for (state_t s=0;s<size;s++){
		bool checked[CSIZE];
		state_set *next[CSIZE];
		for (symbol_t c=0;c<CSIZE;c++){
			checked[c]=false;
			next[c]=new state_set();
		}
		FOREACH_SORTEDLIST(tx[s],sl) next[sl->c()]->insert(sl->state());
		for (symbol_t j=0;j<CSIZE;j++){
			if (checked[j]) continue;
			for (symbol_t c=0;c<CSIZE;c++){
				char_covered[c]=0;
				class_covered[c]=0;
				remap[c]=0;
			}
			for (symbol_t c=0;c<CSIZE;c++){
				if(!checked[c] && equal(next[j],next[c])){
					checked[c]=true;
					char_covered[c]=1;
					class_covered[alphabet_tx[c]]=1;
				}
			}
			for (symbol_t c=0;c<CSIZE;c++){
				if(!char_covered[c] && class_covered[alphabet_tx[c]]){
					if(remap[alphabet_tx[c]]==0) remap[alphabet_tx[c]]=++alphabet_size;
				 	alphabet_tx[c]=remap[alphabet_tx[c]];
				}
			}
		} //end loop state t
		for (symbol_t j=0;j<CSIZE;j++) delete next[j];
	} //end loop state s
	alphabet_size++;
}
