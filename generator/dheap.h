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
 */

/*
 * File:   dheap.h
 * Author: Jon Turner, Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * Comments:  adapted from Jon Turner's code, class CSE542: http://www.arl.wustl.edu/~jst/cse/542/src/index.html
 * 
 * Description: implements a d-heap data structure by maintaining a subset of items {1,..., m}, where item has a key
 */
 
#ifndef __DHEAP_H
#define __DHEAP_H

typedef long keytyp;
typedef unsigned long item;

#define Null 0

class dheap {
	int	N;			// max number of items in heap
	int	n;			// number of items in heap
	int	d;			// base of heap
	item	*h;			// {h[1],...,h[n]} is set of items
	unsigned long	*pos;			// pos[i] gives position of i in h
	keytyp	*kvec;			// kvec[i] is key of item i

	item	minchild(item);		// returm smallest child of item
	void	siftup(item,unsigned long);	// move item up to restore heap order
	void	siftdown(item,unsigned long);	// move item down to restore heap order
public:		dheap(unsigned long=100,int=2);
		~dheap();
	item	findmin();		// return the item with smallest key
	keytyp	key(item);		// return the key of item
	bool	member(item);		// return true if item in heap
	bool	empty();		// return true if heap is empty
	void	insert(item,keytyp);	// insert item with specified key
	void	remove(item);		// remove item from heap
	item 	deletemin();		// delete and return smallest item
	void	changekey(item,keytyp);	// change the key of an item
	void	print(FILE *f);		// print the heap
};

// Return item with smallest key.
inline item dheap::findmin() { return n == 0 ? Null : h[1]; }

// Return key of i.
inline keytyp dheap::key(item i) { return kvec[i]; }

// Return true if i in heap, else false.
inline bool dheap::member(item i) { return pos[i] != Null; }

// Return true if heap is empty, else false.
inline bool dheap::empty() { return n == 0; };

#endif //__DHEAP_H


