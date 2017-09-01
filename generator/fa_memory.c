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
 * File:   fa_memory.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 * 
 */


#include "fa_memory.h"

char *layout2string(layout_t layout){
	switch(layout){
		case(LINEAR):
			return "LINEAR";
		case(BITMAP):
			return "BITMAP";
		case(INDIRECT_ADDR):
			return "INDIRECT_ADDR";
		default:
			fatal("invalid layout");
	}
}

fa_memory::fa_memory(DFA *dfa_in, layout_t layout_in, int threshold_in){
	dfa=dfa_in;
	nfa=NULL;
	layout=layout_in;
	threshold=threshold_in;
	state_index   = new long[dfa->size()];
	state_address = new long[dfa->size()]; 
	if (layout==INDIRECT_ADDR && ((threshold+1)%4!=0)) fatal("incorrect threshold with content addressing");
	base_states=0;
	num_labeled_tr=new int[dfa->size()];
	bitmap = new int[dfa->size()];
	epsilon=NULL;
	for (state_t s=0;s<dfa->size();s++) bitmap[s]=0;
	_cache = NULL;
	_onchip_size=0;
}

fa_memory::fa_memory(NFA *nfa_in, layout_t layout_in, int threshold_in){
	dfa=NULL;
	nfa=nfa_in;
	nfa->reset_state_id();
	int nfa_size=nfa->size();
	layout=layout_in;
	threshold=threshold_in;
	state_index   = new long[nfa_size];
	state_address = new long[nfa_size]; 
	if (layout==INDIRECT_ADDR && ((threshold+1)%4!=0)) fatal("incorrect threshold with content addressing");
	base_states=0;
	num_labeled_tr=new int[nfa_size];
	epsilon = new int[nfa_size];
	bitmap = new int[nfa_size];
	for (state_t s=0;s<nfa_size;s++) bitmap[s]=0;
	_cache = NULL;
	_onchip_size=0;
}
	
fa_memory::~fa_memory(){
	delete [] state_index;
	delete [] state_address;
	delete [] num_labeled_tr;
	delete [] bitmap;
	if (epsilon!=NULL) delete [] epsilon;
	if (_cache!=NULL) delete _cache;
}

void fa_memory::init(long *data){
	if (dfa!=NULL) init_dfa(data);
	else init_nfa(data);
}
	
void fa_memory::init_dfa(long *data){
	tx_list **labeled_tr=dfa->get_labeled_tx();
	for (state_t s=0;s<dfa->size();s++){
		num_labeled_tr[s]=labeled_tr[s]->size(); 
	}
	int index=0;
	long next_address=0;
	for (state_t s=0;s<dfa->size();s++){
		if (num_labeled_tr[s]>threshold){
			base_states++;
			state_index[s]=index++;
			state_address[s]=next_address;
			if (layout==INDIRECT_ADDR) next_address+=CSIZE*(threshold+1); //byte address
			else next_address+=CSIZE*4; //byte address
		} 
	}
	fa_memory_offset=next_address;
	switch(layout){
		case LINEAR:
			for (state_t s=0;s<dfa->size();s++){
				if (num_labeled_tr[s]<=threshold){
					state_index[s]=index++;
					state_address[s]=next_address;
					next_address+=(num_labeled_tr[s]+1)*4; //let's assume 24 bits for fa_memory + 8 for character 
				} 
			}		
			break;
		case BITMAP:
			for (state_t s=0;s<dfa->size();s++){
				if (num_labeled_tr[s]<=threshold){
					state_index[s]=index++;
					state_address[s]=next_address;
					int bv=0;
					for (int i=0;i<CSIZE/8;i++){
						for (int c=i*8; c<(i+1)*8; c++){
							state_t next_state=NO_STATE;
							FOREACH_TXLIST(labeled_tr[s],it){
								if ((*it).first==c){
									next_state=(*it).second;
									break;
								}
							}
							if (next_state!=NO_STATE){
								bitmap[s] = bitmap[s] | (1 << (CSIZE/8-1-i));
								bv++;
								break;
							}
						}
					}
					//printf("bitmap of %ld = 0x%X\n",s,bitmap[s]);
					//printf("state %ld labeled %d bv %d\n",s,num_labeled_tr[s],bv );
					bv = (bv%4==0) ? bv/4 : bv/4+1;
					next_address += (1+bv+num_labeled_tr[s]+1)*4; //32-bit bv 1st level+mini-bv+labeled trans+fp   
				}
			}
			break;	
		case INDIRECT_ADDR:
			//assume 8 bit discriminator
			// width = (threshold+1)*8 bits
			for (state_t s=0;s<dfa->size();s++){
				if (num_labeled_tr[s]<=threshold){
					state_index[s]=index++;
					state_address[s]=next_address;
					next_address += (num_labeled_tr[s]+1)*(threshold+1);    
				}
			}
			break;	
		default:
			fatal("fa_memory:: invalid layout.");	
	}
	
	printf("\nfa_memory init:: layout %s, threshold %d, DFA size %ld\n",layout2string(layout),threshold,dfa->size());
	printf("fa_memory offset: %ld B, %ld KB, %ld MB, states %ld\n",fa_memory_offset,fa_memory_offset/1024,fa_memory_offset/(1024*1024), base_states);
	printf("fa_memory size: %ld B, %ld KB, %ld MB\n\n",next_address,next_address/1024,next_address/(1024*1024));
	
	if (data!=NULL){
		data[0]=fa_memory_offset;
		data[1]=next_address;
	}
}

