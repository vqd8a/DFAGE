/* tcpclient.c */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "tcp.h"

unsigned load_trace(FILE *trace, char **data){
	printf("load_trace: loading trace file to memory...\n");
	unsigned filesize=0;
	rewind(trace);
	int c=fgetc(trace);
	while (c!=EOF) {
		filesize++;
		c=fgetc(trace);
	}
	rewind(trace);
	(*data)=new char[filesize];
	for (unsigned i=0;i<filesize;i++) (*data)[i]=(char)fgetc(trace);		
	printf("load_trace: %d bytes loaded from tracefile\n",filesize);
	return filesize;
}

int main(int argc, char **argv)

{

        int *sock, bytes_recieved;  
        char *trace;
        struct hostent *host;
        struct sockaddr_in server_addr;
        int index=0;
        int num_clients=1;
        
        FILE *tracefile=fopen(argv[1],"rb");
        if (tracefile==NULL){
        	printf("usage ./tcpclient TRACEFILE [NUM_CLIENTS]");
        	exit (-1);
        }
        if (argc>2) num_clients=atoi(argv[2]);
        
        sock=new int[num_clients];
        
        int tracesize = load_trace(tracefile,&trace);
        fclose(tracefile);

        host = gethostbyname(HOST_NAME);

        server_addr.sin_family = AF_INET;     
        server_addr.sin_port = htons(CONN_PORT);   
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr.sin_zero),8); 

        
        for (int i=0;i<num_clients;i++){
	        if ((sock[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	            perror("Socket");
	            exit(1);
	        }
	        if (connect(sock[i], (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) 
	        {
	            perror("Connect");
	            exit(1);
	        }
        }
      

        while(1)
        {
        
          int byte_to_send = (tracesize-index>=PACKET_SIZE) ? PACKET_SIZE : tracesize-index;	
        
          int sent = send(sock[0],trace+index,byte_to_send, 0);
          for (int i=1;i<num_clients;i++){
        	  send(sock[i],trace+index,byte_to_send, 0);
          }
          
          index+=sent;
          
#ifdef LOGGING
          printf("%d data sent, total byte sent\n",sent,index);
#endif                  
          
          if (index>=tracesize){
        	  printf("%d total bytes sent\n",index);
        	  for (int i=0;i<num_clients;i++){
        		  shutdown(sock[i],2);
        		  close(sock[i]);
        	  }
        	  break;
          }
        
        }
        delete [] sock;
        delete [] trace;
        return 0;
}
