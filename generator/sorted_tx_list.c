#include "sorted_tx_list.h"

sorted_tx_list::sorted_tx_list(unsigned c, state_t state){
	_char=c;
	_state=state;
	_next=NULL;
}

sorted_tx_list::~sorted_tx_list(){
	if (_next!=NULL) delete _next;
}

unsigned sorted_tx_list::size(){
	unsigned r=0;
	sorted_tx_list *tl=this;
	while(tl!=NULL){
		if (tl->_state!=NO_STATE) r++;
		tl=tl->_next;
	}
	return r;
}

void sorted_tx_list::insert(unsigned c, state_t state){
	if (_state==NO_STATE){
		_char=c;
		_state=state;
	}else if(c<_char){
		sorted_tx_list *tl =new sorted_tx_list(_char,_state);
		tl->_next=_next;
		_char=c;
		_state=state;
		_next=tl;
	}else if (c==_char){
		if (state<_state){
			sorted_tx_list *tl =new sorted_tx_list(_char,_state);
			tl->_next=_next;
			_state=state;
			_next=tl;
		}else if (state>_state){
			if (_next==NULL)
				_next=new sorted_tx_list(c,state);
			else
				_next->insert(c,state);
		}
	}else if (_next==NULL){
		_next = new sorted_tx_list(c,state);
	}else{
		_next->insert(c,state);
	}
}

void sorted_tx_list::log(FILE *file){
	if (_state==NO_STATE) fprintf(file, "EMPTY\n");
	else{
		sorted_tx_list *tl=this;
		while (tl!=NULL){
			fprintf(file,"(%ld,%ld)",tl->_char,tl->_state);
			tl=tl->_next;
		}
	}
}
