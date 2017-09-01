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
 * File:   main_memory.c
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
#include "memory.h"
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

#define M_DFA 0
#define M_NFA 1
#define M_HFA 2

unsigned load_trace(FILE *trace, char **data){
	if (DEBUG) printf("main_memory:: load_trace: loading trace file to memory...\n");
	unsigned filesize=0;
	int c=fgetc(trace);
	while (c!=EOF) {
		filesize++;
		c=fgetc(trace);
	}
	rewind(trace);
	(*data)=new char[filesize];
	for (unsigned i=0;i<filesize;i++) (*data)[i]=(char)fgetc(trace);		
	if (DEBUG) printf("main_memory:: load_trace: %d bytes loaded from tracefile\n",filesize);
	return filesize;
}

typedef struct{
	int thread_id;
	int socket_id;
	int mode;
	bool done;
	memory *mem;
	FILE *log;
} thread_par;

void *receive_and_process(void *par){
    
	int index=0;
	int bytes_received;
    char *trace = new char[MAX_TRACESIZE];
    struct timeval start,end;
 
    gettimeofday(&start,NULL);
    
    while (1)
    {                                       
      
      //send on connected socket the data. the last flag is 0 by default (MSG_00B for high priority, MSG_PEEK for look message without removing)
      //returns number of bytes sent and received if error
      bytes_received = recv(((thread_par *)par)->socket_id,trace+index,PACKET_SIZE,0);
                  
      index+=bytes_received;
      
#ifdef LOGGING              
      printf("P%d:: %d data received, %d total byte received\n", ((thread_par *)par)->thread_id, bytes_received, index);
#endif
      
      if (bytes_received==0){
    	  printf("P%d:: %d total byte received\n", ((thread_par *)par)->thread_id, index);
    	  gettimeofday(&end,NULL);
    	  unsigned r_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);    	  
    	  printf("P%d:: read-time = %ld sec. %ld usec.\n", ((thread_par *)par)->thread_id, r_time/1000000,r_time%1000000);
    	  if (((thread_par *)par)->mode==M_DFA) 		(((thread_par *)par)->mem)->traverse_dfa(index,trace,((thread_par *)par)->log);
    	  else if (((thread_par *)par)->mode==M_NFA) (((thread_par *)par)->mem)->traverse_nfa(index,trace,((thread_par *)par)->log);
    	  else if (((thread_par *)par)->mode==M_HFA) (((thread_par *)par)->mem)->traverse_hfa(index,trace,((thread_par *)par)->log);
    	  else fatal ("Invalid Mode");    	  
    	  break;
      }              
    }          
    gettimeofday(&end,NULL);    
    unsigned r_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);    	  
    printf("P%d:: exec-time = %ld sec. %ld usec.\n", ((thread_par *)par)->thread_id, r_time/1000000,r_time%1000000);
    
    delete [] trace;
    ((thread_par *)par)->done=true;
    if (((thread_par *)par)->thread_id!=0) pthread_exit(NULL);
}

