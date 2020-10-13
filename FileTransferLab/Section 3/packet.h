#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct Packets {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
} Packet;

void packetToString(char *buf, Packet *packet){
	
}

void stringToPacket(char *buf, Packet *packet){
	
}