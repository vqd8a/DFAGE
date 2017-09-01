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



#include "stdinc.h"
#include <sys/time.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "tcp.h"

#ifndef CUR_VER
#define CUR_VER		"Michela  Becchi 1.3"
#endif

int VERBOSE;
int DEBUG;


typedef struct{
	int thread_id;
	bool done;
	char *regex_file;
	char *trace_file;
} thread_par;

void *receive_and_process(void *par){
    
	struct timeval start,end;
	char command[100];
	
    gettimeofday(&start,NULL);
    for (int i=1;i<=6;i++){
	    sprintf(command, "pcregrep -i -M -f %s%d %s", ((thread_par *)par)->regex_file,i,((thread_par *)par)->trace_file);
	    printf(command);printf("\n");
	    system(command);
    }
    	         
    gettimeofday(&end,NULL);    
    unsigned r_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);    	  
    printf("P%d:: exec-time = %ld sec. %ld usec.\n", ((thread_par *)par)->thread_id, r_time/1000000,r_time%1000000);
    
    ((thread_par *)par)->done=true;
    if (((thread_par *)par)->thread_id!=0) pthread_exit(NULL);
}

void load_servers(int num_servers,char *trace_file, char *regex_file, FILE *log)
{	
    struct timeval start_time,end_time;          
    pthread_t threads[num_servers];
    thread_par params[num_servers];
    for (int i=0;i<num_servers;i++){
    	params[i].thread_id=i;
    	params[i].done=false;
    	params[i].regex_file=regex_file;
    	params[i].trace_file=trace_file;
    }
    
    gettimeofday(&start_time,NULL);
         
    for (int i=1;i<num_servers;i++)
    	pthread_create(&threads[i], NULL, receive_and_process, (void *)(&params[i]));
        
    (*receive_and_process)(&params[0]);
        
     while (1){
    	int i;
    	for (i=0;i<num_servers;i++){
    		if (!params[i].done) break;
    	}
        if (i==num_servers) break;
     }

     gettimeofday(&end_time,NULL);    
     unsigned r_time = (end_time.tv_sec-start_time.tv_sec)*1000000+(end_time.tv_usec-start_time.tv_usec);    	  
     printf("tot-time = %ld = %ld sec. %ld usec.\n", r_time, r_time/1000000,r_time%1000000);
     fprintf(log,"TOT_TIME %ld\n", r_time);	       	        
     fflush(log);
        
     for (int i=0;i<num_servers;i++) params[i].done=false;       
     pthread_exit(NULL);
} 


int main(int argc, char **argv){
	if (argc<7){
		printf("usage:: ./pcre_threads -n NUM_PCREs -f REGEX_FILE -t TRACE_FILE [-server NUM_SERVERS][-l LOG_FILE ]");
		return -1;
	}
	
	char *regex_file=NULL;
	char *trace_file=NULL;
	FILE *log_file=NULL;
	int num_servers=1;
	int num_pcres=0;
	char **pcre;
	
	int i=1;
	while (i<argc){
		if (strcmp(argv[i],"-server")==0) {
			if ((++i)==argc) fatal("number of servers missing");
			num_servers=atoi(argv[i]);
		}
		else if (strcmp(argv[i],"-n")==0) {
			if ((++i)==argc) fatal("number of pcre missing");
			num_pcres=atoi(argv[i]);
		}
		else if (strcmp(argv[i],"-t")==0) {
			if ((++i) == argc) fatal("trace filename missing");
			else trace_file=argv[i];				
		}
		else if (strcmp(argv[i],"-f")==0) {
			if ((++i) == argc) fatal("regex filename missing");
			else regex_file=argv[i];				
		}
		else if (strcmp(argv[i],"-l")==0) {
			if ((++i) == argc) fatal("log filename missing");
			else{
				log_file=fopen(argv[i],"a");
				if (log_file==NULL) fatal ("cannot open log file");
				else printf("log file: %s\n",argv[i]);
			}	
		}
		i++;
	}
	
	FILE *regex = fopen(regex_file,"r");
	if (regex=NULL) fatal("invalid regex file");
	pcre = (char **) malloc(num_pcres*sizeof(char));
	//parsing the RegEx and putting them in a NFA
	unsigned int c=fgetc(file);
	int i=0;
	while(c!=EOF){
			if (c=='\n' || c=='\r'){
				if(i!=0){
					re[i]='\0';
					if (re[0]!='#'){
						j++;
						if (j>=from && (to==-1 || j<=to)){
							if (DEBUG) fprintf(stdout,"\n%d) processing regex:: <%s> ...\n",j,re);
							parse_re(nfa, re);
						}
					} 
					i=0;
					free(re);
					re=allocate_char_array(1000);
				}
			}else{
				re[i++]=c;
			}	
			c=fgetc(file);
	} //end while
	
	load_servers(num_servers,trace_file, regex_file, log_file);
		
	return 0;
}

