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
 * File:   memory.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 */

#include "memory.h"
#include <list>
#include <sys/time.h>

#define MAX_ACTIVE_SET 100

using namespace std;

memory::memory(){
	num_dfas=0;
	dfas=NULL;
	nfa=NULL;
	hfa=NULL;
	mapping=NULL;
	stable=NULL;
	fa=NULL;
	size=NULL;
	alphabet_tx=NULL;
}

memory::memory(compDFA **in_dfas, unsigned in_num_dfas){
	type=DFA_T;
	num_dfas=in_num_dfas;
	dfas=in_dfas;
	nfa=NULL;
	hfa=NULL;
	mapping=NULL;
	stable=NULL;
	
	state_address = (unsigned **)allocate_array(sizeof(unsigned *),num_dfas);
	fa            = (unsigned **)allocate_array(sizeof(unsigned *),num_dfas);
	size          			= new unsigned[num_dfas];
	num_full_states 		= new unsigned[num_dfas];
	num_compressed_states	= new unsigned[num_dfas];
	num_compressed_tx		= new unsigned[num_dfas];
	for (int i=0;i<num_dfas;i++){
		state_address[i]=new unsigned[dfas[i]->size];
		size[i]=0;
		num_full_states[i]=0;
		num_compressed_states[i]=0;
		num_compressed_tx[i]=0;	
	}
	alphabet=dfas[0]->alphabet_size; 
	alphabet_tx=new symbol_t[CSIZE];
	for (symbol_t c=0;c<CSIZE;c++)alphabet_tx[c]=dfas[0]->alphabet_tx[c];
	
	compute_dfa_memory_layout();
	compute_dfa_memory_map();
}

memory::memory(compNFA* comp_nfa){
	type=NFA_T;
	nfa=comp_nfa;
	num_dfas=1;
	dfas=NULL;
	hfa=NULL;
	mapping=NULL;
	
	alphabet=nfa->alphabet_size;
	alphabet_tx=new symbol_t[CSIZE];
	for (symbol_t c=0;c<CSIZE;c++)alphabet_tx[c]=nfa->alphabet_tx[c];
	
	state_address = (unsigned **)allocate_array(sizeof(unsigned *),1);
	fa            = (unsigned **)allocate_array(sizeof(unsigned *),1);
	size          = new unsigned[1];
	num_full_states 		= NULL;
	num_compressed_states	= NULL;
	num_compressed_tx		= NULL;
	state_address[0]=new unsigned[nfa->size];
	
	stable=new bool[nfa->size];
	for (state_t s=0;s<nfa->size;s++) stable[s]=false;
	
	compute_nfa_memory_layout();
	compute_nfa_memory_map();
}

memory::memory(HybridFA *hfa_in){
	type=DFA_T;
	nfa=NULL;
	hfa=hfa_in;
	stable=NULL;
	
	mapping = new map <NFA *, unsigned>(); 
	
	nfa_set *border_states=new nfa_set();
	for(border_it it=hfa->get_border()->begin();it!=hfa->get_border()->end();it++)
		border_states->insert(((*it).second)->begin(),((*it).second)->end());
	
	num_dfas=1+border_states->size(); //head+tails
	
	DFA *sub_dfas[num_dfas];
	
	unsigned i=0;
	sub_dfas[i]=hfa->get_head();
	if (DEBUG) printf("memory:: HFA: head - size=%d\n",hfa->get_head()->size());
	FOREACH_SET(border_states,it){
		(*mapping)[*it]=++i;
		if (DEBUG) printf("memory:: generating tail-DFA[%d] for NFA-state %d (w/ depth=%d,size=%d)...\n",i,(*it)->get_id(),(*it)->get_depth(),(*it)->size());
		sub_dfas[i]=(*it)->nfa2dfa();
		if (sub_dfas[i]==NULL){
			printf("memory:: could not create tail-DFA[%d] for NFA-state %d (w/ size=%d)\n",i,(*it)->get_id(),(*it)->size());
			fatal("memory:: please modify the hybrid-FA generation settings!");
		}else{
			if (sub_dfas[i]->size()<100000) sub_dfas[i]->minimize();
			if (DEBUG) printf("memory:: HFA: tail[%d] for NFA-state %d (w/ depth=%d,size=%d) - size=%d\n",i,(*it)->get_id(),(*it)->get_depth(),(*it)->size(),sub_dfas[i]->size());
		}
		fflush(stdout);
	}
	
	alphabet=0;
	alphabet_tx=new symbol_t[CSIZE];
	for (symbol_t c=0;c<CSIZE;c++) alphabet_tx[c]=0;
	for (int i=0;i<num_dfas;i++){
		if (DEBUG){
			printf("memory:: computing alphabet size on DFA #%d of size %d\n",i,sub_dfas[i]->size());
			fflush(stdout);
		}
		alphabet = sub_dfas[i]->alphabet_reduction(alphabet_tx,false);
	}
	if (VERBOSE) {printf("memory:: alphabet size of Hybrid-FA %d\n",alphabet); fflush(stdout);}
	
	dfas=(compDFA**)allocate_array(sizeof(compDFA*),num_dfas);
	for(int i=0;i<num_dfas;i++){
		if (DEBUG){
			printf("memory:: computing the compressed-DFA for DFA #%d of size %d\n",i,sub_dfas[i]->size());
			fflush(stdout);
		}
		dfas[i]=new compDFA(sub_dfas[i],alphabet,alphabet_tx);
		if (i!=0) delete sub_dfas[i];
	}
	delete border_states;
	
	state_address = (unsigned **)allocate_array(sizeof(unsigned *),num_dfas);
	for (int i=0;i<num_dfas;i++) state_address[i]=new unsigned[dfas[i]->size];
	fa = (unsigned **)allocate_array(sizeof(unsigned *),num_dfas);
	
	num_full_states		  = new unsigned[num_dfas];
	num_compressed_states = new unsigned[num_dfas];
	num_compressed_tx     = new unsigned[num_dfas];
	size                  = new unsigned[num_dfas];
	
	for (int i=0;i<num_dfas;i++){
		num_full_states[i]=0;
		num_compressed_states[i]=0;
		num_compressed_tx[i]=0;
		size[i]=0;
	}
	
	compute_hfa_memory_layout();
	compute_hfa_memory_map();
}

