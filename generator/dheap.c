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
 * File:   dheap.c
 * Author: Jon Turner, Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * Comments:  adapted from Jon Turner's code, class CSE542: http://www.arl.wustl.edu/~jst/cse/542/src/index.html
 * 
 */

#include "stdinc.h"
#include "dheap.h"

// parent of item, leftmost and rightmost children
#define p(x) (((x)+(d-2))/d)
#define left(x) (d*((x)-1)+2)
#define right(x) (d*(x)+1)

dheap::dheap(unsigned long N1, int d1) {
// Initialize a heap to store items in {1,...,N}.
	static int x = 0; x++;
	N = N1; d = d1; n = 0;
	h = new item[N+1];
	pos = new unsigned long[N+1];
	kvec = new keytyp[N+1];
	for (unsigned long i = 1; i <= N; i++) pos[i] = Null;
	h[Null] = pos[Null] = Null; kvec[Null] = 0;
}

dheap::~dheap() { delete [] h; delete [] pos; delete [] kvec; }

void dheap::insert(item i, keytyp k) {
// Add i to heap.
	kvec[i] = k; n++; siftup(i,n);
}

void dheap::remove(item i) {
// Remove item i from heap. Name remove is used since delete is C++ keyword.
	int j = h[n--];
	     if (i != j && kvec[j] <= kvec[i]) siftup(j,pos[i]);
	else if (i != j && kvec[j] >  kvec[i]) siftdown(j,pos[i]);
	pos[i] = Null;
}

item dheap::deletemin() {
// Remove and return item with smallest key.
	if (n == 0) return Null;
	item i = h[1];
	remove(h[1]);
	return i;
}

void dheap::siftup(item i, unsigned long x) {
// Shift i up from position x to restore heap order.
	int px = p(x);
	while (x > 1 && kvec[h[px]] > kvec[i]) {
		h[x] = h[px]; pos[h[x]] = x;
		x = px; px = p(x);
	}
	h[x] = i; pos[i] = x;
}

void dheap::siftdown(item i, unsigned long x) {
// Shift i down from position x to restore heap order.
	int cx = minchild(x);
	while (cx != Null && kvec[h[cx]] < kvec[i]) {
		h[x] = h[cx]; pos[h[x]] = x;
		x = cx; cx = minchild(x);
	}
	h[x] = i; pos[i] = x;
}

// Return the position of the child of the item at position x
// having minimum key.
item dheap::minchild(unsigned long x) {
	int y, minc;
	if ((minc = left(x)) > n) return Null;
	for (y = minc + 1; y <= right(x) && y <= n; y++) {
		if (kvec[h[y]] < kvec[h[minc]]) minc = y;
	}
	return minc;
}

void dheap::changekey(item i, keytyp k) {
// Change the key of i and restore heap order.
	keytyp ki = kvec[i]; kvec[i] = k;
	     if (k < ki) siftup(i,pos[i]);
	else if (k > ki) siftdown(i,pos[i]);
}

// Print the contents of the heap.
void dheap::print(FILE *file) {
	int x;
	for (x = 1; x <= n; x++) fprintf(file,"(%ld=%ld)",h[x],kvec[h[x]]);
	fprintf(file,"\n");
}
