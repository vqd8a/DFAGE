/* tcpserver.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include "tcp.h"

int main()
{
        int sock, connected, bytes_received , true_p = 1;  
        char recv_data[MAX_TRACESIZE];
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
		
        printf("\nTCPServer Waiting for client on port %d",CONN_PORT);
        fflush(stdout);


        while(1)
        {  

            sin_size = sizeof(struct sockaddr_in);

            //waits for an incoming requests; when it arrives creates a socket for it
            //returns the socket ID and the address of the connecting client
            connected = accept(sock, (struct sockaddr *)&client_addr,&sin_size); 

            index=0;
            
            printf("\n I got a connection from (%s , %d)\n",
                   inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

            while (1)
            {
              
              struct timeval start,end;
              gettimeofday(&start,NULL);
            	
              //send on connected socket the data. the last flag is 0 by default (MSG_00B for high priority, MSG_PEEK for look message without removing)
              //returns number of bytes sent and received if error
              bytes_received = recv(connected,recv_data+index,PACKET_SIZE,0);
              
              index+=bytes_received;
              
#ifdef LOGGING              
              printf("%d data received, %d total byte received\n", bytes_received, index);
#endif
              
              if (bytes_received==0){
            	  printf("%d total byte received\n", index);
            	  gettimeofday(&end,NULL);
            	  unsigned r_time = (end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
            	  printf("Running_time=%ld sec. %ld usec.\n",r_time/1000000,r_time%1000000);
            	  break;
              }              
            }
        }       

      close(sock);
      return 0;
} 
