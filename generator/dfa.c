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
 * File:   dfa.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 */

#include "dfa.h"
#include "nfa.h"
#include <stdio.h>
#include <list>
#include "dheap.h"
#include <sys/time.h>

using namespace std;

#define MAX_DFA_SIZE_INCREMENT 50

DFA::DFA(unsigned N){
	dead_state=NO_STATE;
	entry_allocated=N;
	_size=0;
	state_table = allocate_state_matrix(entry_allocated);
	accepted_rules = (linked_set **)allocate_array(entry_allocated,sizeof(linked_set *));
	default_tx=NULL;
	labeled_tx=NULL;
	marking=allocate_int_array(entry_allocated);
	depth=NULL;
	for (state_t s=0;s<entry_allocated;s++){
		state_table[s]=NULL;
		accepted_rules[s]=NULL;
	}
}

DFA::~DFA(){
	for(state_t i=0;i<_size;i++){
		free(state_table[i]);
		if (accepted_rules[i]) delete accepted_rules[i];
	}
	free(state_table);	
	free(accepted_rules);
	if (default_tx!=NULL) free(default_tx);
	if (labeled_tx!=NULL){
		for (state_t s=0;s<_size;s++)  delete labeled_tx[s];
		free(labeled_tx);
	}
	free(marking);
	if (depth!=NULL) free(depth);
}

bool DFA::equal(DFA *dfa){
	if (_size!=dfa->_size) return false;
	for (state_t s=0;s<_size;s++){
		for (int c=0;c<CSIZE;c++)
			if (state_table[s][c]!=dfa->state_table[s][c]) return false;
		if ((accepted_rules[s]==NULL && (dfa->accepted_rules[s]!=NULL && !dfa->accepted_rules[s]->empty())) ||
			(dfa->accepted_rules[s]==NULL && (accepted_rules[s]!=NULL && !accepted_rules[s]->empty())) ||
			!accepted_rules[s]->equal(dfa->accepted_rules[s]))
			return false;
	}
	return true;
}

state_t DFA::add_state(){
	state_t state=_size++;
    if(DEBUG && state%1000==0 && state!=0){
	   printf("... state = %ld\n",state);
	   fflush(stdout);
    }
	if (state==NO_STATE)
		fatal("DFA:: add_state(): too many states!");
	if (state >= entry_allocated){
		entry_allocated += MAX_DFA_SIZE_INCREMENT;
		state_table = reallocate_state_matrix(state_table,entry_allocated);
		accepted_rules = (linked_set **)reallocate_array(accepted_rules,entry_allocated,sizeof(linked_set *));
		marking=reallocate_int_array(marking,entry_allocated);
		for (state_t s=_size;s<entry_allocated;s++){
			state_table[s]=NULL;
			accepted_rules[s]=NULL;
		}
	} 
	state_table[state] = allocate_state_array(CSIZE);
	accepted_rules[state]=new linked_set();
	marking[state]=false;
	int i; for (i=0;i<CSIZE;i++) state_table[state][i]=NO_STATE;
	return state;
}


void DFA::dump(FILE *log){
	fprintf(log,"Dumping DFA: %ld states\n",_size);
    for(state_t i=0;i<_size;i++){
    	fprintf(log,"> state # %d",i);
    	if (!accepted_rules[i]->empty()) {
    		fprintf(log," accepts ");
    		linked_set *s=accepted_rules[i];
    		while (s!=NULL){ fprintf(log,"[%ld]",s->value());s=s->succ();}
    	}
    	fprintf(log,"\n");
    	for(int j=0;j<CSIZE;j++){
    		if (state_table[i]!=NULL && state_table[i][j]!=0) 
	    		fprintf(log," - [%d/%c] %d\n",j,j,state_table[i][j]);
    	}
    }
}

/**
   * Minimization algorithm
   */

