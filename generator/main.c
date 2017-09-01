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
 * File:   main.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: This is the main entry file
 * 
 */

#include "stdinc.h"
#include "nfa.h"
#include "dfa.h"
#include "hybrid_fa.h"
#include "parser.h"
#include "trace.h"

/*
 * Program entry point.
 * Please modify the main() function to add custom code.
 * The options allow to create a DFA from a list of regular expressions.
 * If a single single DFA cannot be created because state explosion occurs, then a list of DFA
 * is generated (see MAX_DFA_SIZE in dfa.h).
 * Additionally, the DFA can be exported in proprietary format for later re-use, and be imported.
 * Moreover, export to DOT format (http://www.graphviz.org/) is possible.
 * Finally, processing a trace file is an option.
 */


#ifndef CUR_VER
#define CUR_VER		"Michela  Becchi 1.3"
#endif

int VERBOSE;
int DEBUG;

/*
 * Returns the current version string
 */
void version(){
    printf("version:: %s\n", CUR_VER);
}

static void usage() 
{
	fprintf(stderr,"\n");
    fprintf(stderr, "Usage: regex [options]\n"); 
    fprintf(stderr, "             [--parse|-p <regex_file> [--m|--i] | --import|-i <in_file> ]\n");
    fprintf(stderr, "             [--export|-e  <out_file>][--graph|-g <dot_file>]\n");
    fprintf(stderr, "             [--trace|-t <trace_file>]\n\n");
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "    --help,-h       print this message\n");
    fprintf(stderr, "    --version,-r    print version number\n");				    
    fprintf(stderr, "    --verbose,-v    basic verbosity level \n");
    fprintf(stderr, "    --debug,  -d    enhanced verbosity level \n");
	fprintf(stderr, "\nOther:\n");
	fprintf(stderr, "    --parse,-p <regex_file>  process regex file\n");
	fprintf(stderr, "    --m,--i  m modifier, ignore case\n");
	fprintf(stderr, "    --import,-i <in_file>    import DFA from file\n");
	fprintf(stderr, "    --export,-e <out_file>   export DFA to file\n");    
	fprintf(stderr, "    --graph,-g <dot_file>    export DFA in DOT format into specified file\n");
    fprintf(stderr, "    --trace,-t <trace_file>  trace file to be processed\n");
    fprintf(stderr, "    --hfa                    generate the hybrid-FA\n");
    fprintf(stderr, "\n");
    exit(0);
}

static struct conf {
	char *regex_file;
	char *in_file;
	char *out_file;
	char *dot_file;
	char *trace_file;
	bool i_mod;
	bool m_mod;
	bool verbose;
	bool debug;
	bool hfa;
} config;

void init_conf(){
	config.regex_file=NULL;
	config.in_file=NULL;
	config.out_file=NULL;
	config.dot_file=NULL;
	config.trace_file=NULL;
	config.i_mod=false;
	config.m_mod=false;
	config.debug=false;
	config.verbose=false;
	config.hfa=false;
}

void print_conf(){
	fprintf(stderr,"\nCONFIGURATION: \n");
	if (config.regex_file) fprintf(stderr, "- RegEx file: %s\n",config.regex_file);
	if (config.in_file) fprintf(stderr, "- DFA import file: %s\n",config.in_file);
	if (config.out_file) fprintf(stderr, "- DFA export file: %s\n",config.out_file);
	if (config.dot_file) fprintf(stderr, "- DOT file: %s\n",config.dot_file);
	if (config.trace_file) fprintf(stderr,"- Trace file: %s\n",config.trace_file);
	if (config.i_mod) fprintf(stderr,"- ignore case selected\n");
	if (config.m_mod) fprintf(stderr,"- m modifier selected\n");
	if (config.verbose && !config.debug) fprintf(stderr,"- verbose mode\n");
	if (config.debug) fprintf(stderr,"- debug mode\n");
	if (config.hfa)   fprintf(stderr,"- hfa generation invoked\n");
}