memory::~memory(){
	if (fa!=NULL){
		for (int i=0;i<num_dfas;i++) delete [] fa[i];
		free(fa);
	}
	if (mapping!=NULL) delete mapping;
	if (size!=NULL) delete [] size;
	if (alphabet_tx!=NULL) delete [] alphabet_tx;
}

void memory::print_layout(){
	for (int i=0;i<num_dfas;i++){
		printf("DFA #%d,states=%d: size=%d, full_states=%d, compressed_states=%d, compressed_tx=%d\n",
				i,dfas[i]->size,size[i],num_full_states[i],num_compressed_states[i],num_compressed_tx[i]);
	}
}

/* DFA */
void memory::compute_dfa_memory_layout(){
		
	/* state analysis - size and index */
	list <state_t> *queue=new list<state_t>();
	
	//EXTERNAL LOOP: for each DFA
	for (int i=0;i<num_dfas;i++){
		//initialize addresses
		unsigned base_index=0;
	 
		//get DFA
		compDFA *dfa=dfas[i];
		
		//LOOP: compute state addresses 
		dfa->mark(0);
		queue->push_back(0);
		if (dfa->labeled_tx[0]->size()!=dfa->alphabet_size) fatal("state 0 is not fully specified!");
			
		while(!queue->empty()){
			state_t s=queue->front(); queue->pop_front();
			sorted_tx_list *tx=dfa->labeled_tx[s];
			/* FULLY SPECIFIED STATE */
			if (tx->size()>TX_THRESHOLD){
				num_full_states[i]++;
				state_address[i][s]=base_index;
				base_index+=dfa->alphabet_size;
			
		    /* COMPRESSED STATE */	
			}else{
				num_compressed_states[i]++;
				num_compressed_tx[i]+=(tx->size()+1);
				state_address[i][s]=base_index;
				base_index+=(tx->size()+1);
			}
			//enqueue the connected states (breath-first traversal)
			while (tx!=NULL && !tx->empty()){
				state_t t=tx->state();
				if (!dfa->marking[t]){
					dfa->mark(t);
					queue->push_back(t);
				}
				tx=tx->next();
			}
		}//end LOOP
		size[i]=base_index;
		
		dfa->reset_marking();
		
		
	}//end EXTERNAL LOOP (for each DFA)
	
	if (VERBOSE) print_layout();
	
	delete [] num_full_states;
	delete [] num_compressed_states;
	delete [] num_compressed_tx;
	
	delete queue;
}

void memory::compute_dfa_memory_map(){
	printf("computing memory map...\n");
	unsigned tot_size=0;
	
	//DFA data
	for (int i=0;i<num_dfas;i++){
		
		unsigned word;
		fa[i] = new unsigned[size[i]];
		if (VERBOSE) printf("allocated %d B/%d KB for DFA #%d\n",size[i]*sizeof(unsigned),size[i]*sizeof(unsigned)/1024,i);
		tot_size+=size[i]*sizeof(unsigned);
		
		for(state_t s=0;s<dfas[i]->size;s++){
			unsigned address=state_address[i][s];
			sorted_tx_list *tx=dfas[i]->labeled_tx[s];
			
			/* state is fully specified*/
			if (tx->size()>TX_THRESHOLD){
				for (int c=0;c<dfas[i]->alphabet_size;c++){
					state_t t=dfas[i]->next_state(s,c);
					word = state_address[i][t] & 0x1FFFFF;
					if (!dfas[i]->accepted_rules[t]->empty()) word = word | 0x80000000;
					if ((dfas[i]->labeled_tx[t])->size() <= TX_THRESHOLD) word = word | 0x40000000;
					fa[i][address]=word;
					address++;
				}
				
			/* state is compressed */		
			}else{
				//inserting the default transition first
				word = state_address[i][dfas[i]->default_tx[s]] & 0x1FFFFF;
				//no match on default transition
				if ((dfas[i]->labeled_tx[dfas[i]->default_tx[s]])->size() <= TX_THRESHOLD) word = word | 0x40000000;
				if (tx->empty()) word = word | 0x20000000;
				fa[i][address]=word;
				address++;
				//if any, insert all the labeled transitions
				while (tx!=NULL && !tx->empty()){
					word = 0;
					if (!dfas[i]->accepted_rules[tx->state()]->empty()) word = word | 0x80000000; //match marker
					if ((dfas[i]->labeled_tx[tx->state()])->size()<= TX_THRESHOLD) word = word | 0x40000000;
					if (tx->next()==NULL) word = word | 0x20000000; //end of state marker
					word = word | ((tx->c() & 0xFF) << 21); //symbol				
					word = word | ( state_address[i][tx->state()] & 0x1FFFFF); //address
					fa[i][address]=word;
					address++;
					tx=tx->next();
				}
			}
		}
	}
	if (VERBOSE) printf("DFAs: allocated %d B/%d KB\n",tot_size, tot_size/1024);
	
	for (int i=0;i<num_dfas;i++) delete [] state_address[i];
	free(state_address);
}

