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
 * File:   dfas_memory.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory 
 */

#include "dfas_memory.h"

char *c_layout2string(layout_t layout){
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

dfas_memory::dfas_memory(unsigned num_dfas_in, DFA **dfas_in, layout_t layout_in, int threshold_in){
	num_dfas=num_dfas_in;
	dfas=dfas_in;
	layout=layout_in;
	threshold=threshold_in;
	if (layout==INDIRECT_ADDR && ((threshold+1)%4!=0)) fatal("incorrect threshold with content addressing");
	num_states=0;
	state_offset=new state_t[num_dfas];
	base_states=new state_t[num_dfas];
	memory_base = new long[num_dfas];
	memory_offset=new long[num_dfas];
	for (unsigned i=0;i<num_dfas;i++){
		base_states[i]=0;
		if (i==0) state_offset[i]=0;
		else state_offset[i]=state_offset[i-1]+dfas[i-1]->size();
		num_states+=dfas[i]->size();
	}
	state_address = new long[num_states]; 
	num_labeled_tr=new int[num_states];
	bitmap = new int[num_states];
	for (state_t s=0;s<num_states;s++) bitmap[s]=0;
	_cache = NULL;
	_onchip_size=0;
}

	
dfas_memory::~dfas_memory(){
	delete [] memory_base;
	delete [] memory_offset;
	delete [] state_offset;
	delete [] state_address;
	delete [] num_labeled_tr;
	delete [] base_states;
	delete [] bitmap;
	if (_cache!=NULL) delete _cache;
}
	
void dfas_memory::init(long *data){
	unsigned tot_base_states=0;
	long next_address=0;
	//loop over all DFAs
	for (unsigned idx=0;idx<num_dfas;idx++){
		memory_base[idx]=next_address;
		DFA *dfa=dfas[idx];
		tx_list **labeled_tr=dfa->get_labeled_tx();
		for (state_t s=0;s<dfa->size();s++){
			num_labeled_tr[s+state_offset[idx]]=labeled_tr[s]->size(); 
		}
		for (state_t s=0;s<dfa->size();s++){
			state_t x=s+state_offset[idx];
			if (num_labeled_tr[x]>threshold){
				base_states[idx]++;
				state_address[x]=next_address;
				if (layout==INDIRECT_ADDR) next_address+=CSIZE*(threshold+1); //byte address
				else next_address+=CSIZE*4; //byte address
			} 
		}
		memory_offset[idx]=next_address;
		switch(layout){
			case LINEAR:
				for (state_t s=0;s<dfa->size();s++){
					state_t x=s+state_offset[idx];
					if (num_labeled_tr[x]<=threshold){
						state_address[x]=next_address;
						next_address+=(num_labeled_tr[x]+1)*4; //let's assume 24 bits for dfas_memory + 8 for character 
					} 
				}		
				break;
			case BITMAP:
				for (state_t s=0;s<dfa->size();s++){
					state_t x=s+state_offset[idx];
					if (num_labeled_tr[x]<=threshold){
						state_address[x]=next_address;
						int bv=0;
						for (int i=0;i<CSIZE/8;i++){
							for (int c=i*8; c<(i+1)*8; c++){
								state_t next_state=NO_STATE;
								FOREACH_TXLIST(labeled_tr[s],it){
									if((*it).first==c) {
										next_state=(*it).second;
										break;
									}
								}
								if (next_state!=NO_STATE){
									bitmap[x] = bitmap[x] | (1 << (CSIZE/8-1-i));
									bv++;
									break;
								}
							}
						}
						//printf("bitmap of %ld = 0x%X\n",s,bitmap[s]);
						//printf("state %ld labeled %d bv %d\n",s,num_labeled_tr[s],bv );
						bv = (bv%4==0) ? bv/4 : bv/4+1;
						next_address += (1+bv+num_labeled_tr[x]+1)*4; //32-bit bv 1st level+mini-bv+labeled trans+fp   
					}
				}
				break;	
			case INDIRECT_ADDR:
				//assume 8 bit discriminator
				// width = (threshold+1)*8 bits
				for (state_t s=0;s<dfa->size();s++){
					state_t x=s+state_offset[idx];
					if (num_labeled_tr[x]<=threshold){
						state_address[x]=next_address;
						next_address += (num_labeled_tr[x]+1)*(threshold+1);    
					}
				}
				break;	
			default:
				fatal("dfas_memory:: invalid layout.");	
		}
		tot_base_states+=base_states[idx];
	}
	
	printf("\nmemory init:: layout %s, threshold %d, num states %ld, based %ld\n",c_layout2string(layout),threshold,num_states, tot_base_states);
	printf("memory size: %ld B, %ld KB, %ld MB\n\n",next_address,next_address/1024,next_address/(1024*1024));
		
	if (data!=NULL){
		data[0]=tot_base_states;
		data[1]=next_address;
	}
}

int *dfas_memory::get_dfa_accesses(unsigned dfa_nr, state_t state,int c, int *size){
	int *result;
	state_t state_idx = state+state_offset[dfa_nr];
	if (state_address[state_idx] < memory_base[dfa_nr]) fatal("dfas_memory::get_dfa_accesses: invalid address!");
	if (state_address[state_idx] < memory_offset[dfa_nr]){
		if (layout!=INDIRECT_ADDR){
			*size=1;
			result=new int[1];
			result[0]=state_address[state_idx]+c*4;
		}else{
			*size=(threshold+1)/4;
			result=new int[(threshold+1)/4];
			for (int i=0;i<(threshold+1)/4;i++)
				result[i]=state_address[state_idx]+c*(threshold+1)+i*4;
		}
		return result;
	}
	tx_list **labeled_tr=dfas[dfa_nr]->get_labeled_tx();
	switch(layout){
		case LINEAR: //fp followed by labeled transitions
			{int left_lab_tx=0; bool found=false;
			FOREACH_TXLIST(labeled_tr[state],it){
				if ((*it).first<=c) left_lab_tx++;
				if ((*it).first==c) found=true;
			}
			if (found){ //match
				*size=left_lab_tx+1;
				result=new int[left_lab_tx+1];
				for (int i=0;i<left_lab_tx+1;i++) result[i]=state_address[state_idx]+i*4;
			}else{ //mismatch
				*size=left_lab_tx+2;
				result=new int[left_lab_tx+2];
				for (int i=0;i<left_lab_tx+2;i++) result[i]=state_address[state_idx]+i*4;
			}
			return result;
			}
		case BITMAP:
			{int mask=0x80000000;
			int bm = c/8;
			int bm_set_left=0;
			bool bm_set=bitmap[state_idx] & (mask >> bm);
			for (int i=0;i<=bm;i++) if (bitmap[state_idx] & (mask>>i)) bm_set_left++;
			int bm_set_left_word = (bm_set_left%4==0) ? bm_set_left/4 : (bm_set_left/4)+1; 
			int total_bm_set=0;
			for (int i=0;i<CSIZE/8;i++) if (bitmap[state_idx] & (mask>>i)) total_bm_set++;
			int total_bm_set_word = (total_bm_set%4==0) ? total_bm_set/4 : (total_bm_set/4)+1;
			int left_lab_tx=0; bool found=false;
			FOREACH_TXLIST(labeled_tr[state],it){
				if ((*it).first<c) left_lab_tx++;
				if ((*it).first==c) found=true;
			}
			if (bm_set==0){
				*size=2;
				result=new int[2];
				result[0]=state_address[state_idx];	//1st level bm
				result[1]=state_address[state_idx]+(1+total_bm_set_word)*4;//fp address
			}else{
				*size=2+bm_set_left_word;
				result=new int[2+bm_set_left_word]; //BM  1st + BMs 2nd + fp/transition
				result[0]=state_address[state_idx];	//1st level bm
				for (int i=1;i<=bm_set_left_word;i++) //2nd level bitmaps
					result[i]=state_address[state_idx]+i*4;
				if  (!found)
					result[1+bm_set_left_word]=state_address[state_idx]+(1+total_bm_set_word)*4;//fp_address
				else	
					result[1+bm_set_left_word]=state_address[state_idx]+(1+total_bm_set_word+1+left_lab_tx)*4;
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
					result[i]=state_address[state_idx]+i*4;
			}else{
				for (int i=0;i<(threshold+1)/4;i++)
					result[i]=state_address[state_idx]+(1+left_lab_tx)*(threshold+1)+i*4;
			}
			return result;
			}
		default:
			fatal("invalid dfas_memory layout");
	}
}

void dfas_memory::set_cache(cache_size_t size,line_size_t line_size, assoc_t assoc){
	if (_cache!=NULL) delete _cache;
	_cache = new cache(size,line_size,assoc);
}

mem_access_t dfas_memory::read(int address){
	if(_cache!=NULL){
		int cache_res=_cache->read(address);
		return ((cache_res==HIT) ? CACHE_HIT : CACHE_MISS);
	}
	if(_onchip_size!=0){
		return ((address<_onchip_size) ? CACHE_HIT : CACHE_MISS); 
	}
	return DIRECT;
}


