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
 * File:   main_ixp_nfa.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: This is the main file used to generate the memory layout used by the IXP implementation
 */


#include "stdinc.h"
#include "nfa.h"
#include "comp_nfa.h"
#include "parser.h"
#include "ixp_memory.h"

#ifndef CUR_VER
#define CUR_VER		"Michela  Becchi 1.3"
#endif

int VERBOSE;
int DEBUG;

int main(int argc, char **argv){
	if (argc<2){
		printf("usage:: ./regex NFA_FILE_NAME [-d|-v] [--hfa]");
		return -1;
	}
	
	bool do_hfa=false;
	char *filename = argv[1];
	if (argc>2 && (strcmp(argv[2],"-d")==0)) {DEBUG=1;VERBOSE=1;printf("DEBUG selected\n");}
	if (argc>2 && (strcmp(argv[2],"-v")==0)) {VERBOSE=1;printf("VERBOSE selected\n");}
	if (argc>3 && (strcmp(argv[3],"--hfa")==0)) do_hfa=true;
	
	printf("processing RegEx file: %s\n\n",filename);
	FILE *regex_file=fopen(filename,"r");
	if (regex_file==NULL) fatal("Cannot open the regex file");
	
	fprintf(stdout,"\nParsing the regular expression file %s ...\n",regex_file);
	regex_parser *parser=new regex_parser(true,true);
	NFA *nfa = parser->parse(regex_file);
	nfa->remove_epsilon();
	nfa->reduce();
	delete parser;
	
	compNFA *comp_nfa = NULL;
	HybridFA *hfa = NULL;
	
	if (do_hfa) hfa=new HybridFA(nfa);
	else{
		comp_nfa = new compNFA(nfa);
		printf("alphabet size = %d\n",comp_nfa->alphabet_size);
		for (symbol_t c=0;c<CSIZE;c++) if (comp_nfa->alphabet_tx[c]) printf("[%c->%d]",c,comp_nfa->alphabet_tx[c]);
		printf("\n");
	}
	
	//create the IXP memory
	printf("creating the IXP memory...\n");
	ixp_memory *ixp_mem = NULL;
	if (!do_hfa) ixp_mem = new ixp_memory(comp_nfa, "ixp/nfa_scratch.map", "ixp/nfa_sram.map", "ixp/nfa_dram.map");
	else ixp_mem = new ixp_memory(hfa, "ixp/hfa_scratch.map", "ixp/hfa_sram.map", "ixp/hfa_dram.map");
	
	//de-allocate 
	delete ixp_mem;
	delete nfa;
	if (comp_nfa!=NULL) delete comp_nfa;
	if (hfa!=NULL) delete hfa;
	
	return 0;
}