/* NFA */

void memory::compute_nfa_memory_layout(){

	/* state analysis */
	list <state_t> *queue=new list<state_t>();
	
	//LOOP: compute state addresses 
	nfa->mark(0);
	queue->push_back(0);
	size[0]=0;

	unsigned base_addr=0;
	
	while(!queue->empty()){
		state_t s=queue->front(); queue->pop_front();
		sorted_tx_list *tx=nfa->tx[s];
		
		//check if the state is stable
		unsigned self_tx=0;
		FOREACH_SORTEDLIST(tx,sl) if (sl->state()==s) self_tx++;
		if (self_tx>=nfa->alphabet_size) {
			stable[s]=true;
			printf("%ld is stable\n",s);
		}
		else self_tx=0;
		
		state_address[0][s]=base_addr;
		if ((tx->size()-self_tx) == 0){
			base_addr++;
			size[0]++;
		}
		else{
			base_addr+=(tx->size()-self_tx);
			size[0]+=(tx->size()-self_tx);
		}
		
		//enqueue the connected states (breath-first traversal)
		while (tx!=NULL && !tx->empty()){
			state_t t=tx->state();
			if (!nfa->marking[t]){
				nfa->mark(t);
				queue->push_back(t);
			}
			tx=tx->next();
		}
	}//end LOOP
	
	if (VERBOSE) printf("allocated %d B/%d KB for the NFA\n",size[0]*sizeof(unsigned),size[0]*sizeof(unsigned)/1024);
	
	delete queue;
}

void memory::compute_nfa_memory_map(){
	printf("computing memory map...\n");
	int index=0;
	unsigned word;
	fa[0]=new unsigned[size[0]];
	
	/* print state information */	
	for(state_t s=0;s<nfa->size;s++){
		index=state_address[0][s];
		sorted_tx_list *tx_list=nfa->tx[s];
		if (stable[s]){
			tx_list = new sorted_tx_list();
			FOREACH_SORTEDLIST(nfa->tx[s],tx) if (tx->state()!=s) tx_list->insert(tx->c(),tx->state());
		}
		if (tx_list->empty()) fa[0][index]=0xFFFFFFFF;
		FOREACH_SORTEDLIST(tx_list,tx){ 
			word = 0;
			if (!nfa->accepted_rules[tx->state()]->empty()) word = word | 0x80000000; //accepting state marker
			if (stable[tx->state()]) word = word | 0x40000000; 						  //stable market
			if (tx->next()==NULL) word = word | 0x20000000; 						  //end of state marker
			word = word | ((tx->c() & 0x000000FF) << 21); //symbol
			word = word | state_address[0][tx->state()] & 0x1FFFFF;
			fa[0][index++]=word;
		}
		if (stable[s]) delete tx_list;	
	}
	
	delete [] state_address[0];
	delete [] stable;
	free(state_address);
	
}

/* Hybrid-FA */

void memory::compute_hfa_memory_layout(){

	//border
	map <state_t, nfa_set*> *border=hfa->get_border();
	
	/* state analysis - size and index */
	list <state_t> *queue=new list<state_t>();
	
	//EXTERNAL LOOP: for each DFA
	for (int i=0;i<num_dfas;i++){
		//initialize addresses
		unsigned base_index=0;
	 
		//get DFA
		compDFA *dfa=dfas[i];
		
		//LOOP: compute state addresses 
		dfa->mark(0);
		queue->push_back(0);
		if (dfa->labeled_tx[0]->size()!=dfa->alphabet_size) fatal("state 0 is not fully specified!");
			
		while(!queue->empty()){
			state_t s=queue->front(); queue->pop_front();
			sorted_tx_list *tx=dfa->labeled_tx[s];
			/* FULLY SPECIFIED STATE */
			if (tx->size()>TX_THRESHOLD){
				num_full_states[i]++;
				state_address[i][s]=base_index;
				base_index+=dfa->alphabet_size;
			
		    /* COMPRESSED STATE */	
			}else{
				num_compressed_states[i]++;
				num_compressed_tx[i]+=(tx->size()+1);
				state_address[i][s]=base_index;
				base_index+=(tx->size()+1);
			}
			if (i==0 && (*border)[s]!=NULL) base_index+=(*border)[s]->size();
			
			//enqueue the connected states (breath-first traversal)
			while (tx!=NULL && !tx->empty()){
				state_t t=tx->state();
				if (!dfa->marking[t]){
					dfa->mark(t);
					queue->push_back(t);
				}
				tx=tx->next();
			}
		}//end LOOP
		size[i]=base_index;
		
		dfa->reset_marking();
		
		
	}//end EXTERNAL LOOP (for each DFA)
	
	if (VERBOSE) print_layout();
	
	delete [] num_full_states;
	delete [] num_compressed_states;
	delete [] num_compressed_tx;
	
	delete queue;
	
}