void fa_memory::init_nfa(long *data){
	nfa_list *queue=new nfa_list();
	nfa->traverse(queue);
	FOREACH_LIST(queue,it){
		NFA *tmp=*it;
		num_labeled_tr[tmp->get_id()]=tmp->get_transitions()->size();
		epsilon[tmp->get_id()]=!(tmp->get_epsilon()->empty()); 
	}
	int index=0;
	long next_address=0;
	//fully specified states w/ epsilon transition
	FOREACH_LIST(queue,it){
		state_t s=(*it)->get_id();
		if (epsilon[s] && num_labeled_tr[s]>threshold){
			base_states++;
			state_index[s]=index++;
			state_address[s]=next_address;
			if (layout==INDIRECT_ADDR) next_address+=(CSIZE+1)*(threshold+1); //byte address
			else next_address+=(CSIZE+1)*4; //byte address
		} 
	}
	fa_memory_offset=next_address;
	//fully specified states w/o epsilon transition
	FOREACH_LIST(queue,it){
		state_t s=(*it)->get_id();
		if (!epsilon[s] && num_labeled_tr[s]>threshold){
			base_states++;
			state_index[s]=index++;
			state_address[s]=next_address;
			if (layout==INDIRECT_ADDR) next_address+=CSIZE*(threshold+1); //byte address
			else next_address+=CSIZE*4; //byte address
		} 
	}
	fa_memory_offset1=next_address;
	//states w/ epsilon transition
	switch(layout){
		case LINEAR:
			FOREACH_LIST(queue,it){
				state_t s=(*it)->get_id();
				if (epsilon[s] && num_labeled_tr[s]<=threshold){
					state_index[s]=index++;
					state_address[s]=next_address;
					next_address+=(num_labeled_tr[s]+1)*4; //let's assume 24 bits for fa_memory + 8 for character 
				} 
			}		
			break;
		case BITMAP:
			FOREACH_LIST(queue,it){
				state_t s=(*it)->get_id();
				if (epsilon[s] && num_labeled_tr[s]<=threshold){
					state_index[s]=index++;
					state_address[s]=next_address;
					pair_set *tx=(*it)->get_transitions(); //don't delete them!
					int *ltx=new int[CSIZE];
					for (int i=0;i<CSIZE;i++) ltx[i]=0;
					FOREACH_PAIRSET(tx,it2) ltx[(*it2)->first]=1;
					int bv=0;
					for (int i=0;i<CSIZE/8;i++){
						for (int c=i*8; c<(i+1)*8; c++){
							if (ltx[c]==1){
								bitmap[s] = bitmap[s] | (1 << (CSIZE/8-1-i));
								bv++;
								break;
							}
						}
					}
					delete [] ltx;
					//printf("bitmap of %ld = 0x%X\n",s,bitmap[s]);
					//printf("state %ld labeled %d bv %d\n",s,num_labeled_tr[s],bv );
					bv = (bv%4==0) ? bv/4 : bv/4+1;
					next_address += (2+bv+num_labeled_tr[s])*4; //epsilon+32-bit bv 1st level+mini-bv+labeled trans   
				}
			}
			break;	
		case INDIRECT_ADDR:
			//assume 8 bit discriminator
			// width = (threshold+1)*8 bits
			FOREACH_LIST(queue,it){
				state_t s=(*it)->get_id();
				if (epsilon[s] && num_labeled_tr[s]<=threshold){
					state_index[s]=index++;
					state_address[s]=next_address;
					next_address += (num_labeled_tr[s]+1)*(threshold+1);    
				}
			}
			break;	
		default:
			fatal("fa_memory:: invalid layout.");	
	}
	fa_memory_offset2=next_address;
	//states w/o epsilon transition
	switch(layout){
		case LINEAR:
			FOREACH_LIST(queue,it){
				state_t s=(*it)->get_id();
				if (!epsilon[s] && num_labeled_tr[s]<=threshold){
					state_index[s]=index++;
					state_address[s]=next_address;
					next_address+=num_labeled_tr[s]*4; //let's assume 24 bits for fa_memory + 8 for character 
				} 
			}		
			break;
		case BITMAP:
			FOREACH_LIST(queue,it){
				state_t s=(*it)->get_id();
				//printf("state %d\n",s);
				//printf("state eps %d\n",epsilon[s]);
				//printf("state tx %d\n",num_labeled_tr[s]);
				if (!epsilon[s] && num_labeled_tr[s]<=threshold){
				//	printf("ok\n");
					state_index[s]=index++;
					state_address[s]=next_address;
					pair_set *tx=(*it)->get_transitions();
					int *ltx=new int[CSIZE];
					for (int i=0;i<CSIZE;i++) ltx[i]=0;
					FOREACH_PAIRSET(tx,it2) ltx[(*it2)->first]=1;
					int bv=0;
					for (int i=0;i<CSIZE/8;i++){
						for (int c=i*8; c<(i+1)*8; c++){
							if (ltx[c]==1){
								bitmap[s] = bitmap[s] | (1 << (CSIZE/8-1-i));
								bv++;
								break;
							}
						}
					}
					delete [] ltx;
					//printf("bitmap of %ld = 0x%X\n",s,bitmap[s]);
					//printf("state %ld labeled %d bv %d\n",s,num_labeled_tr[s],bv );
					bv = (bv%4==0) ? bv/4 : bv/4+1;
					next_address += (1+bv+num_labeled_tr[s])*4; //32-bit bv 1st level+mini-bv+labeled trans   
				}
			}
			break;	
		case INDIRECT_ADDR:
			//assume 8 bit discriminator
			// width = (threshold+1)*8 bits
			FOREACH_LIST(queue,it){
				state_t s=(*it)->get_id();
				if (!epsilon[s] && num_labeled_tr[s]<=threshold){
					state_index[s]=index++;
					state_address[s]=next_address;
					next_address += num_labeled_tr[s]*(threshold+1);    
				}
			}
			break;	
		default:
			fatal("fa_memory:: invalid layout.");	
	}
		
	printf("\nmemory init:: layout %s, threshold %d, NFA size %ld\n",layout2string(layout),threshold,nfa->size());
	printf("memory offset: %ld B, %ld KB, %ld MB, states %ld\n",fa_memory_offset,fa_memory_offset/1024,fa_memory_offset/(1024*1024), base_states);
	printf("memory size: %ld B, %ld KB, %ld MB\n\n",next_address,next_address/1024,next_address/(1024*1024));
	
	if (data!=NULL){
		data[0]=fa_memory_offset;
		data[1]=next_address;
	}

	delete queue;
}


