//  HEADER FILE FOR LAB 3&4
//  contains: structs and checksum
//  Francesca Narea

// universal packet structures	
typedef struct{
	int seq_ack; // sequence number for error correction
	int length; 	
	int checksum; // holds the checksum for packet to account for missing bits during transmission
} HEADER;

typedef struct{
	HEADER header;
	char data[10]; // message/data that is being sent/received
} PACKET;

//universal checksum 
int check_packet_sum(PACKET *packet){
	int i = 0;
	char cksum = 0;
	char* packet_byte = (char*)packet; // gets packet as bytes that can be xor'd to calculate checksum
	for(i=0;i<sizeof(HEADER)+packet->header.length; ++i){ 
		cksum ^= *packet_byte; 
		++packet_byte;
	} //XORING each byte to get sum
	return (int)cksum;
}
