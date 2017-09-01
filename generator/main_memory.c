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

#include <unistd.h>
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

#define M_DFA 		0
#define M_NFA 		1
#define M_HFA 		2
#define M_GENDFA	3

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

void load_servers(int num_servers,int mode, memory *mem, FILE *log)
{
        int sock, connected, bytes_received , true_p = 1;
        char *trace = NULL;
        int index;

        struct sockaddr_in server_addr,client_addr;    
        socklen_t sin_size;
        
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) { //create the socket
            perror("Socket");								  //domain=AF_INET (address+port number)
            exit(1);									      //type=SOCK_STREAM (virtual circuit for streams)
        }													  //protocol=0 (standard)

        if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true_p,sizeof(int)) == -1) {
            perror("Setsockopt");
            exit(1);
        }
        
        //address of the server to which other processes can connect
        server_addr.sin_family = AF_INET;         	//standard
        server_addr.sin_port = htons(CONN_PORT);    //port 
        server_addr.sin_addr.s_addr = INADDR_ANY;   //IP address
        bzero(&(server_addr.sin_zero),8); 

        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) { //give the address of the socket to the server
            perror("Unable to bind");
            exit(1);
        }

        if (listen(sock, MAX_CONN_REQ) == -1) { //max number of connection requests that can be pending 
            perror("Listen");
            exit(1);
        }
		
        printf("\nTCPServer Waiting for client on port %d\n",CONN_PORT);
        fflush(stdout);

        pid_t pID=0;
  
        for (int i=0;i<num_servers-1;i++) if (pID==0) pID=fork();
        
        printf("pID=%d\n",pID);
        

        while(1)
        {  

            sin_size = sizeof(struct sockaddr_in);

            //waits for an incoming requests; when it arrives creates a socket for it
            //returns the socket ID and the address of the connecting client
            connected = accept(sock, (struct sockaddr *)&client_addr,&sin_size); 

            index=0;
            trace = new char[MAX_TRACESIZE];
            
            printf("\nProcess=%d - Received connection from (%s , %d)\n",
                   pID,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
            
            struct timeval start,end;
            gettimeofday(&start,NULL);
            printf("P%d:: start-time %d\n", pID, start.tv_sec*1000000+start.tv_usec);

            while (1)
            {                                       
              
              //send on connected socket the data. the last flag is 0 by default (MSG_00B for high priority, MSG_PEEK for look message without removing)
              //returns number of bytes sent and received if error
              bytes_received = recv(connected,trace+index,PACKET_SIZE,0);
                          
              index+=bytes_received;
              
#ifdef LOGGING              
              printf("P%d:: %d data received, %d total byte received\n", pID, bytes_received, pID, index);
#endif
              
              if (bytes_received==0){
            	  printf("P%d:: %d total byte received\n", pID, index);
            	  gettimeofday(&end,NULL);
            	  
            	  unsigned r_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
            	  printf("P%d:: Reading running_time=%ld sec. %ld usec.\n",pID, r_time/1000000,r_time%1000000);
            	                
            	  if (mode==M_DFA) mem->traverse_dfa(index,trace,log);
            	  else if (mode==M_NFA) mem->traverse_nfa(index,trace,log);
            	  else if (mode==M_HFA) mem->traverse_hfa(index,trace,log);
            	  else fatal ("Invalid Mode");
            	  gettimeofday(&end,NULL);
            	  printf("P%d:: end-time %d\n", pID, end.tv_sec*1000000+end.tv_usec);
            	  break;
              }              
            }          
            
            delete [] trace;
        }       

      close(sock);
} 