int *fa_memory::get_dfa_accesses(state_t state,int c, int *size){
	int *result;
	if(state_address[state]<fa_memory_offset){
		if (layout!=INDIRECT_ADDR){
			*size=1;
			result=new int[1];
			result[0]=state_address[state]+c*4;
		}else{
			*size=(threshold+1)/4;
			result=new int[(threshold+1)/4];
			for (int i=0;i<(threshold+1)/4;i++)
				result[i]=state_address[state]+c*(threshold+1)+i*4;
		}
		return result;
	}
	tx_list **labeled_tr=dfa->get_labeled_tx();
	switch(layout){
		case LINEAR: //fp followed by labeled transitions
			{
			int left_lab_tx=0; bool found=false;
			FOREACH_TXLIST(labeled_tr[state],it){
				if ((*it).first<=c) left_lab_tx++;
				if ((*it).first==c) found=true;
			}
			if (found){ //match
				*size=left_lab_tx+1;
				result=new int[left_lab_tx+1];
				for (int i=0;i<left_lab_tx+1;i++) result[i]=state_address[state]+i*4;
			}else{ //mismatch
				*size=left_lab_tx+2;
				result=new int[left_lab_tx+2];
				for (int i=0;i<left_lab_tx+2;i++) result[i]=state_address[state]+i*4;
			}
			return result;
			}
		case BITMAP:
			{int mask=0x80000000;
			int bm = c/8;
			int bm_set_left=0;
			bool bm_set=bitmap[state] & (mask >> bm);
			for (int i=0;i<=bm;i++) if (bitmap[state] & (mask>>i)) bm_set_left++;
			int bm_set_left_word = (bm_set_left%4==0) ? bm_set_left/4 : (bm_set_left/4)+1; 
			int total_bm_set=0;
			for (int i=0;i<CSIZE/8;i++) if (bitmap[state] & (mask>>i)) total_bm_set++;
			int total_bm_set_word = (total_bm_set%4==0) ? total_bm_set/4 : (total_bm_set/4)+1;
			int left_lab_tx=0; bool found=false;
			FOREACH_TXLIST(labeled_tr[state],it){
				if ((*it).first<c) left_lab_tx++;
				if ((*it).first==c) found=true;
			}
			if (bm_set==0){
				*size=2;
				result=new int[2];
				result[0]=state_address[state];	//1st level bm
				result[1]=state_address[state]+(1+total_bm_set_word)*4;//fp address
			}else{
				*size=2+bm_set_left_word;
				result=new int[2+bm_set_left_word]; //BM  1st + BMs 2nd + fp/transition
				result[0]=state_address[state];	//1st level bm
				for (int i=1;i<=bm_set_left_word;i++) //2nd level bitmaps
					result[i]=state_address[state]+i*4;
				if  (!found)
					result[1+bm_set_left_word]=state_address[state]+(1+total_bm_set_word)*4;//fp_address
				else	
					result[1+bm_set_left_word]=state_address[state]+(1+total_bm_set_word+1+left_lab_tx)*4;
			}
			return result;
			}
		case INDIRECT_ADDR:
			{
			*size = (threshold+1)/4;
			result=new int[(threshold+1)/4];
			int left_lab_tx=0; bool found=false;
			FOREACH_TXLIST(labeled_tr[state],it){
				if ((*it).first<c) left_lab_tx++;
				if ((*it).first==c) found=true;
			}
			if (!found){ //follow fp
				for (int i=0;i<(threshold+1)/4;i++)
					result[i]=state_address[state]+i*4;
			}else{
				for (int i=0;i<(threshold+1)/4;i++)
					result[i]=state_address[state]+(1+left_lab_tx)*(threshold+1)+i*4;
			}
			return result;
			}
		default:
			fatal("invalid fa_memory layout");
	}
}

