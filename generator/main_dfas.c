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
 * File:   main_dfas.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 * Description: This is the main file used to traverse a set of DFAs with given traffic traces and produce
 * 				traversal statistics. The DFAs are layout in the same memory.
 * 				It is assumed that the traffic traces are generated through the "regex_tracegen" tool.
 */


#include "stdinc.h"
#include "nfa.h"
#include "dfa.h"
#include "parser.h"
#include "trace.h"
#include "dfas_memory.h"

int VERBOSE;
int DEBUG;

#ifndef CUR_VER
#define CUR_VER		"\n\tMichela  Becchi 1.3\n"
#endif

/* CONFIGURATION */
#define NUM_MEM_LAYOUT  4
#define NUM_TRAFFIC     4
#define NUM_CACHE_SIZES 4
#define NUM_CACHE_LINES 1
#define NUM_CACHE_WAYS  1
#define NUM_STATISTICS  9 // do not change this value!

float p_m[NUM_TRAFFIC]={0.35,0.55,0.75,0.95};                    //considered probabilities of malicious traffic
cache_size_t cache_size[NUM_CACHE_SIZES] = {K4, K16, K64, K256}; //considered cache sizes
int cache_line[NUM_CACHE_LINES] = {64};                          //considered cache-line sizes
assoc_t cache_ways[NUM_CACHE_WAYS] = {DM};                       //considered cache set associativities                              

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
    fprintf(stderr, "Usage: regex_nfa --number|-n <num_DFAs> --input|-i <DFA_base> --trace|-t <trace_file_base> [options]\n"); 
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "    --help,-h       		   print this message\n");
    fprintf(stderr, "    --version,-r    		   print version number\n");				    
    fprintf(stderr, "    --verbose,-v    		   basic verbosity level \n");
    fprintf(stderr, "    --debug,  -d    		   enhanced verbosity level \n");
	fprintf(stderr, "    --m,--i         		   m modifier, ignore case\n");
	fprintf(stderr, "    --seed,-s <num_seed>      number of seeds for probabilistic traversal\n");
	fprintf(stderr, "    --log,-l <log_file>       log file\n");
    fprintf(stderr, "\n");
    exit(0);
}

static struct conf {
	int num_dfas;
	char *dfa_base_file;
	char *trace_file_base;
	char *log_file;
	int verbose;
	int debug;
	bool i_mod;
	bool m_mod;
	int num_seeds;	
} config;

void init_conf(){
	config.num_dfas=0;
	config.dfa_base_file=NULL;
	config.trace_file_base=NULL;
	config.log_file=NULL;
	config.verbose=false;
	config.debug=false;
	config.i_mod=false;
	config.m_mod=false;
	config.num_seeds=1;
}