void DFA::minimize() {

	if (VERBOSE) fprintf(stdout,"DFA:: minimize: before minimization states = %ld\n",_size);
	
	unsigned long i;

    // the algorithm needs the DFA to be total, so we add an error state 0,
    // and translate the rest of the states by +1
    unsigned int n = _size+1;

    // block information:
    // [0..n-1] stores which block a state belongs to,
    // [n..2*n-1] stores how many elements each block has
    int block[2*n]; for(i=0;i<2*n;i++) block[i]=0;    

    // implements a doubly linked list of states (these are the actual blocks)
    int b_forward[2*n]; for(i=0;i<2*n;i++) b_forward[i]=0;
    int b_backward[2*n]; for(i=0;i<2*n;i++) b_backward[i]=0;  

    // the last of the blocks currently in use (in [n..2*n-1])
    // (end of list marker, points to the last used block)
    int lastBlock = n;  // at first we start with one empty block
    int b0 = n;   // the first block    

    // the circular doubly linked list L of pairs (B_i, c)
    // (B_i, c) in L iff l_forward[(B_i-n)*CSIZE+c] > 0 // numeric value of block 0 = n!
    int *l_forward = allocate_int_array(n*CSIZE+1);
    for(i=0;i<n*CSIZE+1;i++) l_forward[i]=0;
    int *l_backward = allocate_int_array(n*CSIZE+1);
    for(i=0;i<n*CSIZE+1;i++) l_backward[i]=0;
        
    int anchorL = n*CSIZE; // list anchor

    // inverse of the transition state_table
    // if t = inv_delta[s][c] then { inv_delta_set[t], inv_delta_set[t+1], .. inv_delta_set[k] }
    // is the set of states, with inv_delta_set[k] = -1 and inv_delta_set[j] >= 0 for t <= j < k  
    int *inv_delta[n];
    for(i=0;i<n;i++) inv_delta[i]=allocate_int_array(CSIZE);
    int *inv_delta_set=allocate_int_array(2*n*CSIZE); 

    // twin stores two things: 
    // twin[0]..twin[numSplit-1] is the list of blocks that have been split
    // twin[B_i] is the twin of block B_i
    int twin[2*n];
    int numSplit;

    // SD[B_i] is the the number of states s in B_i with delta(s,a) in B_j
    // if SD[B_i] == block[B_i], there is no need to split
    int SD[2*n]; // [only SD[n..2*n-1] is used]


    // for fixed (B_j,a), the D[0]..D[numD-1] are the inv_delta(B_j,a)
    int D[n];
    int numD;    

    // initialize inverse of transition state_table
    int lastDelta = 0;
    int inv_lists[n]; // holds a set of lists of states
    int inv_list_last[n]; // the last element
        
    int c,s;
    
    for (s=0;s<n;s++)
    	inv_lists[s]=-1;
    
    for (c = 0; c < CSIZE; c++) {
      // clear "head" and "last element" pointers
      for (s = 0; s < n; s++) {
        inv_list_last[s] = -1;
        inv_delta[s][c] = -1;
      }
      
      // the error state has a transition for each character into itself
      inv_delta[0][c] = 0;
      inv_list_last[0] = 0;

      // accumulate states of inverse delta into lists (inv_delta serves as head of list)
      for (s = 1; s < n; s++) {
        int t = state_table[s-1][c]+1; //@Michela: check this "+1"

        if (inv_list_last[t] == -1) { // if there are no elements in the list yet
          inv_delta[t][c] = s;  // mark t as first and last element
          inv_list_last[t] = s;
        }
        else {
          inv_lists[inv_list_last[t]] = s; // link t into chain
          inv_list_last[t] = s; // and mark as last element
        }
      }

      // now move them to inv_delta_set in sequential order, 
      // and update inv_delta accordingly
      for (int s = 0; s < n; s++) {
        int i = inv_delta[s][c];  inv_delta[s][c] = lastDelta;
        int j = inv_list_last[s];
        bool go_on = (i != -1);
        while (go_on) {
          go_on = (i != j);
          inv_delta_set[lastDelta++] = i;
          i = inv_lists[i];
        }
        inv_delta_set[lastDelta++] = -1;
      }
    } // of initialize inv_delta

   
    // initialize blocks 
    
    // make b0 = {0}  where 0 = the additional error state
    b_forward[b0]  = 0;
    b_backward[b0] = 0;          
    b_forward[0]   = b0;
    b_backward[0]  = b0;
    block[0]  = b0;
    block[b0] = 1;

    for (int s = 1; s < n; s++) {
      //fprintf(stdout,"Checking state [%d]\n",(s-1));
      // search the blocks if it fits in somewhere
      // (fit in = same pushback behavior, same finalness, same lookahead behavior, same action)
      int b = b0+1; // no state can be equivalent to the error state
      bool found = false;
      while (!found && b <= lastBlock) {
        // get some state out of the current block
        int t = b_forward[b];
        //fprintf(stdout,"  picking state [%d]\n",(t-1));

        // check, if s could be equivalent with t
        found = true;// (isPushback[s-1] == isPushback[t-1]) && (isLookEnd[s-1] == isLookEnd[t-1]);
        if (found) {
          found = accepted_rules[s-1]->equal(accepted_rules[t-1]); 	
        
          if (found) { // found -> add state s to block b
           // fprintf(stdout,"Found! [%d,%d] Adding to block %d\n",s-1,t-1,(b-b0));
            // update block information
            block[s] = b;
            block[b]++;
            
            // chain in the new element
            int last = b_backward[b];
            b_forward[last] = s;
            b_forward[s] = b;
            b_backward[b] = s;
            b_backward[s] = last;
          }
        }

        b++;
      }
      
      if (!found) { // fits in nowhere -> create new block
        //fprintf(stdout,"not found, lastBlock = %d\n",lastBlock);

        // update block information
        block[s] = b;
        block[b]++;
        
        // chain in the new element
        b_forward[b] = s;
        b_forward[s] = b;
        b_backward[b] = s;
        b_backward[s] = b;
        
        lastBlock++;
      }
    } // of initialize blocks
  
    //printBlocks(block,b_forward,b_backward,lastBlock);

    // initialize worklist L
    // first, find the largest block B_max, then, all other (B_i,c) go into the list
    int B_max = b0;
    int B_i;
    for (B_i = b0+1; B_i <= lastBlock; B_i++)
      if (block[B_max] < block[B_i]) B_max = B_i;
    
    // L = empty
    l_forward[anchorL] = anchorL;
    l_backward[anchorL] = anchorL;

    // set up the first list element
    if (B_max == b0) B_i = b0+1; else B_i = b0; // there must be at least two blocks    

    int index = (B_i-b0)*CSIZE;  // (B_i, 0)
    while (index < (B_i+1-b0)*CSIZE) {
      int last = l_backward[anchorL];
      l_forward[last]     = index;
      l_forward[index]    = anchorL;
      l_backward[index]   = last;
      l_backward[anchorL] = index;
      index++;
    }

    // now do the rest of L
    while (B_i <= lastBlock) {
      if (B_i != B_max) {
        index = (B_i-b0)*CSIZE;
        while (index < (B_i+1-b0)*CSIZE) {
          int last = l_backward[anchorL];
          l_forward[last]     = index;
          l_forward[index]    = anchorL;
          l_backward[index]   = last;
          l_backward[anchorL] = index;
          index++;
        }
      }
      B_i++;
    } 
    // end of setup L
    
    // start of "real" algorithm
    
    // while L not empty
    while (l_forward[anchorL] != anchorL) {
     
      // pick and delete (B_j, a) in L:

      // pick
      int B_j_a = l_forward[anchorL];      
      // delete 
      l_forward[anchorL] = l_forward[B_j_a];
      l_backward[l_forward[anchorL]] = anchorL;
      l_forward[B_j_a] = 0;
      // take B_j_a = (B_j-b0)*CSIZE+c apart into (B_j, a)
      int B_j = b0 + B_j_a / CSIZE;
      int a   = B_j_a % CSIZE;
      // determine splittings of all blocks wrt (B_j, a)
      // i.e. D = inv_delta(B_j,a)
      numD = 0;
      int s = b_forward[B_j];
      while (s != B_j) {
        // fprintf(stdout,"splitting wrt. state %d \n",s);
        int t = inv_delta[s][a];
        // fprintf(stdout,"inv_delta chunk %d\n",t);
        while (inv_delta_set[t] != -1) {
          //fprintf(stdout,"D+= state %d\n",inv_delta_set[t]);
          D[numD++] = inv_delta_set[t++];
        }
        s = b_forward[s];
      }      

      // clear the twin list
      numSplit = 0;
    
      // clear SD and twins (only those B_i that occur in D)
      for (int indexD = 0; indexD < numD; indexD++) { // for each s in D
        s = D[indexD];
        B_i = block[s];
        SD[B_i] = -1; 
        twin[B_i] = 0;
      }
      
      // count how many states of each B_i occuring in D go with a into B_j
      // Actually we only check, if *all* t in B_i go with a into B_j.
      // In this case SD[B_i] == block[B_i] will hold.
      for (int indexD = 0; indexD < numD; indexD++) { // for each s in D
        s = D[indexD];
        B_i = block[s];

        // only count, if we haven't checked this block already
        if (SD[B_i] < 0) {
          SD[B_i] = 0;
          int t = b_forward[B_i];
          while (t != B_i && (t != 0 || block[0] == B_j) && 
                 (t == 0 || block[state_table[t-1][a]+1] == B_j)) {
            SD[B_i]++;
            t = b_forward[t];
          }
        }
      }
  
      // split each block according to D      
      for (int indexD = 0; indexD < numD; indexD++) { // for each s in D
        s = D[indexD];
        B_i = block[s];
        
        if (SD[B_i] != block[B_i]) {
          int B_k = twin[B_i];
          if (B_k == 0) { 
            // no twin for B_i yet -> generate new block B_k, make it B_i's twin            
            B_k = ++lastBlock;
            b_forward[B_k] = B_k;
            b_backward[B_k] = B_k;
            
            twin[B_i] = B_k;

            // mark B_i as split
            twin[numSplit++] = B_i;
          }
          // move s from B_i to B_k
          
          // remove s from B_i
          b_forward[b_backward[s]] = b_forward[s];
          b_backward[b_forward[s]] = b_backward[s];

          // add s to B_k
          int last = b_backward[B_k];
          b_forward[last] = s;
          b_forward[s] = B_k;
          b_backward[s] = last;
          b_backward[B_k] = s;

          block[s] = B_k;
          block[B_k]++;
          block[B_i]--;

          SD[B_i]--;  // there is now one state less in B_i that goes with a into B_j
          // fprintf(stdout,"finished move\n");
        }
      } // of block splitting

      // update L
      for (int indexTwin = 0; indexTwin < numSplit; indexTwin++) {
        B_i = twin[indexTwin];
        int B_k = twin[B_i];
        for (int c = 0; c < CSIZE; c++) {
          int B_i_c = (B_i-b0)*CSIZE+c;
          int B_k_c = (B_k-b0)*CSIZE+c;
          if (l_forward[B_i_c] > 0) {
            // (B_i,c) already in L --> put (B_k,c) in L
            int last = l_backward[anchorL];
            l_backward[anchorL] = B_k_c;
            l_forward[last] = B_k_c;
            l_backward[B_k_c] = last;
            l_forward[B_k_c] = anchorL;
          }
          else {
            // put the smaller block in L
            if (block[B_i] <= block[B_k]) {
              int last = l_backward[anchorL];
              l_backward[anchorL] = B_i_c;
              l_forward[last] = B_i_c;
              l_backward[B_i_c] = last;
              l_forward[B_i_c] = anchorL;              
            }
            else {
              int last = l_backward[anchorL];
              l_backward[anchorL] = B_k_c;
              l_forward[last] = B_k_c;
              l_backward[B_k_c] = last;
              l_forward[B_k_c] = anchorL;              
            }
          }
        }
      }
    }

       // transform the transition state_table 
    
    // trans[i] is the state j that will replace state i, i.e. 
    // states i and j are equivalent
    int trans [_size];
    
    // kill[i] is true iff state i is redundant and can be removed
    bool kill[_size];
    
    // move[i] is the amount line i has to be moved in the transition state_table
    // (because states j < i have been removed)
    int move [_size];
    
    // fill arrays trans[] and kill[] (in O(n))
    for (int b = b0+1; b <= lastBlock; b++) { // b0 contains the error state
      // get the state with smallest value in current block
      int s = b_forward[b];
      int min_s = s; // there are no empty blocks!
      for (; s != b; s = b_forward[s]) 
        if (min_s > s) min_s = s;
      // now fill trans[] and kill[] for this block 
      // (and translate states back to partial DFA)
      min_s--; 
      for (s = b_forward[b]-1; s != b-1; s = b_forward[s+1]-1) {
        trans[s] = min_s;
        kill[s] = s != min_s;
      }
    }
    
    // fill array move[] (in O(n))
    int amount = 0;
    for (int i = 0; i < _size; i++) {
      if ( kill[i] ) 
        amount++;
      else
        move[i] = amount;
    }

    int j;
    // j is the index in the new transition state_table
    // the transition state_table is transformed in place (in O(c n))
    for (i = 0, j = 0; i < _size; i++) {
      
      // we only copy lines that have not been removed
      if ( !kill[i] ) {
        
        // translate the target states
        int c; 
        for (c = 0; c < CSIZE; c++) {
          if ( state_table[i][c] >= 0 ) {
            state_table[j][c] = trans[ state_table[i][c] ];
            state_table[j][c]-= move[ state_table[j][c] ];
          }
          else {
            state_table[j][c] = state_table[i][c];
          }
        }
        
		if (i!=j) {
			accepted_rules[j]->clear();
        	accepted_rules[j]->add(accepted_rules[i]);
		}
        
        j++;
      }
    }
    
    _size = j;
    
    //free arrays
    free(l_forward);
    free(l_backward);
    free(inv_delta_set);
    for(int i=0;i<n;i++) free(inv_delta[i]);
    
    //free unused memory in the DFA
    for (state_t s=_size;s<entry_allocated;s++){
    	if (state_table[s]!=NULL){
    		free(state_table[s]);
    		state_table[s]=NULL;
    	}
    	if (accepted_rules[s]!=NULL){
    		delete accepted_rules[s];
    		accepted_rules[s]=NULL;
    	}
    }
    state_table=reallocate_state_matrix(state_table,_size);
    accepted_rules=(linked_set **)reallocate_array(accepted_rules,_size,sizeof(linked_set*));
    marking=reallocate_int_array(marking,_size);	
    if (default_tx!=NULL) {free(default_tx); default_tx=NULL;}
    entry_allocated=_size;

	if (VERBOSE) fprintf(stdout,"DFA:: minimize: after minimization states = %ld\n",_size);
   
 }

