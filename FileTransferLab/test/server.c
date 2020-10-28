#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>  
#include <arpa/inet.h>
#include <stdbool.h>
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
	char buff[BUFFER_SIZE];
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
	
	
	// Start receiving from client
	Packet packet; 
	packet.filename = (char *) malloc(BUFFER_SIZE);
	char filename[BUFFER_SIZE] = {0};
	FILE *file_ptr = NULL;
	bool *isFragmentRecv= NULL;
	
	while (1){
		int recv_ret = recvfrom(socketFD, buff, sizeof(buff), 0, (struct sockaddr *)&clientaddr, &clientsize);
		if (recv_ret < 0){
			printf("Receving error\n");
			exit(1);
		}
		
		stringToPacket(buff, &packet);
		
		// If there exists no file with filename, open a new file and initialize data structure to check 
		// if files are being recieved
		if(file_ptr == NULL){
			strcpy(filename, packet.filename);
			file_ptr = fopen(filename, "w");
			isFragmentRecv = (bool *)malloc(packet.total_frag * sizeof (isFragmentRecv));
			for (int i = 0; i < packet.total_frag; i++){
					isFragmentRecv[i] = false;			
			}
		}
		
		// Copy data into the new file
		if (isFragmentRecv[packet.frag_no] == false){
			int ret = fwrite(packet.filedata, sizeof(char), packet.size, file_ptr);
			if (ret != packet.size){
				printf("writing error\n");
				exit(1);
			}
			isFragmentRecv[packet.frag_no] = true;
		}
		
		// Set up acknowledgment to send back to client 
		strcpy(packet.filedata, "ACK");
		

		packetToString(buff, &packet);
		
		int send = sendto(socketFD, buff, BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr)); 
		if (send < 0){
			printf("Sending No Confirmation Message Error\n"); 
			exit(1);
		}
		
		// If at the last fragment, break out of while loop
		if (packet.frag_no == packet.total_frag){
			break;
		}
		
	}
	
	// waiting for FIN message before closing the connection 
	struct timeval to; 
	to.tv_sec = 1; 
	to.tv_usec = 999999; 
	int retsockopt = setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&to, sizeof(to)); 
	
	if (retsockopt < 0) {
        printf("Setsockopt failure\n");
    }
	
	while (1){
		if (recvfrom(socketFD, buff, sizeof(buff), 0, (struct sockaddr *)&clientaddr, &clientsize) == -1){
			printf("Error when waiting for wait message. Might be timeout. Connection closed.\n"); 
		} 
		stringToPacket (buff, &packet);
		if (strcmp(packet.filedata, "FIN") == 0) { 
			printf("File transfer completed!\n");
			break;
		} else {
			strcpy(packet.filedata, "ACK");
            packetToString(buff, &packet);
            if ((sendto(socketFD, buff, BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) == -1)) {
                printf("sending ACK error\n");
                exit(1);
            }
		}
	}
		
	
	
		
	// Close socket file descriptor 
	freeaddrinfo(serveraddr);
	int closeFD = close(socketFD); 
	if (closeFD < 0){
		printf("Closing Socket Error\n"); 
		exit(1);
	}
	
	int closeFile = fclose(file_ptr);
	if (closeFile < 0){
		printf("Closing File Error\n"); 
		exit(1);
	}
	
	
	return 0;
}