void load_servers(int num_servers,int mode, memory *mem, FILE *log)
{
		int sock;
        int connected, true_p = 1;
        struct timeval start_time,end_time;          
        pthread_t threads[num_servers];
        thread_par params[num_servers];
        for (int i=0;i<num_servers;i++){
        	params[i].thread_id=i;
        	params[i].mode=mode;
        	params[i].mem=mem;
        	params[i].log=log;
        	params[i].done=false;
        }
        
        struct sockaddr_in server_addr,client_addr;    
        socklen_t sin_size;
        
        //address of the server to which other processes can connect
        server_addr.sin_family = AF_INET;         	//standard
       	server_addr.sin_port = htons(CONN_PORT);    //port 
       	server_addr.sin_addr.s_addr = INADDR_ANY;   //IP address
       	bzero(&(server_addr.sin_zero),8); 
       	
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) { //create the socket
            perror("Socket");								  //domain=AF_INET (address+port number)
            exit(1);									      //type=SOCK_STREAM (virtual circuit for streams)
        }													  //protocol=0 (standard)

        if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true_p,sizeof(int)) == -1) {
            perror("Setsockopt");
            exit(1);
        }
                       
        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) { //give the address of the socket to the server
            perror("Unable to bind");
            exit(1);
        }

        if (listen(sock, MAX_CONN_REQ) == -1) { //max number of connection requests that can be pending 
            perror("Listen");
            exit(1);
        }        
        
        sin_size = sizeof(struct sockaddr_in);
        
        while (1){
		
	        printf("\nTCPServer Waiting for client on port %d\n",CONN_PORT);
	        fflush(stdout);              
	        
	        int i=0;
	        
	        while (i<num_servers){
	        	params[i++].socket_id = accept(sock, (struct sockaddr *)&client_addr,&sin_size);
	        	printf("Received connection from (%s , %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));        	    
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
        }
        
        close(sock);        
        pthread_exit(NULL);
} 


int main(int argc, char **argv){
	if (argc<2){
		printf("usage:: ./regex -dfa|-nfa|-hfa [-f REGEX_FILE] [-n NUM_DFAs] [-d|-v] [-t TRACE_FILE] [-e EXPORT_FILE] [-i IMPORT_FILE] [-server NUM_SERVERS]");
		return -1;
	}
	int mode = -1;
	int num_dfas = 0; 		
	char *base_name = NULL; 
	FILE *trace = NULL;
	FILE *log = stdout;
	FILE *export_file = NULL;
	FILE *import_file = NULL;
	int num_servers=0;
	
	int i=1;
	while (i<argc){
		if (strcmp(argv[i],"-dfa")==0){
			mode=M_DFA;
		}else if (strcmp(argv[i],"-nfa")==0){
			mode=M_NFA;
		}else if (strcmp(argv[i],"-hfa")==0){
			mode=M_HFA;
		}else if (strcmp(argv[i],"-f")==0){
			if ((++i)==argc) fatal("regex file missing");
			base_name=argv[i];
		}else if (strcmp(argv[i],"-n")==0){
			if ((++i)==argc) fatal("number of DFAs missing");
			num_dfas=atoi(argv[i]);
		}else if (strcmp(argv[i],"-server")==0) {
			if ((++i)==argc) fatal("number of servers missing");
			num_servers=atoi(argv[i]);
	}else if (strcmp(argv[i],"-d")==0) {DEBUG=1;VERBOSE=1;printf("DEBUG selected\n");}
		else if (strcmp(argv[i],"-v")==0) {VERBOSE=1;printf("VERBOSE selected\n");}
		else if (strcmp(argv[i],"-t")==0) {
			if ((++i) == argc) fatal("trace filename missing");
			else{
				trace=fopen(argv[i],"r");
				if (trace==NULL) fatal ("unexisting trace file");
				else printf("trace file: %s\n",argv[i]);
			}
		}else if (strcmp(argv[i],"-l")==0) {
			if ((++i) == argc) fatal("log filename missing");
			else{
				log=fopen(argv[i],"a");
				if (log==NULL) fatal ("cannot open log file");
				else printf("log file: %s\n",argv[i]);
			}	
		}else if (strcmp(argv[i],"-e")==0) {
			if ((++i) == argc) fatal("export filename missing");
			else{
				export_file=fopen(argv[i],"w");
				if (export_file==NULL) fatal ("cannot create export-file");
				else printf("export file: %s\n",argv[i]);
			}
		}else if (strcmp(argv[i],"-i")==0) {
			if ((++i) == argc) fatal("import filename missing");
			else{
				import_file=fopen(argv[i],"r");
				if (import_file==NULL) fatal ("cannot create import-file");
				else printf("import file: %s\n",argv[i]);
			}
		}
		i++;
	}
	
	if (base_name==0 && import_file==0) fatal("data file for FA missing");
	if (mode==M_DFA){
		if (import_file==0 && num_dfas==0) fatal("0 DFAs selected!");
		printf("DFA selected: dfa-file %s, #=%d\n",base_name,num_dfas);
	}else if (mode==M_NFA){
		printf("NFA selected: regex-file %s\n",base_name);
	}else if (mode==M_HFA){
		printf("Hybrid-FA selected: regex-file %s\n",base_name);
	}else{
		fatal("invalid mode");
	}
	
	/* DFA */
	
	if (mode==M_DFA){
		
		memory *mem=NULL;
		
		if (base_name!=NULL){
			
			compDFA **dfas = (compDFA **)allocate_array(sizeof(compDFA*),num_dfas);
			unsigned alphabet;
			symbol_t alphabet_tx[CSIZE];
			for (symbol_t c=0;c<CSIZE;c++) alphabet_tx[c]=0;
			
			printf("computing the alphabet...\n");
			//initialize alphabet
			for (int i=0;i<num_dfas;i++){
				char name[100];
				sprintf(name,"%s%d",base_name,i);
				printf("processing %s...\n",name);
				FILE *dfa_file=fopen(name,"r");
				if (dfa_file==NULL){
					printf("Could not open %s\n!",name);
					return -1;
				}
				DFA *dfa=new DFA();
				dfa->get(dfa_file);
				if (DEBUG){
					printf("main_memory (DFA):: computing alphabet size on DFA #%d of size %d\n",i,dfa->size());
					fflush(stdout);
				}
				alphabet=dfa->alphabet_reduction(alphabet_tx,false);
				delete dfa;
				fclose(dfa_file);
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
				sprintf(name,"%s%d",base_name,i);
				printf("processing %s...\n",name);
				FILE *dfa_file=fopen(name,"r");
				if (dfa_file==NULL){
					printf("Could not open %s\n!",name);
					return -1;
				}
				DFA *dfa=new DFA();
				dfa->get(dfa_file);
				fclose(dfa_file);
				if (DEBUG){
					printf("main_memory (DFA):: computing the compressed-DFA for DFA #%d of size %d\n",i,dfa->size());
					fflush(stdout);
				}
				dfas[i]=new compDFA(dfa,alphabet,alphabet_tx);
				delete dfa;
			}
			
			//create the memory layout
			printf("creating the memory layout...\n");
			mem= new memory(dfas, num_dfas);						             
			
			//de-allocate DFAs
			for (int i=0;i<num_dfas;i++) delete dfas[i];
			free(dfas);
		
		}else{
			printf("importing memory content from file...\n");
			mem=new memory();
			mem->get(import_file);
		}
		
		//export
		if (export_file!=NULL) mem->put(export_file);
		
		
		//process trace file
		if (trace!=NULL) {
			char *data;
			int trace_size = load_trace(trace,&data);
			mem->traverse_dfa(trace_size,data,log);
			delete [] data;
		}
		
		//server mode
		if (num_servers){
			load_servers(num_servers,M_DFA,mem,log);
		}
		
		delete mem;
		
	} //end DFA
	
	/* NFA */
	
	if (mode==M_NFA){
		
		memory *mem=NULL;
		
		if (base_name!=NULL){
			printf("parsing the regex file and creating the NFA...\n");
			FILE *regex_file=fopen(base_name,"r");
			if (regex_file==NULL) fatal("cannot open regex file!");
			regex_parser *parser=new regex_parser(true,true);
			NFA *nfa = parser->parse(regex_file);
			fclose(regex_file);
			nfa->remove_epsilon();
			nfa->reduce();
			delete parser;
				
			printf("computing the compressed NFA...\n");
			compNFA *comp_nfa = new compNFA(nfa);
			
			if (VERBOSE || DEBUG){
				printf("alphabet size=%d\n",comp_nfa->alphabet_size);
				//for (symbol_t c=0;c<CSIZE;c++) if (comp_nfa->alphabet_tx[c]!=0) printf("[%c->%d]",c,comp_nfa->alphabet_tx[c]); 
				//printf("\n\n");
			}
			delete nfa;
				
			//create the memory layout
			printf("creating the memory layout...\n");
			mem= new memory(comp_nfa);
			delete comp_nfa;
		
		}else{
			printf("importing memory content from file...\n");
			mem=new memory();
			mem->get(import_file);
		}
		
		//export
		if (export_file!=NULL) mem->put(export_file);
						
		//process trace file
		if (trace!=NULL){
			char *data;
			int trace_size = load_trace(trace,&data);
			mem->traverse_nfa(trace_size,data,log);
			delete [] data;
		}
		
		if (num_servers){
			load_servers(num_servers,M_NFA,mem,log);
		}
				
		delete mem;
			
	} //end NFA
	
	/* Hybrid-FA */
	
	if (mode==M_HFA){
		
		memory *mem=NULL;
		
		if (base_name!=NULL){
			printf("parsing the regex file and creating the HFA...\n");
			FILE *regex_file=fopen(base_name,"r");
			if (regex_file==NULL) fatal("cannot open regex file!");
			regex_parser *parser=new regex_parser(true,true);
			NFA *nfa = parser->parse(regex_file);
			fclose(regex_file);
			delete parser;
			nfa->remove_epsilon();
			nfa->reduce();
			HybridFA *hfa = new HybridFA(nfa);
			mem = new memory(hfa);
			delete hfa;
			delete nfa;
		}else{
			printf("importing memory content from file...\n");
			mem=new memory();
			mem->get(import_file);
		}
		
		//export
		if (export_file!=NULL) mem->put(export_file);
		
		printf("memory layout generated\n");
		
		//process trace file
		if (trace!=NULL){
			char *data;
			int trace_size = load_trace(trace,&data);
			mem->traverse_hfa(trace_size,data,log);
			delete [] data;
		}
		
		if (num_servers){
			load_servers(num_servers,M_HFA,mem,log);
		}
	
		delete mem;
			
	}
	
	if (trace!=NULL) fclose(trace);
	if (import_file!=NULL) fclose(import_file);
	if (export_file!=NULL) fclose(export_file);
	
	return 0;
}

