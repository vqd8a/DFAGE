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
 * File:   trace.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 */

#include "trace.h"
#include "dheap.h"

#include <set>
#include <iterator>

using namespace std;

trace::trace(char *filename){
	tracefile=NULL;
	if (filename!=NULL) set_trace(filename);
	else tracename=NULL;
}
	
trace::~trace(){
	if (tracefile!=NULL) fclose(tracefile);
}

void trace::set_trace(char *filename){
	if (tracefile!=NULL) fclose(tracefile);
	tracename=filename;
	tracefile=fopen(tracename,"r");
	if (tracefile==NULL) fatal("trace:: set_trace: error opening trace-file\n");
}
	
void trace::traverse(DFA *dfa, FILE *stream){
	if (tracefile==NULL) fatal("trace file is NULL!");
	rewind(tracefile);
	
	if (VERBOSE) fprintf(stderr, "\n=>trace::traverse DFA on file %s\n...",tracename);
	
	if (dfa->get_depth()==NULL) dfa->set_depth();
	unsigned int *dfa_depth=dfa->get_depth();
	
	state_t state=0;
	int c=fgetc(tracefile);
	long inputs=0;
	
	unsigned int *stats=allocate_uint_array(dfa->size());
	for (int j=1;j<dfa->size();j++) stats[j]=0;
	stats[0]=1;	
	linked_set *accepted_rules=new linked_set();
	
	while(c!=EOF){
		state=dfa->get_next_state(state,(unsigned char)c);
		stats[state]++;
		if (!dfa->accepts(state)->empty()){
			accepted_rules->add(dfa->accepts(state));
			if (DEBUG){
				char *label=NULL;  
				linked_set *acc=dfa->accepts(state);
				while(acc!=NULL && !acc->empty()){
					if (label==NULL){
						label=(char *)malloc(100);
						sprintf(label,"%d",acc->value());
					}else{
						char *tmp=(char *)malloc(5);
						sprintf(tmp,",%d",acc->value());
						label=strcat(label,tmp); 
						free(tmp);
					}
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		inputs++;
		c=fgetc(tracefile);
	}
	fprintf(stream,"\ntraversal statistics:: [state #, depth, # traversals, %%time]\n");
	int num=0;
	for (int j=0;j<dfa->size();j++){
		if(stats[j]!=0){
			fprintf(stream,"[%ld, %ld, %ld, %f %%]\n",j,dfa_depth[j],stats[j],(float)stats[j]*100/inputs);
			num++;
		}
	}
	fprintf(stream,"%ld out of %ld states traversed (%f %%)\n",num,dfa->size(),(float)num*100/dfa->size());
	fprintf(stream,"rules matched: %ld\n",accepted_rules->size());	
	free(stats);		
	delete accepted_rules;
}

void trace::traverse_compressed(DFA *dfa, FILE *stream){
	if (tracefile==NULL) fatal("trace file is NULL!");
	rewind(tracefile);
	
	if (VERBOSE) fprintf(stderr, "\n=>trace::traverse compressed DFA on file %s\n...",tracename);
	
	if (dfa->get_depth()==NULL) dfa->set_depth();
	unsigned int *dfa_depth=dfa->get_depth();
	
	unsigned int accesses=0;
	unsigned int *stats=allocate_uint_array(dfa->size()); 	  //state traversals (including the ones due to default transitions)
	unsigned int *dfa_stats=allocate_uint_array(dfa->size()); //state traversals in the original DFA
	for (int j=0;j<dfa->size();j++){
		stats[j]=0;
		dfa_stats[j]=0;
	}
	dfa_stats[0]++;
	stats[0]++;
	
	linked_set *accepted_rules=new linked_set();
	state_t *fp=dfa->get_default_tx();
	int *is_fp=new int[dfa->size()];
	for (state_t s=0;s<dfa->size();s++) is_fp[s]=0;
	for (state_t s=0;s<dfa->size();s++) if(fp[s]!=NO_STATE) is_fp[fp[s]]=1;
	
	unsigned int inputs=0;
	state_t state=0;
	int c=fgetc(tracefile);
	while(c!=EOF){
		state=dfa->lookup(state,c,&stats,&accesses);
		dfa_stats[state]++;
		if (!dfa->accepts(state)->empty()){
			accepted_rules->add(dfa->accepts(state));
			if (DEBUG){
				char *label=NULL;  
				linked_set *acc=dfa->accepts(state);
				while(acc!=NULL && !acc->empty()){
					if (label==NULL){
						label=(char *)malloc(100);
						sprintf(label,"%d",acc->value());
					}else{
						char *tmp=(char *)malloc(5);
						sprintf(tmp,",%d",acc->value());
						label=strcat(label,tmp); 
						free(tmp);
					}
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		inputs++;
		c=fgetc(tracefile);
	}
	fprintf(stream,"\ntraversal statistics:: [state #, depth, #traversal, (# traversals in DFA), %%time]\n");
	fprintf(stream,"                         - state_id*: target of default transition - not taken\n");
	fprintf(stream,"                         - *: target of default transition - taken \n");
	
	int num=0;
	int more_states=0;
	int max_depth=0;
	for (int j=0;j<dfa->size();j++){
		if(stats[j]!=0){
			if (dfa_depth[j]>max_depth) max_depth=dfa_depth[j];
			if (is_fp[j]){
				if (dfa_stats[j]==stats[j])
					fprintf(stream,"[%ld*, %ld, %ld, %f %%]\n",j,dfa_depth[j],stats[j],(float)stats[j]*100/accesses);
				else{
					fprintf(stream,"[%ld**, %ld, %ld, %ld, %f %%]\n",j,dfa_depth[j],stats[j],dfa_stats[j],(float)stats[j]*100/accesses);
					more_states++;
				}	
			}
			else fprintf(stream,"[%ld, %ld, %ld, %f %%]\n",j,dfa_depth[j],stats[j],(float)stats[j]*100/accesses);
			num++;
		}
	}
	fprintf(stream,"%ld out of %ld states traversed (%f %%)\n",num,dfa->size(),(float)num*100/dfa->size());
	fprintf(stream,"rules matched: %ld\n",accepted_rules->size());
	fprintf(stream,"fraction traversals %f\n",(float)accesses/inputs);
	fprintf(stream,"number of additional states %ld\n",more_states);
	fprintf(stream,"max depth reached %ld\n",max_depth);	
	free(stats);
	free(dfa_stats);
	delete [] is_fp;		
	delete accepted_rules;
}

void trace::traverse(NFA *nfa, FILE *stream){
	if (tracefile==NULL) fatal("trace file is NULL!");
	rewind(tracefile);
	
	if (VERBOSE) fprintf(stderr, "\n=>trace::traverse NFA on file %s\n...",tracename);
	
	nfa->reset_state_id(); //reset state identifiers in breath-first order
	
	//statistics
	unsigned int *active=allocate_uint_array(nfa->size()); //active[x]=y if x states are active for y times
	unsigned int *stats=allocate_uint_array(nfa->size()); //stats[x]=y if state x was active y times
	for (int j=0;j<nfa->size();j++){
		stats[j]=0;
		active[j]=0;
	}
	
	//code
	nfa_set *nfa_state=new nfa_set();
	nfa_set *next_state=new nfa_set();
	linked_set *accepted_rules=new linked_set();
	
	nfa_set *closure = nfa->epsilon_closure(); 
	
	nfa_state->insert(closure->begin(),closure->end());
	delete closure;
	
	FOREACH_SET (nfa_state,it) stats[(*it)->get_id()]=1;
	active[nfa_state->size()]=1;
	
	int inputs=0;
	int c=fgetc(tracefile);	
	while(c!=EOF){
		FOREACH_SET(nfa_state,it){
			nfa_set *target=(*it)->get_transitions(c);
			if (target!=NULL){
				FOREACH_SET(target,it2){
					nfa_set *target_closure=(*it2)->epsilon_closure();	
					next_state->insert(target_closure->begin(),target_closure->end());
					delete target_closure;
				}
	            delete target;        	   
			}
		}
		delete nfa_state;
		nfa_state=next_state;
		next_state=new nfa_set();
		active[nfa_state->size()]++;
		FOREACH_SET (nfa_state,it) stats[(*it)->get_id()]++;
		
		linked_set *rules=new linked_set();
		
		FOREACH_SET (nfa_state,it) rules->add((*it)->get_accepting());
				
		if (!rules->empty()){
			accepted_rules->add(rules);
			if (DEBUG){
				char *label=(char *)malloc(100);
				sprintf(label,"%d",rules->value());
				linked_set *acc=rules->succ();
				while(acc!=NULL && !acc->empty()){
					char *tmp=(char *)malloc(5);
					sprintf(tmp,",%d",acc->value());
					label=strcat(label,tmp); free(tmp);
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		delete rules;
		inputs++;
		c=fgetc(tracefile);
	}//end while (traversal)
	
	fprintf(stream,"\ntraversal statistics:: [state #,#traversal, %%time]\n");
	unsigned long num=0;
	for (int j=0;j<nfa->size();j++)
		if(stats[j]!=0){
			fprintf(stream,"[%ld,%ld,%f %%]\n",j,stats[j],(float)stats[j]*100/inputs);
			num++;
		}
	fprintf(stream,"%ld out of %ld states traversed (%f %%)\n",num,nfa->size(),(float)num*100/nfa->size());
	fprintf(stream,"\ntraversal statistics:: [size of active state vector #,frequency, %%time]\n");
	num=0;
	for (int j=0;j<nfa->size();j++)
		if(active[j]!=0){
			fprintf(stream,"[%ld,%ld,%f %%]\n",j,active[j],(float)active[j]*100/inputs);
			num+=j*active[j];
		}
	fprintf(stream,"average size of active state vector %f\n",(float)num/inputs);
	fprintf(stream,"rules matched: %ld\n",accepted_rules->size());	
	free(stats);		
	free(active);
	delete nfa_state;
	delete next_state;
	delete accepted_rules;
}

void trace::traverse(HybridFA *hfa, FILE *stream){
	if (tracefile==NULL) fatal("trace file is NULL!");
	rewind(tracefile);
	
	if (VERBOSE) fprintf(stderr, "\n=>trace::traverse Hybrid-FA on file %s\n...",tracename);
	
	NFA *nfa=hfa->get_nfa();
	DFA *dfa=hfa->get_head();
	map <state_t,nfa_set*> *border=hfa->get_border();
	
	//statistics
	unsigned long border_stats=0;  //border states
	
	//here we log how often a head/DFA state is active
	unsigned int *dfa_stats=allocate_uint_array(dfa->size());
	for (int j=1;j<dfa->size();j++) dfa_stats[j]=0;
	
	//here we log how often a tail/NFA state is active and the size of the active set. Only tail states are considered 
	unsigned int *nfa_active=allocate_uint_array(nfa->size()); //active[x]=y if x states are active for y times
	unsigned int *nfa_stats=allocate_uint_array(nfa->size()); //stats[x]=y if state x was active y times
	for (int j=0;j<nfa->size();j++){
		nfa_stats[j]=0;
		nfa_active[j]=0;
	}
	
	//code
	state_t dfa_state=0;
	dfa_stats[0]=1;	
	
	nfa_set *nfa_state=new nfa_set();
	nfa_set *next_state=new nfa_set();
	linked_set *accepted_rules=new linked_set();
	
	int inputs=0;
	int c=fgetc(tracefile);	
	while(c!=EOF){
		/* head-DFA */
		//compute next state and update statistics
		dfa_state=dfa->get_next_state(dfa_state,(unsigned char)c);
		dfa_stats[dfa_state]++;
		//compute accepted rules
		if (!dfa->accepts(dfa_state)->empty()){
			accepted_rules->add(dfa->accepts(dfa_state));
			if (DEBUG){
				char *label=NULL;  
				linked_set *acc=dfa->accepts(dfa_state);
				while(acc!=NULL && !acc->empty()){
					if (label==NULL){
						label=(char *)malloc(100);
						sprintf(label,"%d",acc->value());
					}else{
						char *tmp=(char *)malloc(5);
						sprintf(tmp,",%d",acc->value());
						label=strcat(label,tmp); 
						free(tmp);
					}
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		
		/* tail-NFA */
		//compute next state (the epsilon transitions had already been removed)
		FOREACH_SET(nfa_state,it){
			nfa_set *target=(*it)->get_transitions(c);
			if (target!=NULL){
				next_state->insert(target->begin(),target->end());
	            delete target;        	   
			}
		}
		delete nfa_state;
		nfa_state=next_state;
		next_state=new nfa_set();
		
		//insert border state if needed
		border_it map_it=border->find(dfa_state);
		if (map_it!=border->end()){
			//printf("%ld:: BORDER %ld !\n",i,dfa_state);
			nfa_state->insert(map_it->second->begin(), map_it->second->end());
			border_stats++;
		}
		
		//update stats
		 nfa_active[nfa_state->size()]++;
		FOREACH_SET (nfa_state,it) nfa_stats[(*it)->get_id()]++;
		
		//accepted rules computation
		linked_set *rules=new linked_set();
		FOREACH_SET (nfa_state,it) rules->add((*it)->get_accepting());		
		if (!rules->empty()){
			accepted_rules->add(rules);
			if (DEBUG){
				char *label=(char *)malloc(100);
				sprintf(label,"%d",rules->value());
				linked_set *acc=rules->succ();
				while(acc!=NULL && !acc->empty()){
					char *tmp=(char *)malloc(5);
					sprintf(tmp,",%d",acc->value());
					label=strcat(label,tmp); free(tmp);
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		delete rules;
		
		//reads one more character from input stream
		inputs++;
		c=fgetc(tracefile);
	}//end while (traversal)
	
	//compute number of states in the NFA part
	unsigned tail_size=hfa->get_tail_size();	
	
	//print statistics
	//DFA
	dfa->set_depth();
	unsigned int *dfa_depth=dfa->get_depth();
	fprintf(stream,"\nhead-DFA=\n");
	fprintf(stream,"traversal statistics:: [state #, depth, # traversals, %%time]\n");
	int num=0;
	for (int j=0;j<dfa->size();j++){
		if(dfa_stats[j]!=0){
			fprintf(stream,"[%ld, %ld, %ld, %f %%]\n",j,dfa_depth[j],dfa_stats[j],(float)dfa_stats[j]*100/inputs);
			num++;
		}
	}
	fprintf(stream,"%ld out of %ld head-states traversed (%f %%)\n",num,dfa->size(),(float)num*100/dfa->size());
	//NFA
	fprintf(stream,"\ntail-NFA=\n");
	fprintf(stream,"traversal statistics:: [state #,#traversal, %%time]\n");
	num=0;
	for (int j=0;j<nfa->size();j++)
		if(nfa_stats[j]!=0){
			fprintf(stream,"[%ld,%ld,%f %%]\n",j,nfa_stats[j],(float)nfa_stats[j]*100/inputs);
			num++;
		}
	fprintf(stream,"%ld out of %ld tail-states traversed (%f %%)\n",num,nfa->size(),(float)num*100/tail_size);
	fprintf(stream,"\ntraversal statistics:: [size of active state vector #,frequency, %%time]\n");
	num=0;
	for (int j=0;j<nfa->size();j++)
		if(nfa_active[j]!=0){
			fprintf(stream,"[%ld,%ld,%f %%]\n",j,nfa_active[j],(float)nfa_active[j]*100/inputs);
			num+=j*nfa_active[j];
		}
	fprintf(stream,"average size of active state vector %f\n",(float)num/inputs);
	fprintf(stream,"border states: [total traversal, %%time] %ld %f %%\n",border_stats,(float)border_stats*100/inputs);
	
	fprintf(stream,"\nrules matched: %ld\n",accepted_rules->size());	
	
	
	free(dfa_stats);	
	free(nfa_stats);		
	free(nfa_active);
	delete nfa_state;
	delete next_state;
	delete accepted_rules;
}

int trace::bad_traffic(DFA *dfa, state_t s){
	state_t **tx=dfa->get_state_table();
	int forward[CSIZE];
	int backward[CSIZE];
	int num_fw=0;
	int num_bw=0;
	for (int i=0;i<CSIZE;i++){
		if(dfa->get_depth()[tx[s][i]]>dfa->get_depth()[s]) forward[num_fw++]=i;
		else backward[num_bw++]=i;
	}
	//assert(num_fw+num_bw==CSIZE);
	srand(seed++);
	if (num_fw>0) return forward[randint(0,num_fw-1)];
	else return backward[randint(0,num_bw-1)];
}

int trace::avg_traffic(DFA *dfa, state_t s){
	return randint(0,CSIZE-1);
}

int trace::syn_traffic(DFA *dfa, state_t s,float p_fw){
	state_t **tx=dfa->get_state_table();
	int forward[CSIZE];
	int backward[CSIZE];
	int num_fw=0;
	int num_bw=0;
	int threshold=(int)(p_fw*10);
	for (int i=0;i<CSIZE;i++){
		if(dfa->get_depth()[tx[s][i]]>dfa->get_depth()[s]) forward[num_fw++]=i;
		else backward[num_bw++]=i;
	}
	srand(seed++);
	int selector=randint(1,10);
	if ((selector<=threshold && num_fw>0)||num_bw==0) return forward[randint(0,num_fw-1)];
	else return backward[randint(0,num_bw-1)];
}

int trace::syn_traffic(nfa_set *active_set, float p_fw, bool forward){
	srand(seed++);
	int selector=randint(1,100);
	//random character
	if (selector > 100*p_fw) {
		//printf("syn_traffic exit - \n");
		return randint(0,CSIZE-1);
	} 
    //maximize moves forward
	int weight[CSIZE];
	int max_weight=0;
	int num_char=0;
	for (int c=0;c<CSIZE;c++){
		if (forward){
			NFA *next = get_next_forward(active_set,c);
			if (next!=NULL) weight[c] = next->get_depth();
			else weight[c] = 0;
		}
		else {
			nfa_set *next=get_next(active_set,c,true);
			weight[c]=next->size();
			delete next;
		}
		if (weight[c]>max_weight) max_weight=weight[c];
	}
	for (int c=0;c<CSIZE;c++) if (weight[c]==max_weight) num_char++;
	if (num_char==0) return randint(0,CSIZE-1);
	int selection=randint(1,num_char);
	int c ;
	for (c=0;c<CSIZE;c++) {
		if (weight[c]==max_weight) num_char--;
		if (num_char==0) break;
	}
	//printf("syn_traffic exit %d\n",c);
	return c;
}

nfa_set *trace::get_next(nfa_set *active_set, int c, bool forward){
	nfa_set *next_state = new nfa_set();
	FOREACH_SET(active_set,it){
		NFA *state=*it;
		nfa_set *tx = state->get_transitions(c);
		if (tx!=NULL){
			FOREACH_SET(tx,it2){
				if (!forward || (*it2)->get_depth() > state->get_depth()){
					nfa_set *closure=(*it2)->epsilon_closure();
					next_state->insert(closure->begin(),closure->end());
					delete closure;	
				}
			}
			delete tx;
		}
	}
	return next_state;
}

NFA *trace::get_next_forward(nfa_set *active_set, int c){
	NFA *result = NULL;
	FOREACH_SET(active_set,it){
		nfa_set *tx = (*it)->get_transitions(c);
		if (tx!=NULL){
			FOREACH_SET(tx,it2){
				if (result==NULL || (*it2)->get_depth()>result->get_depth()) result=*it2;
			}
			delete tx;
		}
	}
	return result;
}

FILE *trace::generate_trace(NFA *nfa,int in_seed, float p_fw, bool forward, char *trace_name){
	
	FILE *trace=fopen(trace_name,"w");
	if (trace==NULL) fatal("trace::generate_trace(): could not open the trace file\n");
	
	//seed setting
	seed=in_seed;
	srand(seed);
	
	//some initializations...
	nfa->set_depth();  
	nfa_set *active_set = nfa->epsilon_closure();
	unsigned inputs=0;
	unsigned int stream_size=randint(MIN_INPUTS,MAX_INPUTS);
	
	//traversal
	int c=syn_traffic(active_set,p_fw,forward);
	while(inputs<stream_size){
		
		inputs++;
		fputc(c,trace);
			
		//update active set
		nfa_set *next_state=get_next(active_set,c);
		delete active_set;
		active_set=next_state;
		
		//read next char
		c=syn_traffic(active_set,p_fw,forward);
		
	} //end traversal
	delete active_set;
	
	return trace;
}

void trace::traverse(dfas_memory *mem, double *data){
	
	printf("trace:: traverse(dfas_memory) : using tracefile = %s\n",tracename);
	
	//tracefile
	if (tracefile!=NULL) rewind(tracefile);
	else fatal ("trace::traverse(): No tracefile\n");

    unsigned num_dfas=mem->get_num_dfas();
	DFA **dfas=mem->get_dfas();
	for (int i=0;i<num_dfas;i++) dfas[i]->set_depth();
	
	//statistics
	long cache_stats[2]; cache_stats[0]=cache_stats[1]=0; //0=hit, 1=miss
	
	linked_set *accepted_rules=new linked_set();
	
	long mem_accesses=0; //total number of memory accesses
	unsigned int inputs=0; //total number of inputs
	unsigned max_depth=0;
	unsigned *visited=new unsigned[mem->get_num_states()];
	for (unsigned i=0;i<mem->get_num_states();i++) visited[i]=0;
	unsigned int state_traversals=0; //state traversals
	int m_size; //size of current number of memory accesses

	/* traversal */
	
	state_t *state=new state_t[num_dfas]; //current state (for each active DFA)
	for (int i=0;i<num_dfas;i++) state[i]=0;
		
	int c=fgetc(tracefile);
	while(c!=EOF){
		inputs++;
		for (int idx=0;idx<num_dfas;idx++){
			DFA *dfa=dfas[idx];
			bool fp=true;
			while (fp){
				state_traversals++;
				visited[mem->get_state_index(idx,state[idx])]++;
				//access memory
				int *m = mem->get_dfa_accesses(idx,state[idx],c,&m_size);
				mem_accesses+=m_size;
				//trace_accesses(m,m_size,mem_trace);
				
				for (int i=0;i<m_size;i++) cache_stats[mem->read(m[i])]++;
				delete [] m;
				
				//next state
				state_t next_state=NO_STATE;
				FOREACH_TXLIST(dfa->get_labeled_tx()[state[idx]],it){
					if ((*it).first==c){
						next_state=(*it).second;
						break;
					}
				}
				if (next_state==NO_STATE && mem->allow_def_tx(idx,state[idx])){ 
					state[idx]=dfa->get_default_tx()[state[idx]];
				}else{
					state[idx]=dfa->get_state_table()[state[idx]][c];
					if (dfa->get_depth()[state[idx]]>max_depth) max_depth=dfa->get_depth()[state[idx]];
					fp=false;	
				}
			}
			
			//matching
			if (!dfa->accepts(state[idx])->empty()){
				accepted_rules->add(dfa->accepts(state[idx]));  
				linked_set *acc=dfa->accepts(state[idx]);
			}
		}
		c=fgetc(tracefile);
	} //end traversal
	delete [] state;
	
	// statistics computation
	unsigned num=0;
	for (int j=0;j<mem->get_num_states();j++) if (visited[j]!=0) num++;
			
	if (data!=NULL){
		data[0]=inputs; //number of inputs
		data[1]=accepted_rules->size(); //matches
		data[2]=max_depth; //max depth reached
		data[3]=(double)num*100/mem->get_num_states(); //% states traversed
		data[4]=(double)state_traversals/inputs;  //state traversal/input
		data[5]=(double)mem_accesses/inputs; //mem access/input
		data[6]=(double)100*cache_stats[0]/mem_accesses; //hit rate
		data[7]=(double)100*cache_stats[1]/mem_accesses; //miss rate
		data[8]=(double)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs; //clock cycle/input
	}
	
	//free memory
	delete [] visited;		
	delete accepted_rules;
}

void trace::traverse(fa_memory *mem, double *data, int in_seed, float p_fw, bool forward){
	
	if (tracefile==NULL){
		printf("trace:: traverse(fa_memory) : probabilistic traversal; seed = %ld\n",in_seed);
		seed=in_seed;
		srand(seed);
	} else{
		printf("trace:: traverse(fa_memory) : using tracefile = %s\n",tracename);
		rewind(tracefile);
	}

	if (mem->get_dfa()!=NULL) 
		traverse_dfa(mem, data, in_seed, p_fw);
	else if (mem->get_nfa()!=NULL) 
		traverse_nfa(mem, data, in_seed, p_fw, forward);
	else 
		fatal("trace:: traverse - empty memory");
	
}

void trace::traverse_dfa(fa_memory *mem, double *data, int in_seed, float p_fw){
	//dfa
	DFA *dfa=mem->get_dfa();
	dfa->set_depth();
	unsigned int *dfa_depth=dfa->get_depth();
	state_t *fp=dfa->get_default_tx();
	tx_list **lab_tx=dfa->get_labeled_tx();
	state_t **tx=dfa->get_state_table();
	
	//statistics
	unsigned int *lab_stats=allocate_uint_array(dfa->size());
	unsigned int *fp_stats=allocate_uint_array(dfa->size());
	unsigned int *mem_stats=allocate_uint_array(MAX_MEM_REF);
	for (int j=0;j<dfa->size();j++){
		lab_stats[j]=0;
		fp_stats[j]=0;
	}
	for (int j=0;j<MAX_MEM_REF;j++) mem_stats[j]=0;
	lab_stats[0]++;
	long cache_stats[2]; cache_stats[0]=cache_stats[1]=0; //0=hit, 1=miss
	
	linked_set *accepted_rules=new linked_set();
	long mem_accesses=0; //total number of memory accesses
	unsigned int inputs=0; //total number of inputs
	unsigned int stream_size=randint(MIN_INPUTS,MAX_INPUTS);
	unsigned int state_traversals=0; //state traversals
	int m_size; //size of current number of memory accesses

	//traversal
	state_t state=0; 
	int c;
	
	if (tracefile!=NULL) c=fgetc(tracefile);
	else c=syn_traffic(dfa,state,p_fw);
	
	while((tracefile!=NULL && c!=EOF) || (tracefile==NULL && inputs<stream_size) ){
		state_traversals++;
		//access memory
		int *m = mem->get_dfa_accesses(state,c,&m_size);
		mem_accesses+=m_size;
		mem_stats[m_size]++;
		for (int i=0;i<m_size;i++) cache_stats[mem->read(m[i])]++;
		delete [] m;
		
		//next state
		state_t next_state=NO_STATE;
		FOREACH_TXLIST(lab_tx[state],it){
			if ((*it).first==c){
				next_state=(*it).second;
				break;
			}
		}
		if (next_state==NO_STATE && mem->allow_def_tx(state)){ 
			state=fp[state];
			fp_stats[state]++;
		}else{
			state=tx[state][c];
			lab_stats[state]++;
			//next character
			if (tracefile!=NULL) c=fgetc(tracefile);
			else c=syn_traffic(dfa,state,p_fw);
			inputs++;	
		}
		
		//matching
		if (!dfa->accepts(state)->empty()){
			accepted_rules->add(dfa->accepts(state));  
			linked_set *acc=dfa->accepts(state);
			if (DEBUG){
				while(acc!=NULL && !acc->empty()){
					printf("\nrule: %d reached at character %ld \n",acc->value(),inputs);
					acc=acc->succ();
				}
			}
		}
	} //end traversal
	
	// statistics computation
	//printf("\ntraversal statistics:: [state #,depth, #lab tr, #def tr, %%lab tr, %%def tr, %%time]\n");
	int num=0;
	int more_states=0;
	int max_depth=0;
	int num_fp=0;
	for (int j=0;j<dfa->size();j++){
		num_fp+=fp_stats[j];
		if(lab_stats[j]!=0 || fp_stats[j]!=0 ){
			if (dfa_depth[j]>max_depth) max_depth=dfa_depth[j];
			//printf("[%ld, %ld, %ld, %ld, %f %%, %f %%, %f %%]\n",j,dfa_depth[j],lab_stats[j],fp_stats[j],(float)lab_stats[j]*100/state_traversals,
			//		(float)fp_stats[j]*100/state_traversals,(float)(lab_stats[j]+fp_stats[j])*100/state_traversals);		
			num++;
		}
	}
	int *depth_stats=new int[max_depth+1];
	for (int i=0;i<=max_depth;i++) depth_stats[i]=0;

#ifdef LOG	
	//for (state_t s=0;s<dfa->size();s++) depth_stats[dfa->get_depth()[s]]+=(lab_stats[s]+fp_stats[s]);
	printf("%ld out of %ld states traversed (%f %%)\n",num,dfa->size(),(float)num*100/dfa->size());
	printf("rules matched: %ld\n",accepted_rules->size());
	printf("fraction mem accesses/character %f\n",(float)mem_accesses/inputs);
	printf("state traversal - /character %d %f\n",state_traversals,(float)state_traversals/inputs);
	printf("default transitions taken: tot=%ld per input=%f\n",num_fp, (float)num_fp/inputs);
	printf("max depth reached %ld\n",max_depth);	
    /*printf("depth statistics::\n");
    for(int i=0;i<=max_depth;i++) if(depth_stats[i]!=0) printf("[%d=%f]",i,(float)depth_stats[i]*100/state_traversals);
    printf("\n");
    */ 
	printf("cache hits=%ld /hit rate=%f %%, misses=%ld /miss rate=%f %%\n",
			cache_stats[0],(float)100*cache_stats[0]/mem_accesses,cache_stats[1],(float)100*cache_stats[1]/mem_accesses);
	printf("clock cycles = %ld, /input= %f\n",(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY),
			(float)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs);			
	
#endif	
	//mem->get_cache()->debug();
			
	if (data!=NULL){
		data[0]=inputs; //number of inputs
		data[1]=accepted_rules->size(); //matches
		data[2]=max_depth; //max depth reached
		data[3]=(double)num*100/dfa->size(); //% states traversed
		data[4]=(double)state_traversals/inputs;  //state traversal/input
		data[5]=(double)mem_accesses/inputs; //mem access/input
		data[6]=(double)100*cache_stats[0]/mem_accesses; //hit rate
		data[7]=(double)100*cache_stats[1]/mem_accesses; //miss rate
		data[8]=(double)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs; //clock cycle/input
	}
	
	//free memory
	free(fp_stats);
	free(lab_stats);
	free(mem_stats);
	delete [] depth_stats;		
	delete accepted_rules;
}


void trace::traverse_nfa(fa_memory *mem, double *data, int in_seed, float p_fw, bool forward){
	//nfa
	NFA *nfa=mem->get_nfa();
	nfa->set_depth();
	
	//statistics
	int nfa_size=nfa->size();
	unsigned int *state_stats=allocate_uint_array(nfa_size);
	unsigned int *set_stats=allocate_uint_array(nfa_size);
	unsigned int *mem_stats=allocate_uint_array(MAX_MEM_REF);
	for (int j=0;j<nfa_size;j++){
		state_stats[j]=0;
		set_stats[j]=0;
	}
	for (int j=0;j<MAX_MEM_REF;j++) mem_stats[j]=0;
	
	long cache_stats[2]; cache_stats[0]=cache_stats[1]=0; //0=hit, 1=miss
	
	linked_set *accepted_rules=new linked_set();
	long mem_accesses=0; //total number of memory accesses
	unsigned int inputs=0; //total number of inputs
	unsigned int stream_size=randint(MIN_INPUTS,MAX_INPUTS);
	unsigned int state_traversals=0; //state traversals
	int m_size =0; //size of current number of memory accesses

	//active set
	nfa_set *active_set = nfa->epsilon_closure();
 
	int c;
	
	if (tracefile!=NULL) c=fgetc(tracefile);
	else c=syn_traffic(active_set,p_fw,forward);
	
	while((tracefile!=NULL && c!=EOF) || (tracefile==NULL && inputs<stream_size)){
		
		inputs++;
		//update statistics
		state_traversals+=active_set->size();
		set_stats[active_set->size()]++;
		FOREACH_SET(active_set,it) state_stats[(*it)->get_id()]++;
		 
		FOREACH_SET(active_set,it){
			
			NFA *state=*it;
			
			//memory access
			int *m = mem->get_nfa_accesses(state,c,&m_size);
			mem_accesses+=m_size;
			mem_stats[m_size]++;
			//trace_accesses(m,m_size,mem_trace);
			if (m!=NULL){
				for (int i=0;i<m_size;i++) cache_stats[mem->read(m[i])]++;
				delete [] m;
			}
			
			//matching
			if (!state->get_accepting()->empty()){
				accepted_rules->add(state->get_accepting());
				if (DEBUG){  
					linked_set *acc=state->get_accepting();
					while(acc!=NULL && !acc->empty()){
						printf("\nrule: %d reached at character %ld \n",acc->value(),inputs);
						acc=acc->succ();
					}
				}
			}
				
		}
		//update active set
		nfa_set *next_state=get_next(active_set,c);
		delete active_set;
		active_set=next_state;
		
		//read next char
		if (tracefile!=NULL) c=fgetc(tracefile);
		else c=syn_traffic(active_set,p_fw,forward);
		
	} //end traversal
	delete active_set;
	
	// statistics computation
	//printf("\ntraversal statistics:: [state #,depth, #lab tr, #def tr, %%lab tr, %%def tr, %%time]\n");
	int num=0;
	int more_states=0;
	int max_depth=0;
	
	nfa_list *queue=new nfa_list();
	nfa->traverse(queue);
	
	FOREACH_LIST(queue,it){
		NFA *state=*it;
		state_t j=state->get_id();
		if(state_stats[j]!=0){
			if (state->get_depth()>max_depth) max_depth=state->get_depth();
			//printf("[%ld, %ld, %ld, %f %%]\n",j,state->get_depth(),state_stats[j],(float)state_stats[j]*100/state_traversals);		
			num++;
		}
	}
	
	int *depth_stats=new int[max_depth+1];
	for (int i=0;i<=max_depth;i++) depth_stats[i]=0;
	FOREACH_LIST(queue,it) depth_stats[(*it)->get_depth()]+=state_stats[(*it)->get_id()];

#ifdef LOG	
	printf("p_m %f\n",p_fw);
	printf("%ld out of %ld states traversed (%f %%)\n",num,nfa_size,(float)num*100/nfa_size);
	printf("rules matched: %ld\n",accepted_rules->size());
	printf("fraction mem accesses/character %f\n",(float)mem_accesses/inputs);
	printf("state traversal - /character %d %f\n",state_traversals,(float)state_traversals/inputs);
	printf("max depth reached %ld\n",max_depth);	
	/*
    printf("depth statistics::\n");
    for(int i=0;i<=max_depth;i++) if(depth_stats[i]!=0) printf("[%d=%f%%]",i,(float)depth_stats[i]*100/state_traversals);
    printf("\n");
    */
	printf("cache hits=%ld /hit rate=%f %%, misses=%ld /miss rate=%f %%\n",
			cache_stats[0],(float)100*cache_stats[0]/mem_accesses,cache_stats[1],(float)100*cache_stats[1]/mem_accesses);
	printf("clock cycles = %ld, /input= %f\n",(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY),
			(float)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs);				
#endif
	
	//mem->get_cache()->debug();
		
	if (data!=NULL){
		data[0]=inputs; //number of inputs
		data[1]=accepted_rules->size(); //matches
		data[2]=max_depth; //max depth reached
		data[3]=(double)num*100/nfa_size; //% states traversed
		data[4]=(double)state_traversals/inputs;  //state traversal/input
		data[5]=(double)mem_accesses/inputs; //mem access/input
		data[6]=(double)100*cache_stats[0]/mem_accesses; //hit rate
		data[7]=(double)100*cache_stats[1]/mem_accesses; //miss rate
		data[8]=(double)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs; //clock cycle/input
	}
	
	//free memory
	free(state_stats);
	free(set_stats);
	free(mem_stats);
	delete [] depth_stats;		
	delete accepted_rules;
	delete queue;
}