void DFA::to_dot(FILE *file, const char *title){
	set_depth();
	fprintf(file, "digraph \"%s\" {\n", title);
	for (state_t s=0;s<_size;s++){
		if (accepted_rules[s]->empty()){ 
			fprintf(file, " %ld [shape=circle,label=\"%ld",s,s);
			fprintf(file,"-%ld\"];\n",depth[s]);
			//fprintf(file, " %ld [shape=circle];\n", s);
		}else{ 
			fprintf(file, " %ld [shape=doublecircle,label=\"%ld",s,s);
			fprintf(file, "-%ld/",depth[s]);
			linked_set *ls=	accepted_rules[s];
			while(ls!=NULL){
				if(ls->succ()==NULL)
					fprintf(file,"%ld",ls->value());
				else
					fprintf(file,"%ld,",ls->value());
				ls=ls->succ();
			}
			fprintf(file,"\"];\n");
		}
	}
	int *mark=allocate_int_array(CSIZE);
	char *label=NULL;
	//char *temp=(char *)malloc(1);
	char *temp=(char *)malloc(100);//changed to the same value in Becchi's code
	state_t target=NO_STATE;
	for (state_t s=0;s<_size;s++){
		for(int i=0;i<CSIZE;i++) mark[i]=0;
		for (int c=0;c<CSIZE;c++){
			if (!mark[c]){
				mark[c]=1;
				if (state_table[s][c]!=0){
					target=state_table[s][c];
					//fprintf(stdout,"state_table[%ld][%d]=%ld\n",s,c,target);
					label=(char *)malloc(100);
					if ((c>='a' && c<='z') || (c>='A' && c<='Z')) sprintf(label,"%c",c);
					else sprintf(label,"%d",c);
					bool range=true;
					int begin_range=c;
					for(int d=c+1;d<CSIZE;d++){
						if (state_table[s][d]==target){
							mark[d]=1;
							if (!range){
								if ((d>='a' && d<='z') || (d>='A' && d<='Z')) sprintf(temp,"%c",d);
								else sprintf(temp,"%d",d);
								label=strcat(label,",");
								label=strcat(label,temp);
								begin_range=d;
								range=1;
							}
						}
						if (range && (state_table[s][d]!=target || d==CSIZE-1)){
							range=false;
							if(begin_range!=d-1){
								if (state_table[s][d]!=target)
									if ((d-1>='a' && d-1<='z') || (d-1>='A' && d-1<='Z')) sprintf(temp,"%c",d-1);
									else sprintf(temp,"%d",d-1);
								else
									if ((d>='a' && d<='z') || (d>='A' && d<='Z')) sprintf(temp,"%c",d);
									else sprintf(temp,"%d",d);
								label=strcat(label,"-");
								label=strcat(label,temp);
							}
						}	
					}	
				}
			}
			if (label!=NULL) {
				fprintf(file, "%ld -> %ld [label=\"%s\"];\n", s,target,label);	
				free(label); 
				label=NULL;
			}
		}
		if (default_tx!=NULL) fprintf(file, "%ld -> %ld [color=\"limegreen\"];\n", s,default_tx[s]);
	}
	free(temp);
	free(mark);
	fprintf(file,"}");
}