void memory::compute_hfa_memory_map(){
	
	printf("computing memory map...\n");
	
	//border
	map <state_t, nfa_set*> *border=hfa->get_border();
		
	//memory size
	unsigned tot_size=0;
	
	//DFA data
	for (int i=0;i<num_dfas;i++){
		
		unsigned word;
		fa[i] = new unsigned[size[i]];
		if (VERBOSE) printf("allocated %d B/%d KB for DFA #%d\n",size[i]*sizeof(unsigned),size[i]*sizeof(unsigned)/1024,i);
		tot_size+=size[i]*sizeof(unsigned);
		
		for(state_t s=0;s<dfas[i]->size;s++){
			unsigned address=state_address[i][s];
			sorted_tx_list *tx=dfas[i]->labeled_tx[s];
			
			if (i==0){
				nfa_set *bs=(*border)[s];
				if (bs!=NULL){
					int b=0;
					FOREACH_SET(bs,it){
						word = (*mapping)[*it];
						if ((++b)==bs->size()) word=word | 0x80000000;
						fa[i][address++]=word;
					}
				}
			}
			
			/* state is fully specified*/
			if (tx->size()>TX_THRESHOLD){
				for (int c=0;c<dfas[i]->alphabet_size;c++){
					state_t t=dfas[i]->next_state(s,c);
					word = state_address[i][t] & 0xFFFFF;
					if (!dfas[i]->accepted_rules[t]->empty()) word = word | 0x80000000;
					if ((dfas[i]->labeled_tx[t])->size() <= TX_THRESHOLD) word = word | 0x40000000;
					if (i==0 && (*border)[t]!=NULL) word = word | 0x10000000;
					if (i!=0 && t==dfas[i]->get_dead_state()) word = word | 0x10000000;
					fa[i][address++]=word;
				}
				
			/* state is compressed */		
			}else{
				//inserting the default transition first
				state_t t = dfas[i]->default_tx[s];
				if (i!=0 || (*border)[t]==NULL) word = state_address[i][t] & 0xFFFFF;
				else word = (state_address[i][t] & 0xFFFFF) + ((*border)[t])->size();
				//no match on default transition
				if ((dfas[i]->labeled_tx[t])->size() <= TX_THRESHOLD) word = word | 0x40000000;
				if (tx->empty()) word = word | 0x20000000;
				if (i!=0 && dfas[i]->default_tx[s]==dfas[i]->get_dead_state()) word = word | 0x10000000;
				fa[i][address++]=word;
				//if any, insert all the labeled transitions
				while (tx!=NULL && !tx->empty()){
					word = 0;
					if (!dfas[i]->accepted_rules[tx->state()]->empty()) word = word | 0x80000000; //match marker
					if ((dfas[i]->labeled_tx[tx->state()])->size()<= TX_THRESHOLD) word = word | 0x40000000;
					if (tx->next()==NULL) word = word | 0x20000000; //end of state marker
					if (i==0 && (*border)[tx->state()]!=NULL) word = word | 0x10000000;
					if (i!=0 && tx->state()==dfas[i]->get_dead_state()) word = word | 0x10000000;
					word = word | ((tx->c() & 0xFF) << 20); //symbol				
					word = word | ( state_address[i][tx->state()] & 0xFFFFF); //address
					fa[i][address++]=word;
					tx=tx->next();
				}
			}
		}
	}
	if (VERBOSE) printf("DFAs: allocated %d B/%d KB\n",tot_size, tot_size/1024);
	
	for (int i=0;i<num_dfas;i++){
		delete dfas[i];
		delete [] state_address[i];
	}
	free(state_address);
	free(dfas);
}

void memory::put(FILE *stream){
	fprintf(stream,"%d\n",alphabet);
	for (int i=0;i<CSIZE;i++) fprintf(stream,"%d ",alphabet_tx[i]); fprintf(stream,"\n");
	if (type==DFA_T) fprintf(stream,"d\n");
	else  fprintf(stream,"n\n");
	if (type==DFA_T){
		fprintf(stream,"%d\n",num_dfas);
		for (int i=0;i<num_dfas;i++) fprintf(stream,"%d\n",size[i]);
		for (int i=0;i<num_dfas;i++){
				for (unsigned j=0;j<size[i];j++) fprintf(stream,"0x%x\n",fa[i][j]);
		}
	}
	else{
		fprintf(stream,"%d\n",size[0]);
		for (unsigned i=0;i<size[0];i++) fprintf(stream,"0x%x\n",fa[0][i]);
	}
}

void memory::get(FILE *stream){
	unsigned m_size=0;
	char c=0;
	int r = fscanf(stream,"%d\n",&alphabet);
	if (r==0 && r==EOF) fatal("memory:: get: error while reading alphabet");
	alphabet_tx = new symbol_t[CSIZE];
	for (int i=0;i<CSIZE;i++) fscanf(stream,"%d ",&alphabet_tx[i]); fscanf(stream,"\n");
	r = fscanf(stream,"%c\n",&c);
	if (r==0 && r==EOF) fatal("memory");
	if (c=='d'){
		type=DFA_T;
		fscanf(stream,"%d\n",&num_dfas);
		fa = (unsigned **)allocate_array(sizeof(unsigned *),num_dfas);
		size = new unsigned[num_dfas];
		for (int i=0;i<num_dfas;i++){
			fscanf(stream,"%d\n",&size[i]);
			fa[i] = new unsigned[size[i]];
		}
		for (int i=0;i<num_dfas;i++){
				for (unsigned j=0;j<size[i];j++) fscanf(stream,"0x%x\n",&fa[i][j]);
				printf("DFA #%d, size=%d B,%d KB\n",i,sizeof(unsigned)*size[i],sizeof(unsigned)*size[i]/1024);
				m_size+=sizeof(unsigned)*size[i];
		}
		printf("total size=%d B,%d KB\n",m_size,m_size/1024);
	}
	else{
		type=NFA_T;
		num_dfas=1; //only for memory delete purposes
		size=new unsigned[1];
		fscanf(stream,"%d\n",&size[0]);
		fa = (unsigned **)allocate_array(sizeof(unsigned *),1);
		fa[0] = new unsigned[size[0]];
		for (unsigned i=0;i<size[0];i++) fscanf(stream,"0x%x\n",&fa[0][i]);
		printf("NFA size=%d B,%d KB\n",sizeof(unsigned)*size[0],sizeof(unsigned)*size[0]/1024);
	}
	printf("memory:: get() performed\n");
}

