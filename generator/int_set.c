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
 * File:   int_set.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 */

#include "int_set.h"
#include "stdinc.h"

int_set::int_set(unsigned int N1) {
	N = N1; 
	item = new bool[N+1];
	for (unsigned int i = 0; i <= N; i++) item[i] = false;
	num_el=0;
	first=UNDEF;
}

int_set::~int_set() { delete [] item; }

// Change num_el of int_set. Discard old value.
void int_set::reset(unsigned int N1) {
	delete [] item; N = N1; item = new bool[N+1];
	for (unsigned int i = 0; i <= N; i++) item[i] = false;
	num_el=0;
	first=UNDEF;
}
	
// Remove all elements from int_set.
void int_set::clear() {
	for (unsigned int i = 0; i <= N; i++) item[i] = false;
	num_el=0;
	first=UNDEF;
}

// Print the contents of the int_set.
void int_set::print() {
	fprintf(stderr,"[ ");
	for (unsigned int i = 0; i <= N; i++)
		if (item[i]) fprintf(stderr,"%d ",i);
	fprintf(stderr,"]\n");
}

void int_set::operator=(int_set& L) {
	if (N != L.N) {
		N = L.N;
		delete [] item; item = new bool[L.N+1];
	};
	num_el=L.num_el;	
	first=L.first;
	for (unsigned int i = 0; i <= N; i++) item[i] = L.item[i];
}

void int_set::insert(unsigned int i){
	if (i < 0 || i > N) fatal("int_set::insert: item out of range");
	if (first==UNDEF || first > i) first=i;
	if (!item[i]) num_el++;
	item[i]=true;
}

void int_set::remove(unsigned int i)	{
	if (i < 0 || i > N) fatal("int_set::remove: item out of range");
	if (first==i) first=suc(i);
	if (item[i])num_el--;
	item[i]=false;
}

bool int_set::mbr(unsigned int i){		
	if (i < 0 || i > N) fatal("int_set::mbr: item out of range");
	return (item[i]);
}	

unsigned int int_set::suc(unsigned int i){
	if (i < 0 || i > N) fatal("int_set::suc: item out of range");
	if (!item[i] || i==N) return UNDEF;
	for (unsigned int j=i+1;j<=N;j++)
		if (item[j]) return j;
	return UNDEF;	
} 

void int_set::add(int_set *s){
	if (s->N>N) N=s->N;
	unsigned int item=s->first;
	while(item!=UNDEF){
		insert(item);
		item=s->suc(item);
	}
}

void int_set::negate(){
	first=UNDEF;
	num_el=0;
	for (unsigned int i = 0; i < N; i++){
		item[i] = !item[i];
		if (item[i]){
			if (first==UNDEF) first=i;
			num_el++;
		}	
	}
}
	