void DFA::to_file(FILE *file, const char *title) {
	fprintf(file, "# %s\n%u\n", title, size());

	set_depth();

	if(_size)
		fprintf(file, "0 : initial\n");

	// Print the accepting states and the corresponding rules
	for (state_t s=0;s<_size;s++){
		if (!accepted_rules[s]->empty()){ 
			fprintf(file, "%ld : accepting ", s);
			linked_set *ls=	accepted_rules[s];
			while(ls!=NULL){
				if(ls->succ()==NULL)
					fprintf(file,"%ld",ls->value());
				else
					fprintf(file,"%ld ",ls->value());
				ls=ls->succ();
			}
			fprintf(file,"\n");
		}
	}
	int *mark=allocate_int_array(CSIZE);
	char *label=NULL;
	char *temp=(char *)malloc(1000);
	state_t target=NO_STATE;
	for (state_t s=0;s<_size;s++){
		for(int i=0;i<CSIZE;i++) mark[i]=0;
		for (int c=0;c<CSIZE;c++){
			if (!mark[c]){
				mark[c]=1;
				//if (state_table[s][c]!=0){//ORIG code: ignore default transitions to state 0, however with the iNFAnt code it is very important for correct results. 
					target=state_table[s][c];
					//fprintf(stdout,"state_table[%ld][%d]=%ld\n",s,c,target);
					label=(char *)malloc(10000);
					sprintf(label,"%d",c);
					bool range=true;
					int begin_range=c;
					for(int d=c+1;d<CSIZE;d++){
						if (state_table[s][d]==target){
							mark[d]=1;
							if (!range){
								sprintf(temp,"%d",d);
								label=strcat(label," ");
								label=strcat(label,temp);
								begin_range=d;
								range=1;
							}
						}
						if (range && (state_table[s][d]!=target || d==CSIZE-1)){
							range=false;
							if(begin_range!=d-1){
								if (state_table[s][d]!=target)
									sprintf(temp,"%d",d-1);
								else
									sprintf(temp,"%d",d);
								label=strcat(label,"|");
								label=strcat(label,temp);
							}
						}	
					}	
				//}//ORIG code
			}
			if (label!=NULL) {
				fprintf(file, "%ld -> %ld : %s\n", s,target,label);	
				free(label); 
				label=NULL;
			}
		}
		if (default_tx!=NULL) fprintf(file, "%ld -> %ld : default;\n", s,default_tx[s]);
	}
	free(temp);
	free(mark);
	fprintf(file,"\n");
}

/*Dump the dfa into a file it can later be read from.*/
void DFA::put(FILE *file, char *comment){\
	
	if (default_tx==NULL) {
		fprintf(stderr,"Compressing DFA\n");
		fast_compression_algorithm();
		fprintf(stderr,"Compression done\n");
	}
	
	
	//comment
	if (comment!=NULL) fprintf(file,"# %s\n",comment);
	else fprintf(file,"# DFA dump \n");
	//number of states
	fprintf(file, "%ld\n", _size);
	if (_size==0) return;
	//for each state, failure pointer, labeled transitions char-state, accepting states)
	for (state_t s=0;s<_size;s++){
		fprintf(file, "( ");
		fprintf(file,"%ld",default_tx[s]);
		fprintf(file, " ( ");
		FOREACH_TXLIST(labeled_tx[s],it) fprintf(file,"%d %ld ",(*it).first,(*it).second);
		fprintf(file, ")");	
		linked_set *ls=accepted_rules[s];
		while(ls!=NULL && !ls->empty()){fprintf(file," %ld",ls->value());ls=ls->succ();}
		fprintf(file, " )\n");	
	}
}

/*Read the dfa from file.*/
void DFA::get(FILE *file){
	long posn;
	rewind(file);
	//resetting the DFA
	for (int i=0;i<_size;i++) {
		free(state_table[i]); 
		delete accepted_rules[i];
		if (labeled_tx!=NULL) delete labeled_tx[i];
	}
	free(state_table);
	if (labeled_tx!=NULL) free(labeled_tx);
	free(accepted_rules);
	free(marking);
	if (depth!=NULL) {free(depth);depth=NULL;}
	if(default_tx!=NULL) {free(default_tx); default_tx=NULL;}
	//reading from file
	/* comment */
	char c=getc(file);
	if (c=='#'){
		while (c!='\n') c=getc(file);
	}else{
		rewind(file);
	}
	/* number of states */
	int res=fscanf(file,"%d\n",&_size);
	if (res==EOF || res<=0) {
		fatal("DFA:: get: _size not read");
	}
	//printf("%ld states\n",_size);
	if(_size==0) return;
	entry_allocated=_size;
	marking=allocate_int_array(entry_allocated); //marking
	for (state_t s=0;s<_size;s++) marking[s]=0; 
	default_tx=allocate_state_array(_size); //default transitions
	labeled_tx=(tx_list **)allocate_array(_size,sizeof(tx_list *)); //labeled transitions
	state_table=allocate_state_matrix(_size); // state table
	accepted_rules=(linked_set **)allocate_array(_size,sizeof(linked_set *)); //accept state
	/* per state information */
	for (state_t s=0;s<_size;s++){
		//printf("reading state %ld... \n",s);
		symbol_t c; state_t ns;
		labeled_tx[s]=new tx_list();
		state_table[s]=allocate_state_array(CSIZE);
		accepted_rules[s]=new linked_set();
		res=fscanf(file,"( %d ( ",&(default_tx[s]));
		if (res==EOF || res<=0) fatal("DFA:: get: error in reading failure pointer");
		posn=ftell(file);
		res=fscanf(file,"%d %d ",&c,&ns);
		while(res!=EOF && res>0){
			//printf("c %d ns %ld\n",c,ns);
			labeled_tx[s]->push_back(tx_t(c,ns));
			posn=ftell(file);
			res=fscanf(file,"%d %d ",&c,&ns);
		}
		fseek(file,posn,SEEK_SET);
		posn=ftell(file);
		res = getc(file); if (res!=')') fatal("DFA:: get: invalid format");
		res = getc(file); if (res!=' ') fatal("DFA:: get: invalid format");
		posn=ftell(file);
		res=fscanf(file,"%d ",&c);
		while(res!=EOF && res>0){
			//printf("c %d \n",c);
			accepted_rules[s]->insert(c);
			if (c>NFA::num_rules) NFA::num_rules=c;
			posn=ftell(file);
			res=fscanf(file,"%d ",&c);
		}
		fseek(file,posn,SEEK_SET);
		res = getc(file); if (res!=')') fatal("DFA:: get: invalid format");
		res = getc(file); if (res!='\n') fatal("DFA:: get: invalid format");
	}	
	//reconstructing state table
	for (state_t s=0;s<_size;s++)
		for (int c=0;c<CSIZE;c++)
			state_table[s][c]=lookup(s,c);		
}

