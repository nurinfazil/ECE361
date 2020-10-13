#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>  
#include <arpa/inet.h>
#include "packet.h"

int main(int argc, char **argv){
	
	if (argc != 2){
		printf("Bad argument count, USAGE: server <UDP listen port>\n");
	}
	
	const char *port = argv[1];
	
	// Set up to bind socket to server address
	struct addrinfo *serveraddr;
	struct sockaddr_storage clientaddr; 
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(hints));
	
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;  
    hints.ai_flags = AI_PASSIVE; 
	
	getaddrinfo(NULL, port, &hints, &serveraddr);
	
	// Create a socket for communication
	int socketFD = socket(serveraddr->ai_family, serveraddr->ai_socktype, serveraddr->ai_protocol); 
	if (socketFD < 0){
		printf("Socket Error"); 
		exit(1);
	}
			
	int bindSocket = bind(socketFD, serveraddr->ai_addr, serveraddr->ai_addrlen); 
	if (bindSocket < 0){
		printf("Bind Error"); 
		exit(1);
	}
		
	
	// Receive message from client 
	char buff[100];
	socklen_t clientsize = sizeof(struct sockaddr_storage);
	
	int recv = recvfrom(socketFD, buff, sizeof(buff), 0, (struct sockaddr *)&clientaddr, &clientsize);
	if (recv < 0){
		printf("Receiving Client Message Error\n"); 
		exit(1);
	}
	
	buff[recv]= '\0';
	
	// Reply to client. Check what was sent in buff and reply.
	if (strcmp(buff, "ftp") == 0){
		const char *yes = "yes";
		printf("Message is ftp. Sending yes.\n"); 
		int send = sendto(socketFD, yes, sizeof(yes), 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr)); 
		if (send < 0){
			printf("Sending Confirmation Message Error\n"); 
			exit(1);
		}
	} else {
		const char *no = "no";
		printf("Message is not ftp. Sending no.\n"); 
		int send = sendto(socketFD, no, sizeof(no), 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr)); 
		if (send < 0){
			printf("Sending No Confirmation Message Error\n"); 
			exit(1);
		}	
	}
	
		
	// Close socket file descriptor 
	freeaddrinfo(serveraddr);
	int closeFD = close(socketFD); 
	if (closeFD < 0){
		printf("Closing Socket Error\n"); 
		exit(1);
	}
	
	
	return 0;
}
