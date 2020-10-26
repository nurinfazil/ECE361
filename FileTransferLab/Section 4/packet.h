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

// Given a packet, create a string and puts it in buf
void packetToString(char *buf, Packet *packet){
	memset(buf, 0, BUFFER_SIZE); 
	
	int cursor = 0; 
	
	sprintf(buf, "%d", packet -> total_frag);
    cursor = strlen(buf);
    memcpy(buf + cursor, ":", sizeof(char));
    ++cursor;	
	
	sprintf(buf + cursor, "%d", packet -> frag_no);
    cursor = strlen(buf);
    memcpy(buf + cursor, ":", sizeof(char));
    ++cursor;
	
	sprintf(buf  + cursor, "%d", packet -> size);
    cursor = strlen(buf);
    memcpy(buf + cursor, ":", sizeof(char));
    ++cursor;
	
	sprintf(buf  + cursor, "%s", packet -> filename);
    cursor += strlen(packet->filename);
    memcpy(buf + cursor, ":", sizeof(char));
    ++cursor;
	
	memcpy(buf + cursor, packet->filedata, sizeof(char)*DATA_SIZE);
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
	
	// printf("total frag: %d\n", packet->total_frag);

	// Get frag #
	if(regexec(&preg, buf + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "regex error\n");
        exit(1);
    }
	memset(reg_buf, 0, BUFFER_SIZE * sizeof(char));
    memcpy(reg_buf, buf + cursor, pmatch[0].rm_so);
    packet -> frag_no = atoi(reg_buf);
    cursor += (pmatch[0].rm_so + 1);
	
	// printf("frag no: %d\n", packet -> frag_no);

	// Get size 
	if(regexec(&preg, buf + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "regex error\n");
        exit(1);
    }
	memset(reg_buf, 0, BUFFER_SIZE * sizeof(char));
    memcpy(reg_buf, buf + cursor, pmatch[0].rm_so);
    packet -> size = atoi(reg_buf);
    cursor += (pmatch[0].rm_so + 1);
	
	// printf("size: %d\n", packet -> size);
	
	// Get filename
	if(regexec(&preg, buf + cursor, 1, pmatch, REG_NOTBOL)) {
        fprintf(stderr, "regex error\n");
        exit(1);
    }
    memcpy(packet->filename, buf + cursor, pmatch[0].rm_so);
    packet -> filename[pmatch[0].rm_so] = 0;
    cursor += (pmatch[0].rm_so + 1);
	
	// printf("name: %s\n", packet -> filename);
	
	// Get data 
	memcpy(packet->filedata, buf + cursor, packet->size);
	// printf("data: %s\n", packet -> filedata);

}