unsigned DFA::alphabet_reduction(symbol_t *alph_tx, bool init){
	symbol_t max_class=0;
	for (symbol_t c=0;c<CSIZE;c++){
		if (init) alph_tx[c]=0;
		else if (alph_tx[c]>max_class) max_class=alph_tx[c];
	}
	for (state_t s=0;s<_size;s++){
		for (state_t t=0;t<_size;t++){
			bool char_covered[CSIZE];
			bool class_covered[CSIZE];
			symbol_t remap[CSIZE];
			for (symbol_t c=0;c<CSIZE;c++){
				char_covered[c]=0;
				class_covered[c]=0;
				remap[c]=0;
			}
			for (symbol_t c=0;c<CSIZE;c++){
				if(state_table[s][c]==t){
					char_covered[c]=1;
					class_covered[alph_tx[c]]=1;
				}
			}
			for (symbol_t c=0;c<CSIZE;c++){
				if( !char_covered[c] && class_covered[alph_tx[c]] ){
					if(remap[alph_tx[c]]==0) remap[alph_tx[c]]=++max_class;
				 	alph_tx[c]=remap[alph_tx[c]];
				}
			}
		} //end loop state t
	} //end loop state s 
	return (max_class+1);
}

void DFA::analyze_classes(symbol_t *classes, unsigned num_classes){
	bool *class_covered=allocate_bool_array(num_classes);
	unsigned int *entry_state=allocate_uint_array(_size);
	unsigned int *entry_class=allocate_uint_array(_size);
	int max_eclass=0;
	int max_estate=0;
	for(state_t s=0;s<_size;s++){
		entry_state[s]=0;entry_class[s]=0;
		for(int i=0;i<num_classes;i++) class_covered[i]=false;
		for(state_t t=0;t<_size;t++){
			bool found=false;
			for (int c=0;c<CSIZE;c++){
				if(state_table[t][c]==s){
					if(!found){entry_state[s]++;found=true;}
					if(!class_covered[classes[c]]){class_covered[classes[c]]=true;entry_class[s]++;}
				}
			}
		}
		printf("state %ld: entry_states=%ld, entry_classes=%ld\n",s,entry_state[s],entry_class[s]);
		if (entry_class[s]>max_eclass) max_eclass=entry_class[s];
		if (entry_state[s]>max_estate) max_estate=entry_state[s];
	}
	printf("\n");
	int num_states;
	for (int i=max_estate;i>0;i--){
		num_states=0;
		for (state_t s=0;s<_size;s++){
			if(entry_state[s]==i) num_states++;
		}
		if (num_states!=0) printf("entry states:%ld - states=%ld %f%%\n",i,num_states,(float)num_states*100/_size);
	}
	printf("\n");
	for (int i=max_eclass;i>0;i--){
		num_states=0;
		for (state_t s=0;s<_size;s++){
			if(entry_class[s]==i) num_states++;
		}
		if (num_states!=0) printf("entry classes:%ld - states=%ld %f%%\n",i,num_states,(float)num_states*100/_size);
	}
	free(class_covered);
	free(entry_state);
	free(entry_class);
}


void DFA::set_depth(){
	std::list<state_t> *queue=new std::list<state_t>();
	if (depth==NULL) depth = allocate_uint_array(_size);
	for (state_t s=1;s<_size;s++) depth[s]=0xFFFFFFFF;
	depth[0]=0;
	queue->push_back(0);
	while(!queue->empty()){
		state_t s=queue->front(); queue->pop_front();
		for (int c=0;c<CSIZE;c++){
			if (depth[state_table[s][c]]==0xFFFFFFFF){
				depth[state_table[s][c]]=depth[s]+1;
				queue->push_back(state_table[s][c]);
			}
		}
	}
	delete queue;
}

state_t DFA::lookup(state_t s, symbol_t c){
	state_t next=NO_STATE;
	FOREACH_TXLIST(labeled_tx[s],it){
		if ((*it).first==c){
			next=(*it).second;
			break;
		}
	}
	if (next!=NO_STATE)
		return next;
	else
		return lookup(default_tx[s],c); 	
}

state_t DFA::lookup(state_t s, symbol_t c, unsigned **state_traversals, unsigned *total_traversals){
	(*total_traversals)++;
	(*state_traversals)[s]++;
	state_t next = NO_STATE;
	FOREACH_TXLIST(labeled_tx[s],it){
		if ((*it).first==c){
			next=(*it).second;
			break;
		}
	}
	if (next!=NO_STATE)
		return next;
	else{
		return lookup(default_tx[s],c,state_traversals,total_traversals);
	} 	
}

int DFA::traversals_per_lookup(state_t s,symbol_t c){
	state_t next=NO_STATE;
	FOREACH_TXLIST(labeled_tx[s],it){
		if ((*it).first==c){
			next=(*it).second;
			break;
		}
	}
	if (next!=NO_STATE)
		return 0;
	else
		return 1+traversals_per_lookup(default_tx[s],c);
}

void DFA::crosscheck_default_tx(){
	if (labeled_tx==NULL) fatal("default/labeled transitions not yet computed");
	for (state_t s=0;s<_size;s++)
		for (int c=0;c<CSIZE;c++)
			if(state_table[s][c]!=lookup(s,c)) printf("error:: (%ld,%d)\n",s,c);
	printf("crosscheck ok\n");		
}

