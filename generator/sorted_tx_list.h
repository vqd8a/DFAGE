#ifndef __SORTED_TX_LIST_H      
#define __SORTED_TX_LIST_H

#include "stdinc.h"

class sorted_tx_list;

#define FOREACH_SORTEDLIST(sortedlist,sl) \
	for(sorted_tx_list *sl = sortedlist; sl!=NULL && sl->state()!=NO_STATE; sl=sl->next())

class sorted_tx_list{
	unsigned _char;
	state_t _state;
	sorted_tx_list *_next;
public:
	sorted_tx_list(unsigned c=0, state_t state=NO_STATE);
	~sorted_tx_list();	
	bool empty();
	void insert(unsigned c, state_t state);
	unsigned c();
	state_t state();
	sorted_tx_list *next();
	unsigned size();
	void log(FILE* file=stdout);
};	 

inline bool sorted_tx_list::empty(){ return (_state==NO_STATE && _next==NULL);}

inline unsigned sorted_tx_list::c(){ return _char;}
inline state_t sorted_tx_list::state(){ return _state;}
inline sorted_tx_list *sorted_tx_list::next(){ return _next;}

#endif /*__SORTED_TX_LIST_H*/
