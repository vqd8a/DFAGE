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
 * File:   partition.c
 * Author: Jon Turner, Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * Comments:  adapted from Jon Turner's code, class CSE542: http://www.arl.wustl.edu/~jst/cse/542/src/index.html
 * 
 */

#include "stdinc.h"
#include "partition.h"

partition::partition(int N) {
// Initialize partition so that every element is in separate set.
	n = N;
	vec = new pnode[n+1];
	for (int i = 1; i <= n; i++) {
		vec[i].p = i; vec[i].rank = 0;
	}
	nfind = 0;
	vec[Null].p = Null; vec[Null].rank = 0;
}

partition::~partition() { delete [] vec; }

void partition::reset(int x){
	vec[x].p=x;
	vec[x].rank=0;
}

int partition::find(int x) {
// Find and return the canonical element of the set containing x.
	nfind++;
	if (x != vec[x].p) vec[x].p = find(vec[x].p);
	return vec[x].p;
}

int partition::link(int x, int y) {
// Combine the sets whose canonical elements are x and y.
// Return the canonical element of the new set.
	if (vec[x].rank > vec[y].rank) {
		int t = x; x = y; y = t;
	} else if (vec[x].rank == vec[y].rank)
		vec[y].rank++;
	vec[x].p = y;
	return y;
}

int partition::findroot(int x) {
// Return the canonical element of the set containing x.
// Do not restructure tree in the process.
	if (x == vec[x].p) return(x);
	else return findroot(vec[x].p);
}

void partition::print() {
// Print the partition.
	int i,j; int *root = new int[n+1];
	for (i = 1; i <= n; i++) root[i] = findroot(i);
	for (i = 1; i <= n; i++) {
		if (i == vec[i].p) {
			printf("%d:",i);
			for (j = 1; j <= n; j++)
				if (j != i && root[j] == i) printf(" %d",j);
			putchar('\n');
		}
	}
	delete [] root;
}