static int parse_arguments(int argc, char **argv)
{
	int i=1;
    if (argc < 2) {
        usage();
		return 0;
    }
    while(i<argc){
    	if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
    		usage();
    		return 0;
    	}else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--version") == 0){
    		version();
    		return 0;
    	}else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0){
    		config.verbose=1;
    	}else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0){
    		config.debug=1;
    	}else if(strcmp(argv[i], "--hfa") == 0){
    	    		config.hfa=1;	
    	}else if(strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--graph") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Dot file name missing.\n");
    			return 0;
    		}
    		config.dot_file=argv[i];
    	}else if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--import") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Import file name missing.\n");
    			return 0;
    		}
    		config.in_file=argv[i];	
    	}else if(strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--export") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Export file name missing.\n");
    			return 0;
    		}
    		config.out_file=argv[i];
    	}else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--parse") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Regular expression file name missing.\n");
    			return 0;
    		}
    		config.regex_file=argv[i];
    	}else if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Trace file name missing.\n");
    			return 0;
    		}
    		config.trace_file=argv[i];		
    	}else if(strcmp(argv[i], "--m") == 0){
			config.m_mod=true;
		}else if(strcmp(argv[i], "--i") == 0){
			config.i_mod=true;	    		
    	}else{
    		fprintf(stderr,"Ignoring invalid option %s\n",argv[i]);
    	}
    	i++;
    }
	return 1;
}

void check_file(char *filename, char *mode){
	FILE *file=fopen(filename,mode);
	if (file==NULL){
		fprintf(stderr,"Unable to open file %s in %c mode",filename,mode);
		fatal("\n");
	}else fclose(file);
}