int *fa_memory::get_nfa_accesses(NFA* nfa_state,int c, int *size){
	state_t state=nfa_state->get_id();
	int *result;
	//case 1: direct access w/ epsilon transition
	if(state_address[state]<fa_memory_offset){
		if (layout!=INDIRECT_ADDR){
			*size=2;
			result=new int[*size];
			result[0]=state_address[state]; //epsilon
			result[1]=state_address[state]+(c+1)*4;
		}else{
			*size=2*(threshold+1)/4;
			result=new int[*size];
			for (int i=0;i<(threshold+1)/4;i++)
				result[i]=state_address[state]+i*4;
			for (int i=0;i<(threshold+1)/4;i++)
				result[i+(threshold+1)/4]=state_address[state]+(c+1)*(threshold+1)+i*4;
		}
		return result;
	}
	//case 2: direct access without epsilon transition 
	if(state_address[state]<fa_memory_offset1){
		if (layout!=INDIRECT_ADDR){
			*size=1;
			result=new int[1];
			result[0]=state_address[state]+c*4;
		}else{
			*size=(threshold+1)/4;
			result=new int[*size];
			for (int i=0;i<(threshold+1)/4;i++)
				result[i]=state_address[state]+c*(threshold+1)+i*4;
		}
		return result;
	}
	
	//compute the available labeled transitions
	pair_set *tx=nfa_state->get_transitions(); //do not delete!
	int *ltx = new int[CSIZE];
	for (int i=0;i<CSIZE;i++) ltx[i]=0;
	FOREACH_PAIRSET(tx,it) ltx[(*it)->first]=1; 
	 
	//case 3 : layout based access w/ epsilon
	if (state_address[state]<fa_memory_offset2){
		switch(layout){
			case LINEAR: //fp followed by labeled transitions
				{
					int left_lab_tx=0;
					for (int i=0;i<=c;i++) if(ltx[i]==1) left_lab_tx++;
					if (ltx[c]==1){ //match
						*size=left_lab_tx+1;
						result=new int[*size];
						for (int i=0;i<(*size);i++) result[i]=state_address[state]+i*4;
					}else{ //mismatch
						*size=left_lab_tx+2;
						result=new int[*size];
						for (int i=0;i<(*size);i++) result[i]=state_address[state]+i*4;
					}
					delete [] ltx;
					return result;
				}
			case BITMAP:
				{
					int mask=0x80000000;
					int bm = c/8;
					int bm_set_left=0;
					bool bm_set=bitmap[state] & (mask >> bm);
					for (int i=0;i<=bm;i++) if (bitmap[state] & (mask>>i)) bm_set_left++;
					int bm_set_left_word = (bm_set_left%4==0) ? bm_set_left/4 : (bm_set_left/4)+1; 
					int total_bm_set=0;
					for (int i=0;i<CSIZE/8;i++) if (bitmap[state] & (mask>>i)) total_bm_set++;
					int total_bm_set_word = (total_bm_set%4==0) ? total_bm_set/4 : (total_bm_set/4)+1;
					int left_lab_tx=0;
					for (int i=0;i<c;i++) if(ltx[i]==1) left_lab_tx++;
					if (bm_set==0){
						*size=2; //epsilon and first level bm
						result=new int[2];
						result[0]=state_address[state];	//epsilon transition
						result[1]=state_address[state]+4;//first level BM
					}else if (ltx[c]==1){ //match
						*size=3+bm_set_left_word;
						result=new int[*size]; //BM  epsilon + 1st + BMs 2nd + transition
						result[0]=state_address[state];		//epsilon
						result[1]=state_address[state]+4;	//1st level bm
						for (int i=2;i<bm_set_left_word+2;i++) result[i]=state_address[state]+i*4; //2nd level bitmaps
						result[bm_set_left_word+2]=state_address[state]+(2+total_bm_set_word+left_lab_tx)*4; //transition
					}else{ //mismatch
						*size=2+bm_set_left_word;
						result=new int[*size]; //BM  epsilon + 1st + BMs 2nd + transition
						result[0]=state_address[state];		//epsilon
						result[1]=state_address[state]+4;	//1st level bm
						for (int i=2;i<bm_set_left_word+2;i++) result[i]=state_address[state]+i*4; //2nd level bitmaps						
					}
					delete [] ltx;
					return result;
				}
			case INDIRECT_ADDR:
				{
				if (ltx[c]==0){ //epsilon
					*size = (threshold+1)/4;
					result=new int[*size];
					for (int i=0;i<(*size);i++)
						result[i]=state_address[state]+i*4;
				}else{
					*size = 2*(threshold+1)/4;
					result=new int[*size];
					for (int i=0;i<(threshold+1)/4;i++) //epsilon
						result[i]=state_address[state]+i*4;
					int left_lab_tx=0; //labeled transitions preceeding current one
					for (int i=0;i<c;i++) if(ltx[i]==1) left_lab_tx++;
					for (int i=0;i<(threshold+1)/4;i++)
						result[i+(threshold+1)/4]=state_address[state]+(1+left_lab_tx)*(threshold+1)+i*4;
				}
				delete [] ltx;
				return result;
				}
			default:
				fatal("invalid fa_memory layout");
		}
	}
	
	//case 4 : layout based access w/o epsilon
	if (state_address[state]>=fa_memory_offset2){
		switch(layout){
			case LINEAR: //fp followed by labeled transitions
				{
					int left_lab_tx=0;
					for (int i=0;i<=c;i++) if(ltx[i]==1) left_lab_tx++;
					if (ltx[c]==1){ //match
						*size=left_lab_tx;
						result=new int[*size];
						for (int i=0;i<left_lab_tx;i++) result[i]=state_address[state]+i*4;
					}else{ //mismatch
						*size=left_lab_tx+1;
						result=new int[*size];
						for (int i=0;i<left_lab_tx+1;i++) result[i]=state_address[state]+i*4;
					}
					delete [] ltx;
					return result;
				}
			case BITMAP:
				{
					int mask=0x80000000;
					int bm = c/8;
					int bm_set_left=0;
					bool bm_set=bitmap[state] & (mask >> bm);
					for (int i=0;i<=bm;i++) if (bitmap[state] & (mask>>i)) bm_set_left++;
					int bm_set_left_word = (bm_set_left%4==0) ? bm_set_left/4 : (bm_set_left/4)+1; 
					int total_bm_set=0;
					for (int i=0;i<CSIZE/8;i++) if (bitmap[state] & (mask>>i)) total_bm_set++;
					int total_bm_set_word = (total_bm_set%4==0) ? total_bm_set/4 : (total_bm_set/4)+1;
					int left_lab_tx=0;
					for (int i=0;i<c;i++) if(ltx[i]==1) left_lab_tx++;
					if (bm_set==0){
						*size=1;
						result=new int[1];
						result[0]=state_address[state];//first level BM
					}else if (ltx[c]==1){ //match
						*size=2+bm_set_left_word;
						result=new int[*size]; //BM  epsilon + 1st + BMs 2nd + transition
						result[0]=state_address[state];		//1st lebel BM
						for (int i=1;i<bm_set_left_word+1;i++) result[i]=state_address[state]+i*4; //2nd level bitmaps
						result[bm_set_left_word+1]=state_address[state]+(total_bm_set_word+left_lab_tx)*4; //transition
					}else{ //mismatch
						*size=1+bm_set_left_word;
						result=new int[1+bm_set_left_word]; //BM  epsilon + 1st + BMs 2nd + transition
						result[0]=state_address[state];		//1st level BM
						for (int i=1;i<bm_set_left_word+1;i++) result[i]=state_address[state]+i*4; //2nd level bitmaps						
					}
					delete [] ltx;
					return result;
				}
			case INDIRECT_ADDR:
				{
				if (ltx[c]==0){ 
					*size = 0;
					result=NULL;
				}else{
					*size = (threshold+1)/4;
					result=new int[*size];
					int left_lab_tx=0; //labeled transitions preceeding current one
					for (int i=0;i<c;i++) if(ltx[i]==1) left_lab_tx++;
					for (int i=0;i<(threshold+1)/4;i++)
						result[i]=state_address[state]+(left_lab_tx)*(threshold+1)+i*4;
				}
				delete [] ltx;
				return result;
				}
			default:
				fatal("invalid fa_memory layout");
		}
	}
}

void fa_memory::set_cache(cache_size_t size,line_size_t line_size, assoc_t assoc){
	if (_cache!=NULL) delete _cache;
	_cache = new cache(size,line_size,assoc);
}

mem_access_t fa_memory::read(int address){
	if(_cache!=NULL){
		int cache_res=_cache->read(address);
		return ((cache_res==HIT) ? CACHE_HIT : CACHE_MISS);
	}
	if(_onchip_size!=0){
		return ((address<_onchip_size) ? CACHE_HIT : CACHE_MISS); 
	}
	return DIRECT;
}


