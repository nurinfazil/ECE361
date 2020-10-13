#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include "packet.h"

int main(int argc, char **argv){

    char *IPaddr = argv[1];
    
    int port = atoi(argv[2]);

    char *serverPort = argv[2];

    // check if arguments entered in correct format
    if (argc != 5){
		printf("Bad argument count, USAGE: deliver <server address> <server port number> ftp <filename>\n");
        exit(1);
	}
    
    // create a socket

    struct addrinfo hints;
    struct addrinfo *serverinfo;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;


    int status = getaddrinfo(IPaddr, serverPort, &hints, &serverinfo);

    int socketFD = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
    if (socketFD < 0) {
        printf("Socket Error");
        exit(1);
    }
    
	// Start time here
	clock_t startTime, endTime; 
	startTime = clock();
	
    const char ftp[100] = "ftp";
    if ( access(argv[4], F_OK) == 0 ) {
        
        int send = sendto(socketFD, ftp, sizeof(ftp), 0, (const struct sockaddr *) serverinfo->ai_addr, serverinfo->ai_addrlen);  

        if (send < 0) {
            printf("error sending to server\n");
            exit(1);
        }  
        printf("Sending to server...\n");
        
    }
    else {

        printf("Accessing file error: doesn't exist\n");
        exit(1);
    }

    char buff[100] = "\0";
    socklen_t serversize = sizeof(serverinfo);

    int recv = recvfrom(socketFD, buff, sizeof(buff), 0, (struct sockaddr *)&serverinfo, &serversize );
    if (recv < 0) {

        printf("Receiving Server Message Error\n");
        exit(1);
    }

    buff[recv] = '\0';

    if (strcmp(buff, "yes") == 0 ) {
		// Stop time and print time here
        endTime = clock(); 
		printf("RTT = %f sec.\n", ((double) (endTime - startTime) / CLOCKS_PER_SEC));  
        printf("A file transfer can start.\n");
    }
    else {
        exit(1);
    }


    // Close socket file descriptor 
	int closeFD = close(socketFD); 
	if (closeFD < 0){
		printf("Closing Socket Error"); 
		exit(1);
	}

return 0;

}
