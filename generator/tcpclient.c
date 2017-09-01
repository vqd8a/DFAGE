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

        int sock, bytes_recieved;  
        char *trace;
        struct hostent *host;
        struct sockaddr_in server_addr;
        int index=0;
        
        FILE *tracefile=fopen(argv[1],"rb");
        if (tracefile==NULL){
        	printf("usage ./tcpclient TRACEFILE");
        	exit (-1);
        }
        
        int tracesize = load_trace(tracefile,&trace);
        fclose(tracefile);

        host = gethostbyname(HOST_NAME);

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        server_addr.sin_family = AF_INET;     
        server_addr.sin_port = htons(CONN_PORT);   
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr.sin_zero),8); 

        //returns 0 if successful, -1 otherwise
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) 
        {
            perror("Connect");
            exit(1);
        }

        while(1)
        {
        
          int byte_to_send = (tracesize-index>=PACKET_SIZE) ? PACKET_SIZE : tracesize-index;	
        
          int sent = send(sock,trace+index,byte_to_send, 0);          
          
          index+=sent;
          
#ifdef LOGGING
          printf("%d data sent, total byte sent\n",sent,index);
#endif                  
          
          if (index>=tracesize){
        	  printf("%d total bytes sent\n",index);
        	  shutdown(sock,2);
        	  close(sock);
        	  FILE *test=fopen("testclient.txt","w");
        	  for (int i=0;i<index;i++) fputc(trace[i],test);
        	  fclose(test);
        	  break;
          }
        
        }
        delete [] trace;
        return 0;
}
