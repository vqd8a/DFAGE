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
 * File:   subset.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 */

#include "subset.h"
#include <iterator>

subset::subset(state_t id,NFA *nfa){
	nfa_id=id;
	dfa_id=NO_STATE;
	next_o=NULL;
	next_v=NULL;
	nfa_obj=nfa;
}

subset::~subset(){
	if (next_o!=NULL) delete next_o;
	if (next_v!=NULL) delete next_v;
}



//version used in subset construction
bool subset::lookup(set <state_t> *nfaid_set, DFA *dfa, state_t *dfaid){
	if (nfaid_set->empty()) nfaid_set->insert(NO_STATE); //this to consider the case of emptyset.
	state_t id = *(nfaid_set->begin());
	if (id==nfa_id){
		nfaid_set->erase(nfaid_set->begin());
		if(nfaid_set->empty()){
			if (dfa_id==NO_STATE){
				dfa_id=dfa->add_state();
				*dfaid=dfa_id;
				return false;
			}else{
				*dfaid=dfa_id;
				return true;
			}			
		}else{
			id = *(nfaid_set->begin());
			if (next_o==NULL){
				next_o=new subset(id);
			}else if (next_o->nfa_id>id){
				subset *new_node=new subset(id);
				new_node->next_v=next_o;
				next_o=new_node;
			} 
			return next_o->lookup(nfaid_set,dfa,dfaid);
		}
	}else if (id>nfa_id){
		if (next_v==NULL || id<next_v->nfa_id){
			subset *new_node=new subset(id);
			new_node->next_v=next_v;
			next_v=new_node;
		}
		return next_v->lookup(nfaid_set,dfa,dfaid);
	}else{
		fatal("subset::lookup: condition should never occur");
	}
}


//version used in NFA reduction
bool subset::lookup(set <state_t> *nfaid_set, NFA **nfa){
	if (nfaid_set->empty()) fatal("subset:: NFA lookup with empty set");
	state_t id = *(nfaid_set->begin());
	if (id==nfa_id){
		nfaid_set->erase(nfaid_set->begin());
		if(nfaid_set->empty()){
			if (nfa_obj==NULL){
				nfa_obj = new NFA();
				*nfa=nfa_obj;
				return false;
			}else{
				*nfa=nfa_obj;						
				return true;
			}			
		}else{
			id = *(nfaid_set->begin());
			if (next_o==NULL){
				next_o=new subset(id);
			}else if (next_o->nfa_id>id){
				subset *new_node=new subset(id);
				new_node->next_v=next_o;
				next_o=new_node;
			} 
			return next_o->lookup(nfaid_set,nfa);
		}
	}else if (id>nfa_id){
		if (next_v==NULL || id<next_v->nfa_id){
			subset *new_node=new subset(id);
			new_node->next_v=next_v;
			next_v=new_node;
		}
		return next_v->lookup(nfaid_set,nfa);
	}else{
		fatal("subset::lookup: condition should never occur");
	}
}

void subset::analyze(dheap *heap,int size){
	if (this->dfa_id!=NO_STATE || this->nfa_obj!=NULL){
		if(heap->member(size))
			heap->changekey(size,heap->key(size)+1);
		else
			heap->insert(size,1);	
	}
	if(next_v!=NULL) next_v->analyze(heap,size);
	if(next_o!=NULL) next_o->analyze(heap,size+1);
}

void subset::dump(FILE *file){
	dheap *heap=new dheap(1500);
	analyze(heap);
	float avg=0;
	unsigned long max=0;
	unsigned long min=0xFFFFFFFF;
	unsigned long tot=0;
	fprintf(file,"\nsubset analysis\n==========================\n[#states:subset size]\n");
	while(!heap->empty()){
		int level=heap->deletemin();
		int num=heap->key(level);
		max=(level>max)?level:max;
		min=(level<min)?level:min;
		tot+=num;
		avg+=level*num;
		fprintf(file,"[%ld : %ld]\n",num,level);
	}		
	fprintf(file,"min=%ld, max=%ld, avg=%f\n\n",min,max,avg/tot);
	delete heap;
}

