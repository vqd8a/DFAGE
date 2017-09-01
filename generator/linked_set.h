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
 * File:   linked_set.h
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 * 
 * Description: Implemented a SORTED linked set of unsigned integers 
 */

#ifndef LINKED_SET_H_
#define LINKED_SET_H_

#include <stdio.h>

#define _INVALID 0xFFFFFFFF

typedef unsigned int item_t;

class linked_set{
	
	// value
	item_t data;
	
	//pointer to the next in set
	linked_set *next;	
	
public:
	
	// instantiated a linked set with the element item. If item is not specified
	// an empty set is instantiated
	linked_set(item_t item=_INVALID);
	
	// (recursive) destructor
	~linked_set();
	
	// is the set empty?
	bool empty();
	
	//is the current set equal to s (does it contains the same elements?)
	bool equal(linked_set *s);
	
	//insert an items into a set
	void insert(item_t item);
	
	//insert into the current set all the items from s
	void add(linked_set *s);
	
	//delete from the current set all the items from s
	void remove(linked_set *s);
	
	//returns the value associated with the current element of the set
	item_t value();
	
	//returns the next element in the linked list
	linked_set *succ();
	
	//removes all elements from the set
	void clear();
	
	//is data in linked_set?
	bool member(item_t val);
	
	//size of the list
	unsigned size();
	
	//print the content
	void print(FILE *stream=stdout);
	
};

inline item_t linked_set::value(){return data;}    
inline linked_set *linked_set::succ(){return next;}

#endif /*LINKED_SET_H_*/
