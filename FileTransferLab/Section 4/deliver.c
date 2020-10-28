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
#include <stdbool.h>

#include "packet.h"

#define RUNS 6


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

    struct addrinfo * new_serverinfo;

    int recv = recvfrom(socketFD, buff, sizeof(buff), 0, (struct sockaddr *)&new_serverinfo, &serversize );
    if (recv < 0) {

        printf("Receiving Server Message Error\n");
        exit(1);
    }

    buff[recv] = "\0";
	//double initialRTT;

    if (strcmp(buff, "yes") == 0 ) {
        
        // Stop time and print time here
        endTime = clock(); 

        
        

		//initialRTT = ((double) (endTime - startTime) / CLOCKS_PER_SEC);
		//printf("RTT = %f sec.\n", initialRTT); 
        
        printf("A file transfer can start.\n");

    }
    else {
        printf("Error. File transfer may not start.\n");
        exit(1);
    }
	
	//initialRTT = ((double) (endTime - startTime) / CLOCKS_PER_SEC);
    clock_t initialRTT = endTime - startTime;
	printf("initial RTT: %lu usec\n", initialRTT);

    // Section 3

    FILE * file;

    file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error Opening File");
    }

    fseek(file, 0, SEEK_END);

    int fragmentAmt = (ftell(file) / 1000) + 1;
    rewind(file);

    printf("There are %d packets/s\n", fragmentAmt);


    char **packets = malloc(sizeof(char*) * fragmentAmt);


    for(int packet_num = 1; packet_num <= fragmentAmt; ++packet_num) {

        Packet packet;

        memset(packet.filedata, 0, sizeof(char) * (1000));
        
        fread((void*)packet.filedata, sizeof(char), 1000, file);

        packet.total_frag = fragmentAmt;
        packet.frag_no = packet_num;
        packet.filename = filename;

        if (packet_num != fragmentAmt) {
            packet.size = 1000;
        }
        else {
            fseek(file, 0, SEEK_END);
            packet.size = (ftell(file) - 1) % 1000 + 1;

        }

        packets[packet_num - 1] = malloc(BUFFER_SIZE * sizeof(char));
        packetToString(packets[packet_num - 1], &packet);

        
    }


    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 999999;

    if(setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        printf("setsockopt error\n");
    } 


    int retry = 0;   
    socklen_t serverinfo_size = sizeof(serverinfo);

    Packet ack_packet;  
    ack_packet.filename = (char *)malloc(BUFFER_SIZE * sizeof(char));






    // Section 4

    clock_t estimateRTT = 2 * initialRTT;
    clock_t devRTT = initialRTT;
    clock_t sampleRTT, dev;
    clock_t timeStart, timeEnd;

    
    int numbytes;
    int packet_num = 1;
    bool timeoutConfirm;

    while (packet_num <= fragmentAmt) {

        timeoutConfirm = false;
        memset(buff, 0, sizeof(char) * BUFFER_SIZE);
        timeStart = clock();
        

        int send = sendto(socketFD, packets[packet_num - 1], BUFFER_SIZE, 0, (const struct sockaddr *) serverinfo->ai_addr, serverinfo->ai_addrlen);
        if (send < 0) {
            printf("Error sending packet %d\n", packet_num);
            exit(1);
        }

        while (1) {
       
            // ++retry;

            // int send = sendto(socketFD, packets[packet_num - 1], BUFFER_SIZE, 0, (const struct sockaddr *) serverinfo->ai_addr, serverinfo->ai_addrlen);  
        
            // if (send < 0) {
            //     printf("Packet Sending Error %d \n", packet_num);
            //     exit(1);
            // } 
        

            // printf("Send to Successful!\n");

            memset(buff, 0, sizeof(char) * BUFFER_SIZE);

            socklen_t serversize = sizeof(new_serverinfo);

            int received = recvfrom(socketFD, buff, BUFFER_SIZE, 0, (struct sockaddr *)&new_serverinfo, &serversize);

            if (received == -1) {
                
                retry++;

                printf("Error receiving ACK packet #%d, trying to resend: attempt #%d...\n", packet_num, retry);
                if(retry <= RUNS){
                    timeoutConfirm = true;
                    break;
                } else {
                    printf("Too many resends. File transfer terminated.\n");
                    exit(1);
                }
            }
        
            stringToPacket(buff, &ack_packet);
            
            // Check contents of ACK packets

            if(ack_packet.frag_no != packet_num) {

                printf("Not current ACK, packet dropped, still waiting...\n");
                continue;
            } else {
                break;
            }


            // if(strcmp(ack_packet.filename, filename) == 0) {
            //     if(ack_packet.frag_no == packet_num) {
            //         if(strcmp(ack_packet.filedata, "ACK") == 0) {
                        
            //             printf("ACK packet #%d received\n", packet_num);
            //             retry = 0;
            //             continue;
            //         }
            //     }
            // }

            // Resend packet
            // printf("ACK packet #%d not received, trying to resend: attempt #%d...\n", packet_num, retry);
            // --packet_num;

        }

        timeEnd = clock();

        sampleRTT = timeEnd - timeStart;
        estimateRTT = 0.875 * ((double) estimateRTT) + (sampleRTT >> 3);
        dev = (estimateRTT > sampleRTT) ? (estimateRTT - sampleRTT) : (sampleRTT - estimateRTT);
        devRTT = 0.75 * ((double) devRTT) + (dev >> 2);
        timeout.tv_usec = 20 * estimateRTT + (devRTT << 2);
        
        if(setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
            printf("setsockopt failed\n");
        }

        if (timeoutConfirm == false) {
            packet_num++;
            retry = 0;
        } else {
            printf("Packet #%d timeout, timeout reset to:\t%d usec\n", packet_num, timeout.tv_usec);		
        }
	}

	// send FIN message
	Packet fin;
	fin.total_frag = fragmentAmt;
	fin.frag_no = 0;
	fin.size = DATA_SIZE;
	fin.filename = filename;
	strcpy(fin.filedata, "FIN");
	packetToString(buff, &fin);
	
    if((numbytes = sendto(socketFD, buff, BUFFER_SIZE, 0, (const struct sockaddr *) serverinfo->ai_addr, serverinfo->ai_addrlen)) == -1) {
        printf("sendto error for FIN message\n");
        exit(1);
    }

    // Free memory
    for(int packet_num = 1; packet_num <= fragmentAmt; ++packet_num) {
        free(packets[packet_num - 1]);
    }
    free(packets);

    free(ack_packet.filename);

    // Close socket file descriptor 
	int closeFD = close(socketFD); 
	if (closeFD < 0){
		printf("Closing Socket Error"); 
		exit(1);
	}

return 0;

}


