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
 * File:   linked_set.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 */

#include "linked_set.h"
#include "stdinc.h"

linked_set::linked_set(item_t item){
	data=item;
	next=NULL;
}
	
linked_set::~linked_set(){
	if (next!=NULL) delete next;
}
	
bool linked_set::empty(){
	return (data==_INVALID);
}
	
bool linked_set::equal(linked_set *s){
	if (s==NULL && !empty())
		return false;
	if (data!=s->data)
		return false;
	if (next==NULL && s->next==NULL)
		return true;
	if (next==NULL || s->next==NULL)
		return false;
	return next->equal(s->next);					
}
	
//insertion happen in sorted order
void linked_set::insert(item_t item){
	if (data==_INVALID){
		data=item;
	}else if (item<data){
		//can occurr just when this is the first
		linked_set *ns=new linked_set(data);
		data=item;
		ns->next=next;
		next=ns;
	}else if (item==data){
		return;
	}else{ //item > data
		if (next==NULL){
			next=new linked_set(item);
		}else if(item < next->data){
			linked_set *ns = new linked_set(item);
			ns->next=next;
			next=ns;
		}else{
			next->insert(item);
		}	
	}
}
	
void linked_set::add(linked_set *par){
	linked_set *ls=par;
	while(ls!=NULL && ls->data!=_INVALID){
		insert(ls->data);
		ls=ls->next;
	}	
}

void linked_set::remove(linked_set *par){
	if (this->empty() || par==NULL || par->empty()) return;
	linked_set *lt  = this;
	linked_set *ltp = NULL;
	linked_set *tmp;
	for (linked_set *lp=par; lp!=NULL && lt!=NULL; lp=lp->next){
		while (lt!=NULL && lt->data < lp->data){
			ltp=lt;
			lt=lt->next;
		}
		if (lt!=NULL && lt->data==lp->data){
			if (lt==this){
				if (lt->next==NULL){
					lt->data=_INVALID;
					break;
				}
				else{
					lt->data = lt->next->data;
					tmp = lt->next;
					lt->next = lt->next->next;
					tmp->next = NULL;
					delete tmp;
				}	
			}else{
				if (lt->next==NULL){
					ltp->next=NULL;
					delete lt;
					lt=NULL;
				}else{
					lt->data = lt->next->data;
					tmp = lt->next;
					lt->next = lt->next->next;
					tmp->next = NULL;
					delete tmp;
				}
			}
		}
	}	
}

void linked_set::clear(){
	if (next!=NULL) delete next;
	next=NULL;
	data=_INVALID;
}

bool linked_set::member(item_t val){
	if (data==val)
		return true;
	if (next==NULL)
		return false;
	return next->member(val);		
}

unsigned linked_set::size(){
	if (data==_INVALID)
		return 0;
	if (next==NULL)
		return 1;
	return 1+next->size();		
}

void linked_set::print(FILE *stream){
	if (empty()) fprintf(stream, "EMPTY set\n");
	else{
		for (linked_set *ls=this;ls!=NULL;ls=ls->next) fprintf(stream,"0x%x ",ls->data);
		fprintf(stream,"\n");
	}
}