void memory::traverse_dfa(unsigned trace_size, char *data, FILE *log){

	struct timeval start,end;
	
	
	unsigned matches=0, r_time;
	unsigned state[num_dfas];
	bool compressed[num_dfas];
	unsigned s,ds,j;

#ifdef STATISTICS
	unsigned tot_tx=0;
	unsigned tot_default_taken=0;
	unsigned tot_full_traversed=0;
	unsigned tot_compressed_traversed=0;
	unsigned tot_compressed_taken=0;
#endif	

	long inputs=0;
	char payload[2000];
	int payload_size,pi,pj,pk,p_size;
	int64_t cost, copycost;
	double variance;
	double total_time=0;
	double total_bytes=0;
	


	for (int i=0;i<num_dfas;i++) {state[i]=0; compressed[i]=0;}

	
	for(pk=0;pk<MAX_PKT;pk++){
		get_payload(payload, &payload_size);
		total_bytes+=payload_size;
		
		gettimeofday(&start,NULL);
               	//Reset
               	for (int i=0;i<num_dfas;i++) {state[i]=0; compressed[i]=0;}
		data=payload;
		trace_size=payload_size;
	
	
		for (unsigned ic=0;ic<trace_size;ic++){
			unsigned char c=data[ic];
			for (int i=0;i<num_dfas;i++){
				//if (DEBUG) printf("trace:: DFA #%d, char=%d/%c/%d, state=0x%x, compressed=%d\n",i,c,c,alphabet_tx[c],state[i],compressed[i]);	
				while(true){
					if (compressed[i]){ // compressed state
						ds = fa[i][state[i]];
#ifdef STATISTICS
						tot_tx++;
						tot_compressed_traversed++;
						tot_compressed_taken++;
#endif					
						if (ds & 0x20000000){						
							state[i] = ds & 0x1FFFFF;
							compressed[i] = ds & 0x40000000;
#ifdef STATISTICS
							tot_default_taken++;
#endif				
						}else{
							j = 1;
							do{
								s = fa[i][state[i]+j];
#ifdef STATISTIC
								tot_tx++;
								tot_compressed_taken++;
#endif					
								j++;
							}while((s & 0x20000000)==0 && (((s>>21) & 0xFF)!=alphabet_tx[c]));
							if (((s>>21) & 0xFF)==alphabet_tx[c]){
								state[i] = s & 0x1FFFFF;
								compressed[i] = s & 0x40000000;
								if (s & 0x80000000) matches++;
								break;
							}else{
								state[i] = ds & 0x1FFFFF;
								compressed[i] = ds & 0x40000000;
#ifdef STATISTIC
								tot_default_taken++;
#endif					
							}
						}
					}else{ 				// non compressed state
						s = fa[i][state[i]+alphabet_tx[c]];
#ifdef STATISTICS
						tot_tx++;
						tot_full_traversed++;
#endif										
						state[i] = s & 0x1FFFFF;
						compressed[i] = s & 0x40000000;
						if (s & 0x80000000) matches++;
						break;
					}
				}
			}
		} //and for c
		
		gettimeofday(&end,NULL);
		r_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
		total_time += r_time;
		
		if(!(pk%1000))
			printf("Current Throughput %0.6g Bps <> %0.6g bps\r", total_bytes*1000000/total_time, total_bytes*8000000/total_time);
		
	}
	
 
	printf("\n");
	
	printf("\n\nFinal Throughput %0.6g Bps <> %0.6g bps\r", total_bytes*1000000/total_time, total_bytes*8000000/total_time);
	//printf("Running_time=%ld sec. %ld usec.\n",r_time/1000000,r_time%1000000);
	//printf("%d matches detected.\n",matches);
	//fprintf(log,"DFA %ld\n",r_time);

#ifdef STATISTICS
	fprintf(log,"TOT_TX %ld\n",tot_tx);
	fprintf(log,"TOT_DEFAULT_TAKEN %ld\n",tot_default_taken);
	fprintf(log,"TOT_COMPRESSED_TAKEN %ld\n",tot_compressed_taken);
	fprintf(log,"TOT_COMPRESSED_TRAVERSED %ld\n",tot_compressed_traversed);
	fprintf(log,"TOT_FULL_TRAVERSED %ld\n",tot_full_traversed);
#endif										
	fflush(log);
}

#ifdef NFA_STATIC

