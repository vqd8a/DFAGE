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
 * File:   main_ixp.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: This is the main file used to generate the memory layout used by the IXP implementation
 */


#include "stdinc.h"
#include "dfa.h"
#include "comp_dfa.h"
#include "parser.h"
#include "ixp_memory.h"

#ifndef CUR_VER
#define CUR_VER		"Michela  Becchi 1.3"
#endif

int VERBOSE;
int DEBUG;

int main(int argc, char **argv){
	if (argc<3){
		printf("usage:: ./regex NUM_DFAs BASE_DFA_NAME [-d|-v]");
		return -1;
	}
	int num_dfas = atoi(argv[1]);
	char *base_name = argv[2];
	if (argc>3 && (strcmp(argv[3],"-d")==0)) {DEBUG=1;VERBOSE=1;printf("DEBUG selected\n");}
	if (argc>3 && (strcmp(argv[3],"-v")==0)) {VERBOSE=1;printf("VERBOSE selected\n");}
	printf("processing %ld DFAs: %s.dfa#\n\n",num_dfas,base_name);
	
	compDFA **dfas = (compDFA **)allocate_array(sizeof(compDFA*),num_dfas);
	unsigned alphabet;
	symbol_t alphabet_tx[CSIZE];
	for (symbol_t c=0;c<CSIZE;c++) alphabet_tx[c]=0;
	
	printf("computing the alphabet...\n");
	//initialize alphabet
	for (int i=0;i<num_dfas;i++){
		char name[100];
		sprintf(name,"%s.dfa%d",base_name,i);
		printf("processing %s...\n",name);
		FILE *dfa_file=fopen(name,"r");
		if (dfa_file==NULL){
			printf("Could not open %s\n!",name);
			return -1;
		}
		DFA *dfa=new DFA();
		dfa->get(dfa_file);
		alphabet=dfa->alphabet_reduction(alphabet_tx,false);
		delete dfa;
	}
	
	printf("computing the compressed DFAs...\n");
	if (VERBOSE || DEBUG){
		printf("alphabet size=%d\n",alphabet);
		for (symbol_t c=0;c<CSIZE;c++) if (alphabet_tx[c]!=0) printf("[%c->%d]",c,alphabet_tx[c]); 
		printf("\n\n");
	}
	
	//initialize the compressed DFAs
	for (int i=0;i<num_dfas;i++){
		char name[100];
		sprintf(name,"%s.dfa%d",base_name,i);
		printf("processing %s...\n",name);
		FILE *dfa_file=fopen(name,"r");
		if (dfa_file==NULL){
			printf("Could not open %s\n!",name);
			return -1;
		}
		DFA *dfa=new DFA();
		dfa->get(dfa_file);
		dfas[i]=new compDFA(dfa,alphabet,alphabet_tx);
		delete dfa;
	}
	
	//create the IXP memory
	printf("creating the IXP memory...\n");
	ixp_memory *ixp_mem= new ixp_memory(num_dfas, dfas, alphabet, alphabet_tx, "ixp/dfa_scratch.map", "ixp/dfa_sram.map", "ixp/dfa_dram.map");						             
	
	//de-allocate DFAs
	for (int i=0;i<num_dfas;i++) delete dfas[i];
	free(dfas);
	
	delete ixp_mem;
	
	return 0;
}