void DFA::analyze_default_tx(short *classes, short num_classes, int offset,FILE *file){
	if (depth==NULL) set_depth();
	printf("\n> Depth analysis...\n");
	int max_d=0;
	for(state_t s=0;s<_size;s++) if (depth[s]>max_d) max_d=depth[s];
	printf("- max depth=%d\n",max_d);
	state_t *state_d = new state_t[max_d+1];
	state_t *target_d = new state_t[max_d+1];
	state_t *dt_d = new state_t[max_d+1]; 
	for (int d=0;d<=max_d;d++) {state_d[d]=target_d[d]=dt_d[d]=0;}
	for(state_t s=0;s<_size;s++){
		state_d[depth[s]]++;
		for (int c=0;c<CSIZE;c++) target_d[depth[state_table[s][c]]]++;
		dt_d[depth[default_tx[s]]]++;
	}
	//for (int d=0;d<=max_d;d++) printf("depth %ld - states=%ld;%f %%, targets=%ld;%f %%, dt=%ld;%f %%\n",d,state_d[d],(float)100*state_d[d]/_size,target_d[d],(float)100*target_d[d]/(_size*CSIZE),dt_d[d],(float)100*dt_d[d]/_size);
	for (int d=0;d<=max_d;d++) printf("depth %ld - states=%ld, targets=%ld, dt=%ld\n",d,state_d[d],target_d[d],dt_d[d]);
	delete [] state_d; 
	delete [] target_d;
	delete [] dt_d;
	
	printf("\n> Default transitions analysis...\n");
	//number of default transitions
	linked_set *dt=new linked_set();
	for(state_t s=0;s<_size;s++)
		dt->insert(default_tx[s]);
	printf("- number of distinct default_tx targets: %ld\n",dt->size());	
	delete dt;
	//path length
	printf("- failure_pointer path length analysis...\n");
	dheap *heap=new dheap(_size);
	int length;
	unsigned int min_l=NO_STATE;
	int max_l=0;
	unsigned long tot_length=0;
	for (state_t s=0;s<_size;s++){
		for (int c=0;c<CSIZE;c++){
			length=traversals_per_lookup(s,c);
			if(heap->member(length))
				heap->changekey(length,heap->key(length)+1);
			else
				heap->insert(length,1);
		}		
	}
	while(!heap->empty()){
		int length=heap->deletemin();
		int states =heap->key(length);
		tot_length+=length*states;
		if (length>max_l) max_l=length;
		if (length<min_l) min_l=length;
		printf("length=%ld, path=%ld\n",length,states);
	}
	printf("path length: min=%ld, max=%ld, average=%f\n",min_l,max_l,(float)tot_length/(_size*CSIZE));
	delete heap;
	
	printf("\n> Analysis of transitions...\n");
	printf("-char based ...\n");
	heap=new dheap(CSIZE);
	unsigned long reduced=0;
	for (state_t s=0;s<_size;s++){
		int non_null=labeled_tx[s]->size();
		if(heap->member(non_null))
			heap->changekey(non_null,heap->key(non_null)+1);
		else
			heap->insert(non_null,1);		
	}
	while(!heap->empty()){
		int non_null=heap->deletemin();
		int states =heap->key(non_null);
		reduced+=non_null*states;
		printf("transitions=%ld, states=%ld\n",non_null,states);
	}
	unsigned long distinct=0;
	linked_set *set=new linked_set();
	for (int c=0;c<CSIZE;c++){
		for(state_t s=0;s<_size;s++)
			set->insert(state_table[s][c]);
		distinct+=set->size();
		set->clear();	
	}
	delete set;
	delete heap;
	if (offset==-1) fprintf(file,"%d %d %ld %ld %f ",_size, num_classes, _size*CSIZE, distinct,
						        (float)100-distinct*100/(float)(_size*CSIZE));
	fprintf(file,"%f %d %ld %f ",(float)tot_length/(_size*CSIZE),max_l,reduced,(float)100-reduced*100/(float)(_size*CSIZE));   	
	printf("transitions: total=%ld, distinct=%ld, reduced=%ld\n",_size*CSIZE,distinct,reduced);
	printf("percentage: distinct=%f %%, reduced=%f %%\n",(float)100-distinct*100/(float)(_size*CSIZE),(float)100-reduced*100/(float)(_size*CSIZE));
	
	printf("-class based ...\n");
    heap=new dheap(CSIZE);
    unsigned long tot_class_tr=0;
	int *class_covered=allocate_int_array(num_classes);
	for (state_t s=0;s<_size;s++){
		int class_tr=0;
		for(int i=0;i<num_classes;i++) class_covered[i]=false;
		FOREACH_TXLIST(labeled_tx[s],it){
			symbol_t c=(*it).first;
			if(!class_covered[classes[c]]){
				class_covered[classes[c]]=true;
				class_tr++;
			}
		}
		if(heap->member(class_tr))
			heap->changekey(class_tr,heap->key(class_tr)+1);
		else
			heap->insert(class_tr,1);		
	}
	while(!heap->empty()){
		int class_tr=heap->deletemin();
		int states =heap->key(class_tr);
		tot_class_tr+=(class_tr*states);
		printf("transitions=%ld, states=%ld\n",class_tr,states);
	}
	printf("class transitions %ld, % reductions: %f %%\n ", tot_class_tr, (float)100-tot_class_tr*100/(float)(_size*CSIZE));
	fprintf(file,"%ld %f ",tot_class_tr, (float)100-tot_class_tr*100/(float)(_size*CSIZE));
	
	delete heap;
	free(class_covered);
	fflush(stdout);
}

void DFA::fast_compression_algorithm(int k,int bound){
	struct timeval start,end;
	gettimeofday(&start,NULL);
	set_depth();
	//allocate and init default_tx
	if (default_tx!=NULL) free(default_tx);
	default_tx = allocate_state_array(_size);
	default_tx[0]=0;
	//allocate and init reduced state_table
	if (labeled_tx!=NULL){
		for (state_t t=0;t<_size;t++) delete labeled_tx[t];
		free(labeled_tx);
	}
	labeled_tx=(tx_list **)allocate_array(_size,sizeof (tx_list *));
	for (state_t t=0;t<_size;t++) labeled_tx[t]=new tx_list();
	for(int c=0;c<CSIZE;c++) labeled_tx[0]->push_back(tx_t(c,state_table[0][c]));
	int *dt_length = new int[_size];
	dt_length[0]=0;
	
	//compute default transitions and reduced state array
	for (state_t s=1;s<_size;s++){
		state_t dt=NO_STATE;
		int max_common=0; 
		for(state_t t=0;t<_size;t++){
			if ((bound==-1 || dt_length[t]<bound) && (signed)depth[s]-k>=(signed)depth[t]){
				int common=0;
				for (int c=0;c<CSIZE;c++) if (state_table[s][c]==state_table[t][c]) common++;
				if (dt==NO_STATE || common>max_common || (common==max_common  && dt_length[t]<dt_length[dt])){
					dt=t;
					max_common=common;
				}
			}
		}
		if (max_common==1){
			default_tx[s]=NO_STATE;
			dt=NO_STATE;
			dt_length[s]=0;
		}else{
			default_tx[s]=dt;
			dt_length[s]=dt_length[dt]+1;
		}
		for(int c=0;c<CSIZE;c++){
			if (dt==NO_STATE || state_table[s][c]!=state_table[dt][c]) labeled_tx[s]->push_back(tx_t(c,state_table[s][c]));
		}
	}
	delete [] dt_length;
	gettimeofday(&end,NULL);
	if (VERBOSE) printf("Running_time=%ld sec. %ld msec.\n",end.tv_sec-start.tv_sec,(end.tv_usec-start.tv_usec)/1000);
}

inline int top(int x, int y){
	return ((x%y==0) ? x/y : x/y+1);
}

inline int get_weight(wgraph *G, int *distance, edge e){
	return (G->w(e)*1000-top(distance[G->left(e)]+distance[G->right(e)]+1,2));
}

void update_distance(wgraph *G, wgraph *T, dheap *heap, int *distance, vertex u, edge in, int d_v,bool refined) {
	//update distance of u
	if (distance[u] < d_v+1) {
		distance[u] = d_v+1;
		if (refined){
			for(edge e=G->first(u);e!=Null;e=G->next(u,e)){
				if (heap->member(e)){
					heap->changekey(e,-get_weight(G,distance,e));
				}
			}
		} 
	}
	//update distance of all neighbors of u
	for (edge e = T->first(u); e != Null; e = T->next(u,e)) {
		if (e!=in){
			vertex w = T->mate(u,e);
			update_distance(G, T, heap, distance,w,e,d_v+1,refined);
		}
	}
} 

//modified Kruskal's algorithm for D2FA generation
void kruskal(wgraph *G, wgraph *T, partition *P, int *distance, int diameter, bool refined){
	vertex u,v,cu,cv; edge e;
	int d_u,d_v;
    dheap *heap=new dheap(G->m,3);
	for(e=1;e<=G->m;e++){
		if (refined) heap->insert(e,-get_weight(G,distance,e));
		else heap->insert(e,-G->w(e));
	}
	while (!heap->empty()){
		e=heap->deletemin();
		u=G->left(e); v=G->right(e); 
		cu=P->find(u);cv=P->find(v);
		if (cu!=cv){
			d_u=distance[u]; 
			d_v=distance[v];
			if (d_u+d_v+1<=diameter){
				update_distance(G,T,heap,distance,u,e,d_v,refined);
				update_distance(G,T,heap,distance,v,e,d_u,refined);
				P->link(cu,cv);
				T->join(u,v,G->w(e));
			}
		}
	}
	delete heap;
}