void memory::traverse_nfa(unsigned trace_size, char *data, FILE *log){
	
	struct timeval start,end;
	
	
	unsigned matches=0,r_time;
	unsigned active_set[2][MAX_ACTIVE_SET];
	unsigned active_set_size[2]={1,0};
	bool active=0;
	active_set[0][0]=0x40000000; //state 0 is stable

#ifdef STATISTICS
	unsigned avg_active_set=0;
	unsigned max_active_set=0;
	unsigned tot_tx=0;
#endif
	
	unsigned idx,s;
	
	long inputs=0;
	char payload[2000];
	int payload_size,pi,pj,pk,p_size;
	int64_t cost, copycost;
	double variance, total_bytes=0, total_time=0;
	

	matches=0;
	//active_set_size[2][0]=1;
	//active_set_size[2][1]=0;
	active=0;
	active_set[0][0]=0x40000000; //state 0 is stable
		

	
	for(pk=0;pk<MAX_PKT;pk++){
		get_payload(payload, &payload_size);
		total_bytes+=payload_size;
		
		gettimeofday(&start,NULL);
		

               	//Reset
		matches=0;
		//active_set_size[2][0]=1;
		//active_set_size[2][1]=0;
		bool active=0;
		active_set[0][0]=0x40000000; //state 0 is stable
        			
        	data=payload;
		trace_size=payload_size;
	
		for (unsigned ic=0;ic<trace_size;ic++){
			unsigned char c=data[ic];
#ifdef STATISTICS
			avg_active_set+=active_set_size[active];
			if (active_set_size[active]>max_active_set) max_active_set=active_set_size[active];
#endif	
			for (unsigned i=0;i<active_set_size[active];i++){
				//stable
				if (active_set[active][i] & 0x40000000){
					unsigned j=0;
					for (j=0;j<active_set_size[1-active];j++){
						if (active_set[1-active][j]==active_set[active][i]) break;
					}
					if (j==active_set_size[1-active]){
						active_set_size[1-active]++;
						if (active_set_size[1-active]==MAX_ACTIVE_SET) fatal("memory: traverse_nfa:: max active set size reached!\n");
						active_set[1-active][j]=active_set[active][i];
					}
				}
				idx=active_set[active][i] & 0x1FFFFF;
				do{
					s=fa[0][idx++];
#ifdef STATISTICS
					tot_tx++;
#endif
							
					if (s!=0xFFFFFFFF && ((s>>21) & 0xFF)==alphabet_tx[c]){
						unsigned v = s & 0x401FFFFF;
						unsigned j=0;
						for (j=0;j<active_set_size[1-active];j++){
							if (active_set[1-active][j]==v) break;
						}
						if (j==active_set_size[1-active]){
							active_set_size[1-active]++;
							if (active_set_size[1-active]==MAX_ACTIVE_SET) fatal("memory: traverse_nfa:: max active set size reached!\n");
							active_set[1-active][j]=v;
						}
						if (s & 0x80000000) matches++;
					}
				}while(((s & 0x20000000)==0) && (((s>>21) & 0xFF)<=alphabet_tx[c]));
			}
			active=1-active;
			active_set_size[1-active]=0;
			
		} //and for c
		
		gettimeofday(&end,NULL);
		r_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
		total_time += r_time;
		
		if(!(pk%1000))
			printf("Current Throughput %0.6g Bps <> %0.6g bps\r", total_bytes*1000000/total_time, total_bytes*8000000/total_time);
		
		
	}
	printf("\n");
	
	printf("\n\nFinal Throughput %0.6g Bps <> %0.6g bps\r", total_bytes*1000000/total_time, total_bytes*8000000/total_time);
	
	
// 	printf("Running_time=%ld sec. %ld usec.\n",r_time/1000000,r_time%1000000);
// 	printf("%d matches detected.\n",matches);
// 	fprintf(log,"NFA %ld\n",r_time);
	
#ifdef STATISTICS
	fprintf(log,"AVG_ACTIVE_SET %f\n",(float)avg_active_set/trace_size);
	fprintf(log,"MAX_ACTIVE_SET %ld\n",max_active_set);
	fprintf(log,"TOT_TX %ld\n",tot_tx);
#endif	
	fflush(log);			
}

#else

void memory::traverse_nfa(unsigned trace_size, char *data, FILE *log){
	
	struct timeval start,end;
	gettimeofday(&start,NULL);
	
	unsigned matches=0;
	linked_set *active_set=new linked_set();
	active_set->insert(0x40000000); //state 0 is stable
	linked_set *next=new linked_set();

#ifdef STATISTICS
	unsigned avg_active_set=0;
	unsigned max_active_set=0;
	unsigned tot_tx=0;
#endif
	
	unsigned idx,s;
	
	for (unsigned ic=0;ic<trace_size;ic++){
		unsigned char c=data[ic];
		
#ifdef STATISTICS
		unsigned as=active_set->size();
		avg_active_set+=as;
		if (as>max_active_set) max_active_set=as;
#endif	
		
		//if (DEBUG) {printf("> active set size=%d: ",active_set->size()); active_set->print();}
		for (linked_set *ls=active_set; ls!=NULL && !ls->empty();ls=ls->succ()){
			if (ls->value() & 0x40000000) next->insert(ls->value());
			idx=ls->value() & 0x1FFFFF;
			do{
				s=fa[0][idx++];
#ifdef STATISTICS
				tot_tx++;
#endif							
				if (s!=0xFFFFFFFF && ((s>>21) & 0xFF)==alphabet_tx[c]){
					next->insert(s & 0x401FFFFF);
					if (s & 0x80000000) matches++;
				}
			}while(((s & 0x20000000)==0) && (((s>>21) & 0xFF)<=alphabet_tx[c]));
		}
		delete active_set;
		active_set=next;
		next=new linked_set();		
	} //and for c
	
	gettimeofday(&end,NULL);
	unsigned r_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
	printf("Running_time=%ld sec. %ld usec.\n",r_time/1000000,r_time%1000000);
	printf("%d matches detected.\n",matches);
	fprintf(log,"NFA %ld\n",r_time);
	
	delete active_set;
	delete next;

#ifdef STATISTICS
	fprintf(log,"AVG_ACTIVE_SET %f\n",(float)avg_active_set/trace_size);
	fprintf(log,"MAX_ACTIVE_SET %ld\n",max_active_set);
	fprintf(log,"TOT_TX %ld\n",tot_tx);	
#endif		
	fflush(log);	
}