void print_conf(){
	fprintf(stderr,"\nCONFIGURATION: \n");
	printf("- Number of DFAs: %d\n",config.num_dfas);
	if (config.dfa_base_file) fprintf(stderr, "- DFA base file: %s\n",config.dfa_base_file);
	if (config.trace_file_base) fprintf(stderr, "- Trace base file: %s\n",config.trace_file_base);
	if (config.log_file) fprintf(stderr, "- Log file: %s\n",config.log_file);
	if (config.verbose && !config.debug) fprintf(stderr,"- verbose mode\n");
	if (config.debug) fprintf(stderr,"- debug mode\n");
	if (config.i_mod) fprintf(stderr,"- ignore case selected\n");
	if (config.m_mod) fprintf(stderr,"- m modifier selected\n");
	fprintf(stderr,"- number of seeds: %d\n",config.num_seeds);
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
    	}else if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"DFA base file name missing.\n");
    			return 0;
    		}
    		config.dfa_base_file=argv[i];
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
    	}else if(strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--number") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Number of DFAs is missing.\n");
    			return 0;
    		}
    		config.num_dfas=atoi(argv[i]);
    		if (config.num_dfas==0){
    			printf("NO DFAs selected!");
    			return 0;
    		}	
    	}else if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Trace file name missing.\n");
    			return 0;
    		}
    		config.trace_file_base=argv[i];
    	}else if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0){
    		i++;
    		if(i==argc){
    			fprintf(stderr,"Log file name missing.\n");
    			return 0;
    		}
    		config.log_file=argv[i];			
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
	while(!parse_arguments(argc,argv) || config.num_dfas==0 || !config.dfa_base_file || !config.trace_file_base) usage();
	print_conf();
	VERBOSE=config.verbose;
	DEBUG=config.debug; if (DEBUG) VERBOSE=1; 

	//log file
	FILE *log=NULL;
	if (config.log_file) log=fopen(config.log_file,"w");
	if (log==NULL){
		printf("Log to stdout\n");
		log=stdout;
	}
	
	//stats file
	FILE *stats_file=NULL;
	char stats_file_name[100];
	sprintf(stats_file_name,"%s_dfa.max",config.trace_file_base);
	stats_file=fopen(stats_file_name,"w");
	if (stats_file==NULL) fatal("cannot open stats.max file\n");
	else fclose(stats_file);
	sprintf(stats_file_name,"%s_dfa.avg",config.trace_file_base);
	stats_file=fopen(stats_file_name,"w");
	if (stats_file==NULL) fatal("cannot open stats.avg file\n");
	else fclose(stats_file);
	
	
	/* DFAs */
	DFA **dfas=(DFA**)malloc(config.num_dfas*sizeof(DFA*));
	unsigned size=0;
	unsigned tx=0;
	for (unsigned idx=0;idx<config.num_dfas;idx++){
		char filename[100];
		sprintf(filename,"%s%d",config.dfa_base_file,idx);
		FILE *file=fopen(filename,"r");
		if (file==NULL) fatal("error in opening DFA file");
		else fprintf(stderr,"\nImporting from file %s ...\n",filename);
		dfas[idx]=new DFA();
		dfas[idx]->get(file);
		fclose(file);
		printf("DFA #%d imported - size %ld\n",idx,dfas[idx]->size());
		size+=dfas[idx]->size();
		for (state_t s=0;s<dfas[idx]->size();s++) tx+=dfas[idx]->get_labeled_tx()[s]->size();
	}
	printf("\nDFAs:: states: %ld, tx: %ld\n\n",size,tx);
	
	//check that trace files are available
	char trace_name[100];
	for (int seed=0;seed<config.num_seeds;seed++){
		for (int i=0;i<NUM_TRAFFIC;i++){
			sprintf(trace_name,"%s_s%d_p%.2f.trace",config.trace_file_base,seed,p_m[i]);
			if (!check_file(trace_name,"r")){
				printf("Cannot open file %s\n",trace_name); 
				exit(-1);
			}
		}
	}
			
	double current_data[NUM_STATISTICS]; //current traversal statistics 
	for (int h=0;h<NUM_STATISTICS;h++) current_data[h]=0;
	
	double max_data[NUM_MEM_LAYOUT][NUM_TRAFFIC][NUM_CACHE_SIZES][NUM_CACHE_LINES][NUM_CACHE_WAYS][NUM_STATISTICS];	//mem_layout, p_w, cache_size, cache_assoc, data
	double avg_data[NUM_MEM_LAYOUT][NUM_TRAFFIC][NUM_CACHE_SIZES][NUM_CACHE_LINES][NUM_CACHE_WAYS][NUM_STATISTICS];	//mem_layout, p_w, cache_size, cache_assoc, data
	
	for (int l=0;l<NUM_MEM_LAYOUT;l++)
		for (int i=0;i<NUM_TRAFFIC;i++)
			for (int j=0;j<NUM_CACHE_SIZES;j++)
				for (int k=0;k<NUM_CACHE_LINES;k++)
					for (int h=0;h<NUM_CACHE_WAYS;h++)
						for (int s=0;s<NUM_STATISTICS;s++){
							max_data[l][i][j][k][h][s]=0;
							avg_data[l][i][j][k][h][s]=0;
						}
	
	
	dfas_memory *mem;		
	trace *trace_handler = new trace();
	
	for (int layout=0;layout<NUM_MEM_LAYOUT;layout++){
	
		//instantiate the memory
		if (layout==0) 		mem=new dfas_memory(config.num_dfas,dfas,LINEAR,50);
		else if (layout==1) mem=new dfas_memory(config.num_dfas,dfas,BITMAP,50);
		else if (layout==2) mem=new dfas_memory(config.num_dfas,dfas,INDIRECT_ADDR,3);
		else 				mem=new dfas_memory(config.num_dfas,dfas,INDIRECT_ADDR,7);
		mem->init(NULL);
		
		for(int cs=0;cs<NUM_CACHE_SIZES;cs++){
			for (int cl=0;cl<NUM_CACHE_LINES;cl++){
				for (int cw=0;cw<NUM_CACHE_WAYS;cw++){
					//cache
					mem->set_cache(cache_size[cs],cache_line[cl],cache_ways[cw]);
					mem->get_cache()->debug(log,false);
					//traffic
					for (int i=0;i<NUM_TRAFFIC;i++){
						//seed
						for (int seed=0;seed<config.num_seeds;seed++){
							sprintf(trace_name,"%s_s%d_p%.2f.trace",config.trace_file_base,seed,p_m[i]);
							trace_handler->set_trace(trace_name);
							
							//traversal
							trace_handler->traverse(mem,current_data);
							
							//log and updata max_data/avg_data
							fprintf(log,"===> ");
							fprintf(log,"%d ",layout);
							fprintf(log,"%f ",p_m[i]);
							fprintf(log,"%d ",seed);
							fprintf(log,"%dB ",cache_size[cs]);
							fprintf(log,"%d ", cache_line[cl]);
							fprintf(log,"%d-way ",cache_ways[cw]);
							for (int m=0;m<NUM_STATISTICS;m++){
								fprintf(log,"%.2f ",current_data[m]);
								if (current_data[m] > max_data[layout][i][cs][cl][cw][m]) max_data[layout][i][cs][cl][cw][m] = current_data[m];
								avg_data[layout][i][cs][cl][cw][m]+=current_data[m];
							}
							fprintf(log,"\n");
							fflush(log);
							
							//clean the cache					
							mem->get_cache()->clean();
							
						}//end seed
					} //end traffic
				} //end cw
			}// end cl
		} //end cs
				
		delete mem;
		
		if (layout==0) 		fprintf(stderr,"Note:: linear layout finished\n");
		else if (layout==1) fprintf(stderr,"Note:: bitmapped layout finished\n");
		else if (layout==2) fprintf(stderr,"Note:: indirect addr/32 bit layout finished\n");
		else 				fprintf(stderr,"Note:: indirect addr/64 bit layout finished\n");
	
	} //end layout
	
	//correct average statistics
	for (int l=0;l<NUM_MEM_LAYOUT;l++)
		for (int i=0;i<NUM_TRAFFIC;i++)
			for (int j=0;j<NUM_CACHE_SIZES;j++)
				for (int k=0;k<NUM_CACHE_LINES;k++)
					for (int h=0;h<NUM_CACHE_WAYS;h++)
						for (int s=0;s<NUM_STATISTICS;s++){						
							avg_data[l][i][j][k][h][s]= avg_data[l][i][j][k][h][s]/config.num_seeds;
						}
	
	//MAX
	sprintf(stats_file_name,"%s_dfa.max",config.trace_file_base);
	stats_file=fopen(stats_file_name,"w");
	
	for (int cl=0;cl<NUM_CACHE_LINES;cl++){
		for (int cw=0;cw<NUM_CACHE_WAYS;cw++){
			for (int l=0;l<NUM_MEM_LAYOUT;l++){
				for (int i=0;i<NUM_TRAFFIC;i++){
					for (int cs=0;cs<NUM_CACHE_SIZES;cs++){
						if (l==0) fprintf(stats_file,"LINEAR "); 
						else if (l==1) fprintf(stats_file,"BITMAP "); 
						else if (l==2) fprintf(stats_file,"IND_ADDR/32bit "); 
						else fprintf(stats_file,"IND_ADDR/64bit ");  
						fprintf(stats_file,"%.2f ",p_m[i]);
						fprintf(stats_file,"%dB ",cache_size[cs]);
						fprintf(stats_file,"%d-line ",cache_line[cl]);
						fprintf(stats_file,"%d-way ",cache_ways[cw]);
						for (int m=0;m<NUM_STATISTICS;m++){
							fprintf(stats_file,"%.2f ",max_data[l][i][cs][cl][cw][m]);	
						}
						fprintf(stats_file,"\n");
					}
				}
			}
		}
	}
	fprintf(stats_file,"\n");
	
	for (int cl=0;cl<NUM_CACHE_LINES;cl++){
		for (int cw=0;cw<NUM_CACHE_WAYS;cw++){
			for (int l=0;l<NUM_MEM_LAYOUT;l++){
				for (int cs=0;cs<NUM_CACHE_SIZES;cs++){
					for (int i=0;i<NUM_TRAFFIC;i++){
						if (l==0) fprintf(stats_file,"LINEAR "); 
						else if (l==1) fprintf(stats_file,"BITMAP "); 
						else if (l==2) fprintf(stats_file,"IND_ADDR/32bit "); 
						else fprintf(stats_file,"IND_ADDR/64bit ");  
						fprintf(stats_file,"%.2f ",p_m[i]);
						fprintf(stats_file,"%dB ",cache_size[cs]);
						fprintf(stats_file,"%d-line ",cache_line[cl]);
						fprintf(stats_file,"%d-way ",cache_ways[cw]);
						for (int m=0;m<NUM_STATISTICS;m++){
							fprintf(stats_file,"%.2f ",max_data[l][i][cs][cl][cw][m]);	
						}
						fprintf(stats_file,"\n");
					}
				}
			}
		}
	}
	fprintf(stats_file,"\n");
	fclose(stats_file);
	
	//AVG
	sprintf(stats_file_name,"%s_dfa.avg",config.trace_file_base);
	stats_file=fopen(stats_file_name,"w");
	
	for (int cl=0;cl<NUM_CACHE_LINES;cl++){
		for (int cw=0;cw<NUM_CACHE_WAYS;cw++){
			for (int l=0;l<NUM_MEM_LAYOUT;l++){
				for (int i=0;i<NUM_TRAFFIC;i++){
					for (int cs=0;cs<NUM_CACHE_SIZES;cs++){
						if (l==0) fprintf(stats_file,"LINEAR "); 
						else if (l==1) fprintf(stats_file,"BITMAP "); 
						else if (l==2) fprintf(stats_file,"IND_ADDR/32bit "); 
						else fprintf(stats_file,"IND_ADDR/64bit ");  
						fprintf(stats_file,"%.2f ",p_m[i]);
						fprintf(stats_file,"%dB ",cache_size[cs]);
						fprintf(stats_file,"%d-line ",cache_line[cl]);
						fprintf(stats_file,"%d-way ",cache_ways[cw]);
						for (int m=0;m<NUM_STATISTICS;m++){
							fprintf(stats_file,"%.2f ",avg_data[l][i][cs][cl][cw][m]);	
						}
						fprintf(stats_file,"\n");
					}
				}
			}
		}
	}
	fprintf(stats_file,"\n");
	
	for (int cl=0;cl<NUM_CACHE_LINES;cl++){
		for (int cw=0;cw<NUM_CACHE_WAYS;cw++){
			for (int l=0;l<NUM_MEM_LAYOUT;l++){
				for (int cs=0;cs<NUM_CACHE_SIZES;cs++){
					for (int i=0;i<NUM_TRAFFIC;i++){
						if (l==0) fprintf(stats_file,"LINEAR "); 
						else if (l==1) fprintf(stats_file,"BITMAP "); 
						else if (l==2) fprintf(stats_file,"IND_ADDR/32bit "); 
						else fprintf(stats_file,"IND_ADDR/64bit ");  
						fprintf(stats_file,"%.2f ",p_m[i]);
						fprintf(stats_file,"%dB ",cache_size[cs]);
						fprintf(stats_file,"%d-line ",cache_line[cl]);
						fprintf(stats_file,"%d-way ",cache_ways[cw]);
						for (int m=0;m<NUM_STATISTICS;m++){
							fprintf(stats_file,"%.2f ",avg_data[l][i][cs][cl][cw][m]);	
						}
						fprintf(stats_file,"\n");
					}
				}
			}
		}
	}
	fprintf(stats_file,"\n");
	fclose(stats_file);

	//de-allocate
	delete trace_handler;
	for (int idx=0;idx<config.num_dfas;idx++) delete dfas[idx];
	free(dfas);
				
	return 0;

}