// generates default and labeled transitions for a D2FA 
void DFA::d2fa_graph_to_default_tx(wgraph *T,partition *P,int *distance){
	//init default_tx and labeled_tx
	if (default_tx==NULL) default_tx=allocate_state_array(_size);
	for (state_t s=0;s<_size;s++) default_tx[s]=NO_STATE;
	if (labeled_tx==NULL){
		labeled_tx=(tx_list **)allocate_array(_size,sizeof(tx_list*));
		for (state_t s=0;s<_size;s++) labeled_tx[s]=new tx_list();
	}
	int *marking=new int[_size];
	for (state_t s=0;s<_size;s++) marking[s]=0;
	for (state_t s=0;s<_size;s++){
		state_t cs=P->find(s);
		state_t root;
		if (!marking[cs]){
			//printf("processing %ld\n",cs);
			marking[cs]=1;
			//find the root of the tree (nodes with min distance from other nodes)
			std::list<state_t> *queue=new std::list<state_t>();
			queue->push_back(cs);
			root=cs;
			while(!queue->empty()){
				state_t u=queue->front(); queue->pop_front();
				marking[u]=1;
				if (distance[u]<distance[root]) root=u;
				for (edge e=T->first(u);e!=Null;e=T->next(u,e)){
					if (T->w(e)>0){
						vertex v=T->mate(u,e);
						if (!marking[v]) queue->push_back(v);
					}
				}
			}
			
			//build default transitions to root
			default_tx[root]=root;
			for (int c=0;c<CSIZE;c++) labeled_tx[root]->push_back(tx_t(c,state_table[root][c]));
			//process other states in same tree
			for (edge e=T->first(root);e!=Null;e=T->next(root,e)){
				if (T->w(e)>0){
					state_t u = T->mate(root,e);
					default_tx[u]=root;
					for (int c=0;c<CSIZE;c++){
						if (state_table[root][c]!=state_table[u][c]) labeled_tx[u]->push_back(tx_t(c,state_table[u][c]));
					}
					queue->push_back(u);
				}
			}
			while(!queue->empty()){
				state_t v=queue->front(); queue->pop_front();
				for (edge e=T->first(v);e!=Null;e=T->next(v,e)){
					if (T->w(e)>0){
						state_t u = T->mate(v,e);
						if (u!=default_tx[v]){
							default_tx[u]=v;
							for (int c=0;c<CSIZE;c++){
								if (state_table[v][c]!=state_table[u][c]) labeled_tx[u]->push_back(tx_t(c,state_table[u][c]));
							}
							queue->push_back(u);
						}
					}
				}	
			}
			delete queue;
		}
	}
	delete [] marking;
}

void DFA::D2FA(int diameter, bool refined){
	struct timeval start,end;
	gettimeofday(&start,NULL);
	wgraph *T=new wgraph(_size,_size-1); //undirected forest of tree of failure ptrs;
	wgraph *G; 							 //space reduction graph
	partition *P=new partition(_size);   //partition data structure used by Kruskal algorithm
	int *distance=new int[_size];		 //distance vector
	for (state_t s=0;s<_size;s++) distance[s]=0;
	int DECREMENT=32;  /* IMPORTANT: This variable has been introduced to deal with big DFAs which
						             would lead to space reduction graphs not fitting memory. You may 
						             increase its value to CSIZE (a single pass) for small DFAs, and decrease it
						             to 1 for big DFAs-DFAs leading to dense space reduction graphs.
						             Multiple scans do affect the running time of the compression algorithm! */	
	for (int max=CSIZE; max>0; max=max-DECREMENT){
		G=new wgraph(_size,30000000);
		for (state_t s=0;s<_size-1;s++){
			for (state_t t=s+1;t<_size;t++){
				int common=0;
				for (int c=0;c<CSIZE;c++) if (state_table[s][c]==state_table[t][c]) common++;
				if (common<=max && common>max-DECREMENT) G->join(s,t,common);
			}
		}
		kruskal(G,T,P,distance,diameter,refined); //modified Kruskal's algorithm
		delete G;
	}
	d2fa_graph_to_default_tx(T,P,distance);
	gettimeofday(&end,NULL);
	if (VERBOSE) printf("Running_time = %ld sec. %ld msec.\n",end.tv_sec-start.tv_sec,(end.tv_usec-start.tv_usec)/1000);
	delete [] distance;
	delete T;
	delete P;
}

vertex tree_root(wgraph *T, vertex v, int *distance, unsigned *in_degree){
	vertex root=v;
	std::list<state_t> *queue=new std::list<state_t>();
	int *marked=new int[T->n]; for (state_t s=0;s<T->n;s++) marked[s]=0;
	
	queue->push_back(v);
	while(!queue->empty()){
		state_t u=queue->front(); queue->pop_front();
		marked[u]=1;
		if (distance[u]<distance[root] || (distance[u]==distance[root] && in_degree[u]>in_degree[root])) root=u;
		for (edge e=T->first(u);e!=Null;e=T->next(u,e)){
			if(T->w(e)>0){
				vertex v=T->mate(u,e);
				if (!marked[v]) queue->push_back(v);
			}
		}
	}
	delete queue; 
	delete [] marked;
	return root;
}

unsigned tree_w(wgraph *T, state_t state){
	unsigned treew=0;
	std::list<state_t> *queue=new std::list<state_t>();
	int *marked=new int[T->n]; for (state_t s=0;s<T->n;s++) marked[s]=0;
	
	queue->push_back(state);
	marked[state]=1;
	while(!queue->empty()){
		state_t u=queue->front(); queue->pop_front();
		for (edge e=T->first(u);e!=Null;e=T->next(u,e)){
			if (T->w(e)>0){
				vertex v=T->mate(u,e);
				if (!marked[v]) {treew+=T->w(e);queue->push_back(v); marked[v]=1;}
			}
		}
	}
	delete queue; 
	delete [] marked;
	return treew;
}
		
//creation phase for C2DFA algorithm		
void creation_phase(wgraph *G, wgraph *T, partition *P, int *distance, unsigned *in_degree, unsigned max_in_degree){
	vertex u,v,cu,cv; edge e;
	int d_u,d_v;
    dheap *heap=new dheap(G->m,3);
	for(e=1;e<=G->m;e++){
		heap->insert(e,-(G->w(e)*(max_in_degree*2+1)+in_degree[G->left(e)]+in_degree[G->right(e)]));
	}
	while (!heap->empty()){
		e=heap->deletemin();
		u=G->left(e); v=G->right(e); 
		cu=P->find(u);cv=P->find(v);
		if (cu!=cv){
			d_u=distance[u]; 
			d_v=distance[v];
			if (d_u+d_v+1<=2){
				update_distance(G,T,heap,distance,u,e,d_v,false);
				update_distance(G,T,heap,distance,v,e,d_u,false);
				P->link(cu,cv);
				T->join(u,v,G->w(e));
			}
		}
	}
	delete heap;
}