#endif

void memory::traverse_hfa(unsigned trace_size, char *data, FILE *log){
	
	struct timeval start,end;
	
	
	unsigned matches=0;
	
	unsigned state[num_dfas], r_time;
	bool compressed[num_dfas];
	bool border=false;
	unsigned num_active_tails=0;
	unsigned s,ds,idx;
	
	for (int i=0;i<num_dfas;i++) {state[i]=0xFFFFFFFF; compressed[i]=0;}
	state[0]=0;	
	
#ifdef STATISTICS
	unsigned max_active_tails=0;
	unsigned avg_active_tails=0;
	unsigned tot_tx=0;
	unsigned tot_default_taken=0;
	unsigned tot_full_traversed=0;
	unsigned tot_compressed_traversed=0;
	unsigned tot_compressed_taken=0;
#endif	

	long inputs=0;
	char payload[2000];
	int payload_size,pi,pj,pk,p_size;
	int64_t cost, copycost;
	double variance,total_bytes=0,total_time=0;
	
	border=false;
	num_active_tails=0;
	for (int i=0;i<num_dfas;i++) {state[i]=0xFFFFFFFF; compressed[i]=0;}
	state[0]=0;

	
	for(pk=0;pk<MAX_PKT;pk++){
		get_payload(payload, &payload_size);
		total_bytes+=payload_size;
		
		gettimeofday(&start,NULL);
		
                //Reset
		bool border=false;
		num_active_tails=0;
		for (int i=0;i<num_dfas;i++) {state[i]=0xFFFFFFFF; compressed[i]=0;}
		state[0]=0;
        	
        	data=payload;
		trace_size=payload_size;
			
		for (unsigned ic=0;ic<trace_size;ic++){
			unsigned char c=data[ic];		
			
#ifdef STATISTICS
			avg_active_tails+=num_active_tails;
			if (num_active_tails>max_active_tails) max_active_tails=num_active_tails;
#endif			
					
			//update the active tail-DFA, if necessary
			if (border){					
				do{
					unsigned n=fa[0][state[0]] & 0x7FFFFFFF;					
					if (state[n]==0xFFFFFFFF){
						state[n]=0;
						num_active_tails++;
					}
				}while(fa[0][state[0]++] & 0x80000000 == 0);
			}
					
			//compute next transition for head-DFA
			while(true){					
				idx = state[0];
				if (compressed[0]){ // compressed state
					ds = fa[0][idx++];
#ifdef STATISTICS
					tot_tx++;
					tot_compressed_taken++;
					tot_compressed_traversed++;
#endif							
					if (ds & 0x20000000){						
						state[0] = ds & 0xFFFFF;
						compressed[0] = ds & 0x40000000;
#ifdef STATISTICS
						tot_default_taken++;
#endif												
					}else{
						do{
							s = fa[0][idx++];
#ifdef STATISTICS
							tot_tx++;
							tot_compressed_taken++;
#endif						
						}while((s & 0x20000000)==0 && (((s>>20) & 0xFF)!=alphabet_tx[c]));
						if (((s>>20) & 0xFF)==alphabet_tx[c]){
							state[0] = s & 0xFFFFF;
							compressed[0] = s & 0x40000000;
							border = s & 0x10000000;
							if (s & 0x80000000) matches++;
							break;
						}else{
							state[0] = ds & 0xFFFFF;
							compressed[0] = ds & 0x40000000;
#ifdef STATISTICS
							tot_default_taken++;
#endif						
						}
					}
				}else{ 				// non compressed state
					s = fa[0][idx+alphabet_tx[c]];
#ifdef STATISTICS
					tot_tx++;
					tot_full_traversed++;
#endif				
					state[0] = s & 0xFFFFF;
					compressed[0] = s & 0x40000000;
					border = s & 0x10000000;
					if (s & 0x80000000) matches++;
					break;
				}
			}//end compute next transition on head-DFA
					
			//tail processing
			if (num_active_tails!=0){					
				for (int i=1;i<num_dfas;i++){
					while(state[i]!=0xFFFFFFFF){
						idx = state[i];
						if (compressed[i]){ // compressed state
							ds = fa[i][idx++];
#ifdef STATISTICS
							tot_tx++;
							tot_compressed_traversed++;
							tot_compressed_taken++;
#endif						
							if (ds & 0x20000000){
								if (ds & 0x10000000){
									state[i]=0xFFFFFFFF;
									num_active_tails--;										
								}else{
									state[i] = ds & 0xFFFFF;
									compressed[i] = ds & 0x40000000;
#ifdef STATISTICS
									tot_default_taken++;
#endif						
								}
							}else{
								do{
									s = fa[i][idx++];
#ifdef STATISTICS
									tot_tx++;
									tot_compressed_taken++;
#endif								
								}while((s & 0x20000000)==0 && (((s>>20) & 0xFF)!=alphabet_tx[c]));
								if (((s>>20) & 0xFF)==alphabet_tx[c]){
									if (s & 0x80000000) matches++;
									if (s & 0x10000000){
										state[i]=0xFFFFFFFF;
										num_active_tails--;
									}else{
										state[i] = s & 0xFFFFF;
										compressed[i] = s & 0x40000000;									
									}
									break;
								}else{
									if (ds & 0x10000000){
										state[i]=0xFFFFFFFF;
										num_active_tails--;											
									}else{
										state[i] = ds & 0xFFFFF;
										compressed[i] = ds & 0x40000000;
#ifdef STATISTICS
										tot_default_taken++;
#endif							
									}
								}
							}
						}else{ 				// non compressed state						
							s = fa[i][idx+alphabet_tx[c]];
#ifdef STATISTICS
							tot_tx++;
							tot_full_traversed++;
#endif						
							if ( s & 0x80000000 ) matches++;
							if ( s & 0x10000000 ){
								state[i]=0xFFFFFFFF;
								num_active_tails--;
							}else{
								state[i] = s & 0xFFFFF;
								compressed[i] = s & 0x40000000;															
							}
							break;
						}
					} //end while-true (processing of single char until labeled transition is taken or dead state is reached)
				} //end for-tail-processing
			}//end if-tail-processing
		} //and for c
		
		gettimeofday(&end,NULL);
		r_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
		
		total_time += r_time;
		
		if(!(pk%1000))
			printf("Current Throughput %0.6g Bps <> %0.6g bps\r", total_bytes*1000000/total_time, total_bytes*8000000/total_time);
		
	}
	
	printf("\n");
	
	printf("\n\nFinal Throughput %0.6g Bps <> %0.6g bps\r", total_bytes*1000000/total_time, total_bytes*8000000/total_time);
// 	printf("Running_time=%ld sec. %ld usec.\n",r_time/1000000,r_time%1000000);
// 	printf("%d matches detected.\n",matches);
// 	fprintf(log,"HFA %ld\n",r_time);

#ifdef STATISTICS
	fprintf(log,"AVG_ACTIVE_TAILS %f\n",(float)avg_active_tails/trace_size);
	fprintf(log,"MAX_ACTIVE_TAILS %ld\n",max_active_tails);
	fprintf(log,"TOT_TX %ld\n",tot_tx);
	fprintf(log,"TOT_DEFAULT_TAKEN %ld\n",tot_default_taken);
	fprintf(log,"TOT_COMPRESSED_TAKEN %ld\n",tot_compressed_taken);
	fprintf(log,"TOT_COMPRESSED_TRAVERSED %ld\n",tot_compressed_traversed);
	fprintf(log,"TOT_FULL_TRAVERSED %ld\n",tot_full_traversed);
#endif		
	fflush(log);
}


