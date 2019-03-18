// UDP_SERVER.C
// COEN146L--Lab4
// Francesca Narea

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include "lab3.h"

int main (int argc, char *argv[])
{
	srand(time(NULL)); // seed for rand
	int sock, nBytes, i;
	uint8_t cksum;
	PACKET name_buffer; // name_buffer used for file name, buffer used for data
	struct sockaddr_in serverAddr, clientAddr; // setup for udp connection
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	FILE *f_out; 

        // need input of port number of the process that server is running
    	if(argc != 2)
	{
        printf ("Need the port number as input for this process.\n");
        return 1;
	 }

	// init 
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons ((short)atoi (argv[1]));
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof (serverStorage);

	// create socket
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf ("Socket error.\n");
		return 1;
	}

	// bind
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0){
		printf ("Bind error.\n");
		return 1;
	}

	// receive output file name
	if((recvfrom(sock,&name_buffer,sizeof(name_buffer),0,(struct sockaddr *)&serverStorage, &addr_size)) < 0){
		printf("Error receiving output name.\n");
		return 1;
	}

	// open output file 
	f_out = fopen(name_buffer.data, "w"); 
	
	// will continue to receive packets containing the data from the client until recieves empty packet
	while (1)
	{
		PACKET rcvpacket;
		// receiving packet from client
		while(recvfrom(sock,&rcvpacket,sizeof(rcvpacket),0,(struct sockaddr *)&serverStorage,&addr_size) == -1){
			printf("Error receiving packet from client.\n");
			return 1;
		}
		
		// an empty packet is client's signal that transmission is complete
		if(rcvpacket.header.length == 0){
			printf("Client completed data transmitting. Closing socket.");
			break;
		}

		// if randomly decide to drop packet (not send ack) to fake error... will continue to next loop iteration
		if(rand()%3 == 0){ continue; }

		// need to calculate the packet's checksum with the packet's sum set to 0
		uint8_t save_pkt_cksum = rcvpacket.header.checksum;
		rcvpacket.header.checksum = 0;
		
		// calculate checksum
		cksum = check_packet_sum(&rcvpacket);	
		if(rand()%3==0){ cksum=0; } //randomization error, cksum may be incorrectly calculated as 0

		printf("Calculated checksum: %d\nGiven checksum:%d\n", cksum, save_pkt_cksum);

		if(cksum == save_pkt_cksum){
			
			// deliver data
			fwrite(rcvpacket.data,1,rcvpacket.header.length,f_out);
			
			// sending ack
			HEADER header;
			header.seq_ack = rcvpacket.header.seq_ack; // the ack = current acknowledgement seq num
			header.length = 0;
			header.checksum = 0;
			
			PACKET sndpacket;
			sndpacket.header = header;
			strcpy(sndpacket.data,"");
			
			printf("Sending ACK, seq #: %d\n", header.seq_ack);			
			// will continuously send the ack package until success
			while(sendto(sock,&sndpacket,sizeof(sndpacket),0,(struct sockaddr *)&serverStorage,addr_size) == -1){
				printf("Error sending ACK. Retransmitting...\n");
				}
		}	
		else{
			// the ACK was for another seq #
			HEADER header;
			header.seq_ack = (rcvpacket.header.seq_ack+1)%2; // flip the ack # to correct the sequence error
			header.length = 0;
			header.checksum = 0;
			PACKET sndpacket;
			sndpacket.header = header;
			strcpy(sndpacket.data,"");

			while(sendto(sock,&sndpacket,sizeof(sndpacket),0,(struct sockaddr *)&serverStorage,addr_size) == -1){
				printf("Error sending ACK. Retransmitting...\n");
			}
		}
	}	
	fclose(f_out);
	close(sock);	
	return 0;
}
