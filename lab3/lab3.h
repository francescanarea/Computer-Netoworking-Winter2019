//  HEADER FILE FOR LAB 3
//  contains: structs and checksum
//
//  Francesca Narea
//  Date: Wednesday, February 6, 2019
//

// universal packet structures	
typedef struct{
	int seq_ack;		// sequence number for error correction
	int length; 	
	uint8_t checksum;	// holds the checksum for packet to account for missing bits during transmission
} HEADER;

typedef struct{
	HEADER header;
	char data[10];		// actual message/data that is being sent/received
} PACKET;

//universal checksum 
int check_packet_sum(PACKET packet){
	uint8_t cksum = 0,i;
	char* packet_byte = (char*)&packet; // gets packet as bytes that can be xor'd to calculate checksum
	for(i=0;i<sizeof(packet_byte);i++){ cksum ^= packet_byte[i]; } //XORING	each byte to get sum
	return cksum;
}