int memory::open_capture(char *filename, char *dumpfilename){
	const char* outfilename=dumpfilename;
            
	fout = fopen(outfilename,"w");
	if(fout == NULL){
		printf("->Unable to open dumpfile %s\n", outfilename);
		return 0;
	}
	
	
/*	if(PcapHandle!=NULL){
		pcap_close(PcapHandle);
	}
*/	
	
	PcapHandle=NULL;
	
	if ((PcapHandle= pcap_open_offline(filename, ErrBuf)) == NULL)
	{
		printf("Cannot opening the capture source file: %s\n", ErrBuf);
		return 0;
	}
            
	if (pcap_compile(PcapHandle, &FilterCode, "(tcp)", 1, 0xffffffff)<0)
	{
		printf("Error compiling the packet filter: wrong syntax.\n");
		return 0;
	}
            
	if (pcap_setfilter(PcapHandle, &FilterCode) == -1) {
		fprintf(stderr, "Couldn't install filter tcp: %s\n", pcap_geterr(PcapHandle));
		return 0;
	}
            
	return 1;

}


void memory::get_payload(char *payload, int *size){
            
	int RetVal, length, payload_size=0;
	struct pcap_pkthdr *pkt_header;
	const unsigned char *pkt_data, *payload_ptr;
	int size_ethernet = sizeof(struct sniff_ethernet);
	int size_ip = sizeof(struct sniff_ip);
	int size_tcp = sizeof(struct sniff_tcp);
	short total_length;
    
	while(payload_size==0){
		RetVal = pcap_next_ex(PcapHandle, &pkt_header, &pkt_data);

		if (RetVal <= 0)
		{
			printf("\nCannot read from the capture source file: %s\n", pcap_geterr(PcapHandle) );
			exit(1);
		}
                    
		//payload_size = pkt_header->len-(size_ethernet + size_ip + size_tcp);
		memcpy(&length,pkt_data + size_ethernet, 4);
		length&=0x0f;
		length*=4;
		size_ip=length;
                    
		memcpy(&total_length,pkt_data + size_ethernet + 2, 2);
		total_length=ntohs(total_length);
		//printf("IP->%d Total->%d ", length, total_length);
                    
		payload_ptr = (u_char *)(pkt_data + size_ethernet + size_ip);
                    
                memcpy(&length,payload_ptr + 12, 4);
                length=(length & 0xf0) >> 4;
                length*=4;
                size_tcp=length;
                payload_ptr = (u_char *)(payload_ptr + size_tcp);
                    
                //printf("TCP->%d\n", length);
                payload_size = total_length-(size_tcp+size_ip);
                    
                    
                memcpy(payload, payload_ptr, payload_size);
                payload[payload_size]='\0';
                *size=payload_size;
                   
	}
}

