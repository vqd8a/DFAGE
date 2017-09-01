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
 * File:   dfa.h
 * Author: Jon Turner, Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * Comments:  adapted from Jon Turner's code, class CSE542: http://www.arl.wustl.edu/~jst/cse/542/src/index.html
 * 
 */

#include "stdinc.h"
#include "wgraph.h"

wgraph::wgraph(unsigned int N1, unsigned long M1) {
	N = N1; M = M1;
	firstedge = new edge[N+1];
	edges = new wgedge[M+1];
	n = N; m = 0;
	for (vertex u = 0; u <= n; u++) firstedge[u] = Null;
	edges[0].l = edges[0].r = edges[0].lnext = edges[0].rnext = 0;
	edges[0].wt = 0;
}

wgraph::~wgraph() { delete [] firstedge; delete [] edges; }

void wgraph::reset() {
	delete [] firstedge; 
	delete [] edges;
	firstedge = new edge[N+1];
	edges = new wgedge[M+1];
	for (vertex u = 0; u <= n; u++) firstedge[u] = Null;
	edges[0].l = edges[0].r = edges[0].lnext = edges[0].rnext = 0;
	edges[0].wt = 0;
	n = N; m=0;
}


char wgraph::cflush(char c, FILE *f) {
// Read up to first occurrence of c or EOF.
	char c1; while ((c1 = getc(f)) != EOF && c1 != c) {} return c1;
}

bool wgraph::getedge(edge e, FILE* f) {
// Read one edge from *f into edges[e]. Return true on success, else false.
	char c;
	if (cflush('(',f) == EOF) return false;
	
	fscanf(f,"%d",&edges[e].l);
	if (cflush(',',f) == EOF) return false;
	fscanf(f,"%d",&edges[e].r);
	
	if (cflush(',',f) == EOF) return false;
	fscanf(f,"%d",&edges[e].wt);
	if (cflush(')',f) == EOF) return false;
	return true;
}

edge wgraph::join(vertex u, vertex v, weight W) {
// Join u and v with edge of given weight. Return edge number.
	if (++m > M) fatal("wgraph: too many edges");
	edges[m].l = u; edges[m].r = v; edges[m].wt = W;
	edges[m].lnext = firstedge[u]; firstedge[u] = m;
	edges[m].rnext = firstedge[v]; firstedge[v] = m;
	return m;
}

void wgraph::bldadj() {
// Build adjacency lists.
	for (vertex u = 0; u < n; u++) firstedge[u] = Null;
	for (edge e = m; e >= 1; e--) {
		edges[e].lnext = firstedge[edges[e].l];
		firstedge[edges[e].l] = e;
		edges[e].rnext = firstedge[edges[e].r];
		firstedge[edges[e].r] = e;
	}
}

bool wgraph::get(FILE* f) {
// Get graph from f.
	if (fscanf(f,"%d%d",&N,&M) != 2) return false;
	delete [] firstedge; delete [] edges;
	firstedge = new edge[N+1]; edges = new wgedge[M+2];
	n = N; m = 1;
	for (int i = 1; i <= 2*M; i++) {
		// each edge appears twice in input
		if (!getedge(m,f)) break;
		if (edges[m].l > n || edges[m].r > n)
			fatal("wgraph::get: out of range vertex number");
		if (edges[m].l < edges[m].r) {
			if (m > M) fatal("wgraph::get: edge mismatch");
			m++;
		}
	}
	m--; bldadj();
	return true;
}

void wgraph::put(FILE* f) {
// Put graph out on f.
	int i; vertex u; edge e;
	fprintf(f,"%d %d\n",n,m);
	for (u = 0; u < n; u++) {
		i = 0;
		for (e = first(u); e != Null; e = next(u,e)) {
			fprintf(f,"%ld=(%2d,%2d,%2d)  ",e, u,mate(u,e),w(e));
            if ((++i)%5 == 0) putc('\n',f);
        }
	}
	putc('\n',f);
}

void wgraph::esort() {
// Sort the edges by cost.
	hsort(); bldadj();
}

void wgraph::hsort() {
// Sort edges according to weight, using heap-sort.
	int i, mm, p, c; weight w; wgedge e;

	for (i = m/2; i >= 1; i--) {
		// do pushdown starting at i
		e = edges[i]; w = e.wt; p = i;
		while (1) {
			c = 2*p;
			if (c > m) break;
			if (c+1 <= m && edges[c+1].wt >= edges[c].wt) c++;
			if (edges[c].wt <= w) break;
			edges[p] = edges[c]; p = c;
		}		
		edges[p] = e;
	}
	// now edges are in heap-order with largest weight edge on top

	for (mm = m-1; mm >= 1; mm--) {
		e = edges[mm+1]; edges[mm+1] = edges[1];
		// now largest edges are m, m-1,..., mm+1
		// edges 1,...,mm form a heap with largest weight edge on top
		// pushdown from 1
		w = e.wt; p = 1;
		while (1) {
			c = 2*p;
			if (c > mm) break;
			if (c+1 <= mm && edges[c+1].wt >= edges[c].wt) c++;
			if (edges[c].wt <= w) break;
			edges[p] = edges[c]; p = c;
		}		
		edges[p] = e;
	}
}

void wgraph::to_dot(char *filename){
	FILE *file=fopen(filename,"w");
	fprintf(file, "digraph \"%s\" {\n", filename);
	for (vertex u = 0; u < n; u++) {
    	fprintf(file, " %ld [shape=circle,label=\"%ld\"];\n",u,u);
    }
    for (edge e = 1; e <=m; e++) {
        fprintf(file, "%ld -> %ld [shape=none,label=\"%ld\"];\n", left(e),right(e),w(e));                       
    }
    fprintf(file, "}\n");
    fclose(file);
}