//reduction phase for C2DFA algorithm
unsigned reduction_phase(wgraph *G, wgraph *T, partition *P, int *distance, unsigned *in_degree){
	state_t *root=new state_t[G->n];
	for (state_t s=0;s<G->n;s++) root[s]=NO_STATE;
	for (state_t s=0;s<G->n;s++){
		state_t cs=P->find(s);
		if (root[cs]==NO_STATE) {root[cs]= tree_root(T,cs,distance,in_degree);}
	}
	//put in the heap the subtrees (characteristic elements) and their weight
	dheap *heap=new dheap(G->n,2);
	for (state_t s=0;s<G->n;s++){
		state_t cs=P->find(s);
		if (!heap->member(cs)){
			heap->insert(cs,tree_w(T,cs));
		}
	}
	while (!heap->empty()){
		//get the canonical element of the tree with smallest total weight
		state_t cs=heap->deletemin();
		if (heap->empty()) break;
		int old_w=heap->key(cs);
		int new_w=0;
		//build list of nodes in tree to be dissolved
		linked_set *tree_nodes=new linked_set();
		linked_set *old_edges=new linked_set();
		linked_set *new_edges=new linked_set();
		std::list<state_t> *queue=new std::list<state_t>();
		queue->push_back(cs);
		while (!queue->empty()){
			state_t u=queue->front(); queue->pop_front();
			tree_nodes->insert(u);
			for (edge e=T->first(u);e!=Null;e=T->next(u,e)){
				if (T->w(e)>0){
					vertex v=T->mate(u,e);
					if (!tree_nodes->member(v)) {queue->push_back(v); old_edges->insert(e);}
				}
			}	
		}
		delete queue;
		//try to connect to other root nodes
		linked_set *ls=tree_nodes;
		while (ls!=NULL && ls->value()!=_INVALID){
			state_t s=ls->value();
			int w=0;
			edge new_e;
			for (state_t t=0;t<G->n;t++){ //loop around roots
				if (root[t]!=NO_STATE && root[t]!=root[cs]){
					for (edge e=G->first(s);e!=Null;e=G->next(s,e)){
						if (G->mate(s,e)==root[t] && G->w(e)>w){
							new_e=e;w=G->w(e);
						}
					}
				}
			}
			if (w>0) {new_edges->insert(new_e); new_w+=w; }
			ls=ls->succ();
		}
		//printf("printf old_weight:%ld - new weight:%ld\n",old_w,new_w);
		// in case the tree has really to be dissolved
		if (new_w>old_w){
			//printf ("reducing\n");
			//delete the old edges from the tree by setting their weight to 0
			root[cs]=NO_STATE;
			ls=old_edges;
			while(ls!=NULL && ls->value()!=_INVALID){
			   T->changeWt(ls->value(),0);
			   ls=ls->succ();
			}
			// add the new edges to the tree, delete them from the previous partition
			// add them to the new partition, update the value of the modified tree weight
			ls=new_edges;
			while(ls!=NULL && ls->value()!=_INVALID){
			   edge e= ls->value();
			   state_t x = (tree_nodes->member(G->left(e)))? G->left(e) : G->right(e);
			   state_t y = G->mate(x,e);
			   //printf("new edge %ld-%ld, %ld\n",x,y,e);
			   P->reset(x);
			   P->link(x,y);
			   distance[x]=distance[y]+1;
			   T->join(x,y,G->w(e));
			   state_t cy=P->find(y);
			   if (heap->member(cy))
			      heap->changekey(cy,tree_w(T,y));
			   else	
			   	  heap->insert(cy,tree_w(T,y));
			   ls=ls->succ();
			} 
		}
		delete tree_nodes;
		delete old_edges;
		delete new_edges;
	}//while on heap
	unsigned trees=0;
	for (state_t s=0;s<G->n;s++) if (root[s]!=NO_STATE) trees++;
	delete heap;
	delete [] root;
	return trees;
}

//optimization phase for C2DFA algorithm
unsigned optimization_phase(wgraph *G, wgraph *T, partition *P, int *distance, unsigned *in_degree){
	linked_set *root=new linked_set();
	linked_set *ls=new linked_set();
	int *bound=new int[T->n];
	edge *edges=new edge[T->n];
	for (state_t s=0;s<G->n;s++){
		state_t cs=P->find(s);
		if (!ls->member(cs)) {root->insert(tree_root(T,cs,distance,in_degree));}
		ls->insert(cs);
	}
	delete ls;
	dheap *heap=new dheap(G->n,2);
	for (state_t s=0;s<T->n;s++){
		if(root->member(s)) bound[s]=256;
		else {
			edge e=T->first(s);
			while (T->w(e)==0) e=T->next(s,e);
			bound[s]=256-T->w(e);
			edges[s]=e;
			heap->insert(s,bound[s]);
		}
	}
	while(!heap->empty()){
		state_t s=heap->deletemin();
		for (edge e=G->first(s);e!=Null;e=G->next(s,e)){
			if (e!=edges[s]){
				state_t u=G->mate(s,e);
				if (!root->member(u) && 256-G->w(e)+bound[s]<bound[u]){
					bound[u]=256-G->w(e)+bound[s];
					T->changeWt(edges[u],0);
					edges[u]=e;
					T->join(s,u,G->w(e));
					P->reset(u);
					P->link(u,s);
					distance[u]=distance[s]+1;
					if (heap->member(u))
						heap->changekey(u,bound[u]);
					else
						heap->insert(u,bound[u]);
				}
			}
		}	
	}
	
	delete root;
	delete heap;
	delete [] bound;
	delete [] edges;
}

unsigned DFA::CD2FA(){
	struct timeval start,end;
	gettimeofday(&start,NULL);
	wgraph *T=new wgraph(_size,1000000); //undirected forest of tree of failure ptrs;
	wgraph *G; //support graph
	partition *P=new partition(_size); //partition data structure used by Kruskal algorithm
	int *distance=new int[_size];
	unsigned *in_degree=new unsigned[_size];
	unsigned max_in_degree=0;
	for (state_t s=0;s<_size;s++) {distance[s]=0;in_degree[s]=0;}
	for (state_t s=0;s<_size;s++){
		for (int c=0;c<CSIZE; c++) in_degree[state_table[s][c]]++;
	}
	for (state_t s=0;s<_size;s++) {if (in_degree[s]>max_in_degree) max_in_degree=in_degree[s];}
	
	G=new wgraph(_size,30000000);
	for (state_t s=0;s<_size-1;s++){
		for (state_t t=s+1;t<_size;t++){
			int common=0;
			for (int c=0;c<CSIZE;c++) if (state_table[s][c]==state_table[t][c]) common++;
			if (common>251) G->join(s,t,common);
		}
	}
	
	//printf("\nCREATION PHASE...\n");
	creation_phase(G,T,P,distance,in_degree,max_in_degree);
	d2fa_graph_to_default_tx(T,P,distance);
	
	//character classes construction
	symbol_t *classes=new symbol_t[CSIZE];
	symbol_t num_classes=alphabet_reduction(classes);
	
	//printf("\nREDUCTION PHASE...\n");
	unsigned trees=reduction_phase(G,T,P,distance,in_degree);
	d2fa_graph_to_default_tx(T,P,distance);
	
	//printf("\nOPTIMIZATION PHASE...\n");
	optimization_phase(G,T,P,distance,in_degree);
	d2fa_graph_to_default_tx(T,P,distance);

	gettimeofday(&end,NULL);
	if (VERBOSE) printf("Running_time = %ld sec. %ld msec.\n",end.tv_sec-start.tv_sec,(end.tv_usec-start.tv_usec)/1000);

	delete [] classes;
	delete [] distance;
	delete [] in_degree;
	delete G;
	delete T;
	delete P;
	
	return trees;
}
