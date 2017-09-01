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
 * File:   main_tracegen.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: This is the main file used to generate traversal traces for a given NFA
 * 
 */

#include "stdinc.h"
#include "nfa.h"
#include "parser.h"
#include "trace.h"

int VERBOSE;
int DEBUG;

#ifndef CUR_VER
#define CUR_VER		"\n\tMichela  Becchi 1.3\n"
#endif

/* CONFIGURATION */
#define NUM_TRAFFIC 4
float p_m[NUM_TRAFFIC]={0.35,0.55,0.75,0.95}; //considered probabilities of malicious traffic 

/*
 * Functions restricted to this file
 */
static void usage(void);
static int process_arguments(int, char **);

/*
 * Returns the current version string
 */
void version(){
    printf("version:: %s\n", CUR_VER);
}

static void usage() 
{
	fprintf(stderr,"\n");
    fprintf(stderr, "Usage: regex_tracegen --parse|-p <regex_file> --trace|-t <trace_file_base> [options]\n"); 
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "    --help,-h       		   print this message\n");
    fprintf(stderr, "    --version,-r    		   print version number\n");				    
    fprintf(stderr, "    --verbose,-v    		   basic verbosity level \n");
    fprintf(stderr, "    --debug,  -d    		   enhanced verbosity level \n");
	fprintf(stderr, "    --m,--i         		   m modifier, ignore case\n");
	fprintf(stderr, "    --seed,-s <num_seed>      number of seeds for probabilistic traversal\n");
	fprintf(stderr, "    --mode <depth|size|both>  traversal mode - maximize depth(default)|size|do both\n");
    fprintf(stderr, "\n");
    exit(0);
}

static struct conf {
	char *regex_file;
	char *trace_file_base;
	int verbose;
	int debug;
	bool i_mod;
	bool m_mod;
	int num_seeds;
	bool depth_mode;
	bool size_mode;
} config;

void init_conf(){
	config.regex_file=NULL;
	config.trace_file_base=NULL;
	config.verbose=false;
	config.debug=false;
	config.i_mod=false;
	config.m_mod=false;
	config.num_seeds=1;
	config.depth_mode=true;
	config.size_mode=false;
}

void print_conf(){
	fprintf(stderr,"\nCONFIGURATION: \n");
	if (config.regex_file) fprintf(stderr, "- Reg. Exp. file: %s\n",config.regex_file);
	if (config.trace_file_base) fprintf(stderr, "- Trace base file: %s\n",config.trace_file_base);
	if (config.verbose && !config.debug) fprintf(stderr,"- verbose mode\n");
	if (config.debug) fprintf(stderr,"- debug mode\n");
	if (config.i_mod) fprintf(stderr,"- ignore case selected\n");
	if (config.m_mod) fprintf(stderr,"- m modifier selected\n");
	fprintf(stderr,"- number of seeds: %d\n",config.num_seeds);
	if (config.depth_mode) fprintf(stderr,"- depth mode selected\n");
	if (config.size_mode) fprintf(stderr,"- size mode selected\n");
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
    	}else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--parse") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Regular expression file name missing.\n");
    			return 0;
    		}
    		config.regex_file=argv[i];
    	}else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--seed") == 0){
    	    		i++;
    	    		if(i==argc){
    	    			fprintf(stderr,"Number of seeds is missing.\n");
    	    			return 0;
    	    		}
    	    		config.num_seeds=atoi(argv[i]);
    	    		if (config.num_seeds==0){
    	    			printf("NO seed selected!");
    	    			return 0;
    	    		}
    	}else if(strcmp(argv[i], "--mode") == 0){
    		i++;
    	    if(i==argc){
    	    	fprintf(stderr,"Mode missing.\n");
    	    	return 0;
    	    }
    	    config.depth_mode=false;
    	    config.size_mode=false;
    	    if (strcmp(argv[i], "depth")==0) config.depth_mode=true;
    	    if (strcmp(argv[i], "size")==0) config.size_mode=true;
    	    if (strcmp(argv[i], "both")==0) {
    	    	config.depth_mode=true;
    	    	config.size_mode=true;
    	    }
    	    if (!config.depth_mode && !config.size_mode){
    	    	fprintf(stderr,"No mode selected.\n");
    	    	return 0;
    	    }
    	}else if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Trace file name missing.\n");
    			return 0;
    		}
    		config.trace_file_base=argv[i];		
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

int check_file(char *filename, char *mode){
	FILE *file=fopen(filename,mode);
	if (file==NULL){
		fprintf(stderr,"Unable to open file %s\n",filename);
		return 0;
	}else{
		fclose(file);
		return 1;
	}
}

int main(int argc, char **argv){

	//read configuration
	init_conf();
	while(!parse_arguments(argc,argv) || !config.regex_file || !config.trace_file_base) usage();
	print_conf();
	VERBOSE=config.verbose;
	DEBUG=config.debug; if (DEBUG) VERBOSE=1; 
		
	if (!check_file(config.regex_file,"r")) fatal("Could not open the regex-file");
	
	FILE *re_file=fopen(config.regex_file,"r");
	regex_parser *parser=new regex_parser(config.i_mod, config.m_mod);
	NFA *nfa=parser->parse(re_file);
	fclose(re_file);
	delete parser;
	
	//reduce NFA
	nfa->remove_epsilon();
	nfa->reduce();
	nfa->split_states();
	
	FILE *trace_file=NULL;
	char trace_name[100];
	trace *trace_gen=new trace();
	
	for (int seed=0;seed<config.num_seeds;seed++){
		for (int i=0;i<NUM_TRAFFIC;i++){
			if (config.depth_mode){
				sprintf(trace_name,"%s_depth_s%d_p%.2f.trace",config.trace_file_base,seed,p_m[i]);
				trace_file = trace_gen->generate_trace(nfa, seed, p_m[i], true, trace_name);
				fclose(trace_file);
			}
			if (config.size_mode){
				sprintf(trace_name,"%s_size_s%d_p%.2f.trace",config.trace_file_base,seed,p_m[i]);
				trace_file = trace_gen->generate_trace(nfa, seed, p_m[i], false, trace_name);
				fclose(trace_file);
			}
		}
	}

	delete trace_gen;
	delete nfa;
				
	return 0;

}

