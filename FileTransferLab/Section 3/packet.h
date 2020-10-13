#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>

typedef struct Packets {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
} Packet;

#define BUFFER_SIZE 1200
#define DATA_SIZE 1000

void packetToString(char *buf, Packet *packet){
	
}

// Using the information from the buffer, a packet is created
void stringToPacket(char *buf, Packet *packet){
	regex_t preg; 
	if(regcomp(&preg, "[:]", REG_EXTENDED)) {
        fprintf(stderr, "regcomp error\n");
    }
	
	regmatch_t pmatch[1];
	int cursor = 0; 
	char reg_buf[BUFFER_SIZE]; 
	
	// Get the total_frag #
    if(regexec(&preg, buf + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "regex error\n");
        exit(1);
    }
    memset(reg_buf, 0, BUFFER_SIZE * sizeof(char));
    memcpy(reg_buf, buf + cursor, pmatch[0].rm_so);
    packet -> total_frag = atoi(reg_buf);
    cursor += (pmatch[0].rm_so + 1);
	
	printf("total frag: %d\n", packet -> total_frag);

	
	
	
	exit(1);
}