int main(int argc, char **argv){
	if (argc<2){
		printf("usage:: ./regex -dfa|-nfa|-hfa|-gendfa [-f REGEX_FILE] [-n NUM_DFAs] [-d|-v] [-z dump_outfile] [-t TRACE_FILE] [-e EXPORT_FILE] [-E AUTOMATON_FILE] [-I IMPORT_AUTOMATON_FILE] [-g DOT_FILE] [-i IMPORT_FILE] [-server NUM_SERVERS]\n");
		return -1;
	}
	int mode = -1;
	int num_dfas = 0; 		
	char *base_name = NULL; 
	FILE *trace = NULL;
	FILE *log = stdout;
	FILE *export_file = NULL;
	FILE *import_file = NULL;
	FILE *dump_file = NULL;
	FILE *dot_file = NULL;
	FILE *aut_file = NULL;
	FILE *aut_binfile = NULL; char fname1[500];
	FILE *aut_accst_binfile = NULL; char fname2[500];
	FILE *dump_source = NULL;
	char *trace_filename = NULL;
	char *dump_filename = NULL;
	char *dump_source_file = NULL;
	int num_servers=0;
	
	int imod=0;
	bool imod_bool;
	
	int i=1;
	while (i<argc){
		if (strcmp(argv[i],"-dfa")==0){
			mode=M_DFA;
		}else if (strcmp(argv[i],"-nfa")==0){
			mode=M_NFA;
		}else if (strcmp(argv[i],"-hfa")==0){
			mode=M_HFA;
		}else if (strcmp(argv[i],"-gendfa")==0){
			mode=M_GENDFA;
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
				else {
					printf("trace file: %s\n",argv[i]);
					trace_filename=argv[i];
				}
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
		}else if (strcmp(argv[i],"-E")==0) {
			if ((++i) == argc) fatal("automaton filename missing");
			else{
				aut_file=fopen(argv[i],"w");
				if (aut_file==NULL) fatal ("cannot create automaton-file");
				else printf("automaton file: %s\n",argv[i]);
				if (mode==M_GENDFA) {
					strcpy (fname1,argv[i]);
					strcat (fname1,"bin");
					aut_binfile=fopen(fname1,"wb");
					if (aut_binfile==NULL) fatal ("cannot create automaton-binfile");
					else printf("automaton binfile: %s\n",fname1);
					
					strcpy (fname2,argv[i]);
					strcat (fname2,"accstbin");
					aut_accst_binfile=fopen(fname2,"wb");
					if (aut_accst_binfile==NULL) fatal ("cannot create automaton-acceptingstate-binfile");
					else printf("automaton accepting state binfile: %s\n",fname2);
				}
			}
		}else if (strcmp(argv[i],"-I")==0) {
			if ((++i) == argc) fatal("import automaton filename missing");
			else{
				dump_source=fopen(argv[i],"r");
				if (dump_source==NULL) fatal ("cannot open import automaton-file");
				else printf("import automaton file: %s\n",argv[i]);
			}
		}else if (strcmp(argv[i],"-g")==0) {
			if ((++i) == argc) fatal("dot filename missing");
			else{
				dot_file=fopen(argv[i],"w");
				if (dot_file==NULL) fatal ("cannot create dot-file");
				else printf("dot file: %s\n",argv[i]);
			}
		}else if (strcmp(argv[i],"-z")==0) {
			if ((++i) == argc) fatal("dump filename missing");
			else{
				dump_file=fopen(argv[i],"w");
				if (dump_file==NULL) fatal ("cannot create dump-file");
				else {
					printf("dump file: %s\n",argv[i]);
					dump_filename=argv[i];
				}
			}
		}else if (strcmp(argv[i],"-i")==0) {
			if ((++i) == argc) fatal("import filename missing");
			else{
				import_file=fopen(argv[i],"r");
				if (import_file==NULL) fatal ("cannot create import-file");
				else printf("import file: %s\n",argv[i]);
			}
		}else if (strcmp(argv[i],"-imod")==0){//true: ignore case selected (case insensitive), false: ignore case not selected (case sensitive)
			sscanf(argv[++i],"%d", &imod);
     		if (imod==0) imod_bool = false;
            else imod_bool=true;
            printf("imod=%d\n",imod); 			
		}
		i++;
	}
	
	if (base_name==0 && import_file==0 && dump_source==0) fatal("data file for FA missing");
	if (mode==M_DFA){
		if (import_file==0 && num_dfas==0) fatal("0 DFAs selected!");
		printf("DFA selected: dfa-file %s, #=%d\n",base_name,num_dfas);
	}else if (mode==M_NFA){
		printf("NFA selected: regex-file %s\n",base_name);
	}else if (mode==M_HFA){
		printf("Hybrid-FA selected: regex-file %s\n",base_name);
	}else if (mode==M_GENDFA){
		printf("Generate DFA mode selected: regex-file %s\n", base_name);
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
				
				if (aut_file) {
					const int kTitleLen = 1024;
					char title[kTitleLen];
					snprintf(title, kTitleLen, "DFA_%d", i);
					dfa->to_file(aut_file, title);
				}
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
			for (int i=0;i<num_dfas;i++) {
				delete dfas[i];
			}
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
			//int trace_size = load_trace(trace,&data);
			int trace_size=0;
			if(mem->open_capture(trace_filename, dump_filename))
				mem->traverse_dfa(trace_size,data,log);
			//delete [] data;
		}
		
		//server mode
		if (num_servers){
			load_servers(num_servers,M_DFA,mem,log);
		}
		
		delete mem;
		
	} //end DFA
	
	/* NFA */
	
	if (mode==M_NFA || mode==M_GENDFA){
		
		memory *mem=NULL;
		
		if (base_name!=NULL || dump_source!=NULL){
			NFA *nfa=NULL;
			if(base_name!=NULL){
				printf("parsing the regex file and creating the NFA...\n");
				FILE *regex_file=fopen(base_name,"r");
				if (regex_file==NULL) fatal("cannot open regex file!");
				//regex_parser *parser=new regex_parser(false,true);
				regex_parser *parser=new regex_parser(imod_bool,true);
				nfa = parser->parse(regex_file);
				delete parser;
				fclose(regex_file);
			}
			else{
				nfa = NFA::from_file(dump_source);
			}
			
			nfa->remove_epsilon();
			nfa->reduce();
				
			printf("computing the compressed NFA...\n");
			compNFA *comp_nfa = new compNFA(nfa);
			
			if (VERBOSE || DEBUG){
				printf("alphabet size=%d\n",comp_nfa->alphabet_size);
				//for (symbol_t c=0;c<CSIZE;c++) if (comp_nfa->alphabet_tx[c]!=0) printf("[%c->%d]",c,comp_nfa->alphabet_tx[c]); 
				//printf("\n\n");
			}

			if(mode==M_GENDFA) {
				DFA *dfa = nfa->nfa2dfa();
				dfa->minimize(); //orig, comment to test, will recover when testing done
				
				if (aut_binfile != NULL) {
					
					if (dfa->get_default_tx()!=NULL) printf("default_tx != NULL\n");
					else printf("default_tx == NULL\n");
					
					//if (dfa->get_default_tx()==NULL) {
					//	printf("Compressing DFA\n");
					//	dfa->fast_compression_algorithm();
					//	printf("Compression DFA done\n");
					//}
					
					//typedef unsigned int state_t; //typedef unsigned symbol_t;
					//void add_transition(state_t old_state, symbol_t c, state_t new_state);
					//inline void DFA::add_transition(state_t old_state, symbol_t c, state_t new_state){ state_table[old_state][c]=new_state;}
					state_t **dfa_state_table;
					dfa_state_table = dfa->get_state_table();
					unsigned dfa_size = dfa->size();
					// Write the number of states and the transitions of the DFA
					fwrite(&dfa_size, sizeof(unsigned), 1, aut_binfile);
					for (unsigned int row=0; row < dfa_size; row++)
						fwrite(dfa_state_table[row], sizeof(state_t), CSIZE, aut_binfile);
					fclose(aut_binfile);
					
					linked_set **dfa_accepted_rules;
					dfa_accepted_rules = dfa->get_accepted_rules();
					// Write the accepting states and the corresponding rules
					for (state_t s=0; s<dfa_size; s++){
						if (!dfa_accepted_rules[s]->empty()){
							linked_set *ls=	dfa_accepted_rules[s];
							while(ls!=NULL){
								unsigned int tmpval=ls->value();
								fwrite(&s, sizeof(unsigned int), 1, aut_accst_binfile);
								fwrite(&tmpval, sizeof(unsigned int), 1, aut_accst_binfile);
								//printf("%d : accepting %d\n", s, tmpval);
								ls=ls->succ();
							}
						}
					}
					fclose(aut_accst_binfile);
					
					/*//TEST HERE				
					FILE *log1;
					log1 = fopen ("DFA_test1.txt","w");
					fprintf(log1,"Dumping DFA: %ld states\n",dfa->size());
					for (unsigned int row=0; row < dfa->size(); row++){
						fprintf(log1,"> state # %d",row);
						//fprintf(log1,"\n");
						for(unsigned col=0; col<CSIZE; col++){
							//if (dfa_state_table[row]!=NULL && dfa_state_table[row][col]!=0)
								fprintf(log1," - [%d/%c] %d ",col,col,dfa_state_table[row][col]);
						}
						fprintf(log1,"\n");
					}
					fclose (log1);
					
					FILE *log2;
					state_t tmp_state_table;				
					unsigned tmp_size;
					aut_binfile=fopen(fname1,"rb");
					log2 = fopen ("DFA_test2.txt","w");
					fread (&tmp_size, sizeof(unsigned), 1, aut_binfile); fprintf(log2,"Dumping DFA: %ld states\n",tmp_size);
					for (unsigned int row=0; row < tmp_size; row++){
						fprintf(log2,"> state # %d",row);
						for(unsigned col=0; col<CSIZE; col++){
							fread (&tmp_state_table, sizeof(state_t), 1, aut_binfile);								
							fprintf(log2," - [%d/%c] %d ",col,col,tmp_state_table);
						}
						fprintf(log2,"\n");
					}
					fclose(aut_binfile);
					fclose (log2);
					
					FILE *log3;
					unsigned int tmpval;
					aut_accst_binfile = fopen(fname2,"rb");
					log3 = fopen ("DFA_acceptingstates.txt","w");
					while (fread (&tmpval, sizeof(unsigned int), 1, aut_accst_binfile) == 1) {
						fprintf(log3,"%d : accepting ",tmpval); //printf("%d : accepting ", tmpval);
						fread (&tmpval, sizeof(unsigned int), 1, aut_accst_binfile);
						fprintf(log3,"%d\n",tmpval); //printf("%d\n", tmpval);
					}
					fclose(aut_accst_binfile);
					fclose (log3);
					//END TEST*/
				}
				if(aut_file != NULL) {
					printf("printing the converted DFA...\n");
					dfa->to_file(aut_file, "DFA");
				}
			
				if(dot_file != NULL) {
					printf("printing the converted DFA in DOT format...\n");
					dfa->to_dot(dot_file, "DFA");
				}

				delete dfa;
			} else {
				if(aut_file != NULL) {
					printf("printing the compressed NFA...\n");
					nfa->to_file(aut_file, "NFA");
				}
			
				if(dot_file != NULL) {
					printf("printing the compressed NFA in DOT format...\n");
					nfa->to_dot(dot_file, "NFA");
				}
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
		        printf("Qui\n");
			char *data;
			//int trace_size = load_trace(trace,&data);
			int trace_size = 0;
			if(mem->open_capture(trace_filename, dump_filename)){
				printf("qui2\n");
				mem->traverse_nfa(trace_size,data,log);
                        }
			//delete [] data;
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

			if (aut_file) {
				hfa->to_file(aut_file, "HFA");
			}

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
			//int trace_size = load_trace(trace,&data);
			int trace_size = 0;
			if(mem->open_capture(trace_filename, dump_filename))
				mem->traverse_hfa(trace_size,data,log);
			//delete [] data;
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