int main(int argc, char **argv){
	
	//read configuration
	init_conf();
	while(!parse_arguments(argc,argv)) usage();
	print_conf();
	VERBOSE=config.verbose;
	DEBUG=config.debug; if (DEBUG) VERBOSE=1; 
	
	//check that it is possible to open the files
	if (config.regex_file!=NULL) check_file(config.regex_file,"r");
	if (config.in_file!=NULL) check_file(config.in_file,"r");
	if (config.out_file!=NULL) check_file(config.out_file,"w");
	if (config.dot_file!=NULL) check_file(config.dot_file,"w");
	if (config.trace_file!=NULL) check_file(config.trace_file,"r");
	
	if (config.regex_file==NULL && config.in_file==NULL){
		fatal("No data file - please use either a regex or a DFA import file\n");
	}
	if (config.regex_file!=NULL && config.in_file!=NULL){
		printf("DFA will be imported from the Regex file. Import file will be ignored");
	}
	
	NFA *nfa=NULL;  	// NFA
	DFA *dfa=NULL;		// DFA
	dfa_set *dfas=NULL; // set of DFAs, in case a single DFA for all RegEx in the set is not possible
	HybridFA *hfa=NULL; // Hybrid-FA
	
	if (config.regex_file!=NULL){
		FILE *regex_file=fopen(config.regex_file,"r");
		fprintf(stderr,"\nParsing the regular expression file %s ...\n",config.regex_file);
		regex_parser *parse=new regex_parser(config.i_mod,config.m_mod);
		nfa = parse->parse(regex_file);
		nfa->remove_epsilon();
	 	nfa->reduce();
		dfa=nfa->nfa2dfa();
		if (dfa==NULL) printf("Max DFA size %ld exceeded during creation: the DFA was not generated\n",MAX_DFA_SIZE);
		else dfa->minimize();
		fclose(regex_file);
		delete parse;
	}
	
	if (config.regex_file==NULL && config.in_file!=NULL){
		FILE *in_file=fopen(config.in_file,"r");
		fprintf(stderr,"\nImporting from file %s ...\n",config.in_file);
		dfa=new DFA();
		dfa->get(in_file);
		fclose(in_file);
	}
	
	if (dfa!=NULL && config.out_file!=NULL){
		FILE *out_file=fopen(config.out_file,"w");
		fprintf(stderr,"\nExporting to file %s ...\n",config.out_file);
		dfa->put(out_file);
		fclose(out_file);
		fprintf(stderr,"\nDone ...\n");
	}
	
	if (dfa!=NULL && config.dot_file!=NULL){
		FILE *dot_file=fopen(config.dot_file,"w");
		fprintf(stderr,"\nExporting to DOT file %s ...\n",config.dot_file);
		char string[100];
		if (config.regex_file!=NULL) sprintf(string,"source: %s",config.regex_file);
		else sprintf(string,"source: %s",config.in_file);
		dfa->to_dot(dot_file, string);
		fclose(dot_file);
	}
	
	if (config.hfa){
		if (nfa==NULL) fatal("Impossible to build a Hybrid-FA if no NFA is given.");
		hfa=new HybridFA(nfa);
		if (hfa->get_head()->size()<100000) hfa->minimize();
		printf("HFA:: head size=%d, tail size=%d, number of tails=%d, border size=%d\n",hfa->get_head()->size(),hfa->get_tail_size(),hfa->get_num_tails(),hfa->get_border()->size());
	}

	if (config.trace_file){
		trace *tr=new trace(config.trace_file);
		if (nfa!=NULL) tr->traverse(nfa);
	    if (dfa!=NULL){
			tr->traverse(dfa);	
			if (dfa->get_default_tx()!=NULL) tr->traverse_compressed(dfa);
		}		
		if (hfa!=NULL) tr->traverse(hfa);
		delete tr;
	}
	 
	if (dfa==NULL){
		printf("\nCreating multiple DFAs...\n");
		FILE *re_file=fopen(config.regex_file,"r");
		regex_parser *parser=new regex_parser(config.i_mod,config.m_mod);
		dfas = parser->parse_to_dfa(re_file);
		printf("%d DFAs created\n",dfas->size());
		fclose(re_file);
		delete parser;
		int idx=0;
		FOREACH_DFASET(dfas,it) {
			printf("DFA #%d::  size=%ld\n",idx,(*it)->size());
			if (config.out_file!=NULL){
				char out_file[100];
				sprintf(out_file,"%s%d",config.out_file,idx);
				FILE *file=fopen(out_file,"w");
				fprintf(stderr,"Exporting DFA #%d to file %s ...\n",idx,out_file);
				(*it)->put(file);
				fclose(file);
				idx++;
			}
		}
	}
	
	/*
	 * 
	 * ADD YOUR CODE HERE (see NFA and DFA for compression functions and similar)
	 * 
	 */
	
	/*
	if (nfa!=NULL){
		FILE *dot_file=fopen("examples/nfa.dot","w");
		fprintf(stderr,"\nExporting to DOT file examples/nfa.dot ...\n");
		nfa->to_dot(dot_file, "");
		fclose(dot_file);
	}
	
	if (hfa!=NULL){
		FILE *dot_file=fopen("examples/hfa.dot","w");
		fprintf(stderr,"\nExporting to DOT file examples/hfa.dot ...\n");
		hfa->to_dot(dot_file, "");
		fclose(dot_file);
	}
	*/
	
	if (dfa!=NULL){
		printf("DFA size = %ld\n",dfa->size());
		symbol_t *alph_tx=new symbol_t[CSIZE];
		unsigned alphabet=dfa->alphabet_reduction(alph_tx);
		printf("alphabet size=%d\n",alphabet);
		for (symbol_t c=0;c<CSIZE;c++) if (alph_tx[c]!=0) printf("[%d->%d]",c,alph_tx[c]); 
		printf("\n\n");
		delete [] alph_tx;
	}
	
	/* Automata de-allocation */
	
	if (nfa!=NULL) delete nfa;
	if (dfa!=NULL) delete dfa;
	if (dfas!=NULL){
		FOREACH_DFASET(dfas,it) delete (*it);
		delete dfas;
	}
	if (hfa!=NULL) delete hfa;				
	
	return 0;

}

