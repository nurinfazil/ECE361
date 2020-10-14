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

#define ALIVE 6


void send_file(char * filename, int socketFD, struct addrinfo* serverinfo);



int main(int argc, char **argv){

    
    // check if arguments entered in correct format
    if (argc != 5){
		printf("Bad argument count, USAGE: deliver <server address> <server port number> ftp <filename>\n");
        exit(1);
	}

    // initialise variables

    char * IPaddr = argv[1];
    
    int port = atoi(argv[2]);

    char * serverPort = argv[2];

    char * filename = argv[4];

    
    // create socket variables and declare settings

    struct addrinfo hints;
    struct addrinfo *serverinfo;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    // get info tied to socket created on server

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


    if ( access(filename, F_OK) == 0 ) {
        
        int send = sendto(socketFD, ftp, sizeof(ftp), 0, (const struct sockaddr *) serverinfo->ai_addr, serverinfo->ai_addrlen);  

        if (send < 0) {
            printf("Sending Message to Server failed. \n");
            exit(1);
        }  
        printf("Sending to server...\n");
        
    }
    else {

        printf("Accessing file error: doesn't exist\n");
        exit(1);
    }

    char buff[BUFFER_SIZE] = "\0";
    socklen_t serversize = sizeof(serverinfo);

    int recv = recvfrom(socketFD, buff, sizeof(buff), 0, (struct sockaddr *)&serverinfo, &serversize );
    if (recv < 0) {

        printf("Receiving Server Message Error\n");
        exit(1);
    }

    buff[recv] = "/0";

    if (strcmp(buff, "yes") == 0 ) {
        
        // Stop time and print time here
        //endTime = clock(); 
		//printf("RTT = %f sec.\n", ((double) (endTime - startTime) / CLOCKS_PER_SEC)); 
        
        printf("A file transfer can start.\n");

    }
    else {
        printf("Incorrect message received. File transfer initialization unsuccessful.\n");
        exit(1);
    }


    send_file(filename, socketFD, serverinfo);


    // Close socket file descriptor 
	int closeFD = close(socketFD); 
	if (closeFD < 0){
		printf("Closing Socket Error"); 
		exit(1);
	}

    //freeaddrinfo(serverinfo);


return 0;

}



void send_file(char * filename, int socketFD, struct addrinfo* serverinfo) {

    FILE * file;

    file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error Opening File");
    }

    fseek(file, 0, SEEK_END);

    int fragmentAmt = (ftell(file) / 1000) + 1;
    rewind(file);

    printf("File produces %d packet/s\n", fragmentAmt);

    char buff[BUFFER_SIZE] = "/0";

    char **packets = malloc(sizeof(char*) * fragmentAmt);

    for (int num = 1; num <= fragmentAmt; num++) {

        Packet packet;

        memset(packet.filedata, 0, sizeof(char) * (1000));
        
        fread((void*)packet.filedata, sizeof(char), 1000, file);

        packet.total_frag = fragmentAmt;
        packet.frag_no = num;
        packet.filename = filename;

        if (num != fragmentAmt) {
            packet.size = 1000;
        }
        else {
            fseek(file, 0, SEEK_END);
            packet.size = (ftell(file) - 1) % 1000 + 1;

        }


        
    }


    struct timeval timeout;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if(setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        printf("Error with setsockopt\n");
    } 



    int timesent = 0;   
    socklen_t serverinfo_size = sizeof(serverinfo);

    Packet ack_packet;  
    ack_packet.filename = (char *)malloc(BUFFER_SIZE * sizeof(char));

    for(int packet_num = 1; packet_num <= fragmentAmt; ++packet_num) {

        int numbytes;       
        ++timesent;

        
        printf("hello %s\n", packets[packet_num - 1]);

        numbytes = sendto(socketFD, packets[packet_num - 1], BUFFER_SIZE, 0 , (struct sockaddr *) serverinfo->ai_addr, serverinfo->ai_addrlen);
        
        if(numbytes == -1) {
            printf("Error Sending Packet #%d\n", packet_num);
            exit(1);
        }

        printf("Sendto Successful\n");

        memset(buff, 0, sizeof(char) * BUFFER_SIZE);

        socklen_t serversize = sizeof(serverinfo);

        numbytes = recvfrom(socketFD, buff, BUFFER_SIZE, 0, (struct sockaddr *)&serverinfo, &serversize);

        if (numbytes == -1) {
            
            printf("Timeout or recvfrom error for ACK packet #%d, resending attempt #%d...\n", --packet_num, timesent);
            if(timesent < ALIVE){
                continue;
            }
            else {
                printf("Too many resends. File transfer terminated.\n");
                exit(1);
            }
        }
        
        stringToPacket(buff, &ack_packet);
        
        // Check contents of ACK packets
        if(strcmp(ack_packet.filename, filename) == 0) {
            if(ack_packet.frag_no == packet_num) {
                if(strcmp(ack_packet.filedata, "ACK") == 0) {
                    
                    printf("ACK packet #%d received\n", packet_num);
                    timesent = 0;
                    continue;
                }
            }
        }

        // Resend packet
        printf("ACK packet #%d not received, resending attempt #%d...\n", packet_num, timesent);
        --packet_num;

    }
    

    // Free memory
    for(int packet_num = 1; packet_num <= fragmentAmt; ++packet_num) {
        free(packets[packet_num - 1]);
    }
    free(packets);

    free(ack_packet.filename);








}
