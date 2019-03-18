// UDP_SERVER.C
// Lab3--COEN146L
// Francesca Narea

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include "lab3.h"

int main (int argc, char *argv[])
{
	int sock, nBytes, i;
	uint8_t cksum;
	char buffer[10], name_buffer[1024]; 			// name_buffer used for file name, buffer used for data
	struct sockaddr_in serverAddr, clientAddr;		// setup for udp connection
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	FILE *f_out;						// file that will write received data to

    if (argc != 2)						// need input of port number of the process that server is running
    {
        printf ("need the port number\n");
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
		printf ("socket error\n");
		return 1;
	}

	// bind
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0){
		printf ("bind error\n");
		return 1;
	}

	// receive output file name
	if((recvfrom(sock,name_buffer,sizeof(name_buffer),0,(struct sockaddr *)&serverStorage, &addr_size)) < 0){
		printf("error receiving output name\n");
		return 1;
	}

	// open output file 
	f_out = fopen(name_buffer, "w"); 
	
	// will continue to receive packets containing the data from the client 
	while (1)
	{
		PACKET rcvpacket;
		// receiving packet from client
		while(recvfrom(sock,&rcvpacket,sizeof(rcvpacket),0,(struct sockaddr *)&serverStorage,&addr_size) == -1){
			printf("error receiving packet from client\n");
			return 1;
		}
		
		printf("Checksum Received:%d\n",rcvpacket.header.checksum);
	
		// when receive an empty packet, this is client's signal that transmission is complete
		if(rcvpacket.header.length == 0){
			printf("client completed data transmitting. closing socket.");
			break;
		}

		printf("received a packet from client.\n");
		
		// need to calculate the packet's checksum with the packet's sum set to 0
		uint8_t save_pkt_cksum = rcvpacket.header.checksum;
		rcvpacket.header.checksum = 0;
		
		// calculate checksum
		cksum = check_packet_sum(rcvpacket);	
		if(rand()%2==0){ cksum=0; } //randomization error

		printf("Calculated checksum: %d\nGiven checksum:%d\n", cksum, save_pkt_cksum);
		if(cksum == save_pkt_cksum){
			printf("Checksum correct.\n");
			
			// deliver data
			strcpy(buffer, rcvpacket.data);
			fwrite(buffer,1,rcvpacket.header.length,f_out);
			
			printf("Buffer data:%s\n",buffer);

			// sending ack
			HEADER header;
			header.seq_ack = rcvpacket.header.seq_ack; // the ack = current acknowledgement seq num
			header.length = 0;
			header.checksum = 0;
		
			PACKET sndpacket;
			sndpacket.header = header;
			strcpy(sndpacket.data,"");
			
			// will continuously send the ack package until success
			while(sendto(sock,&sndpacket,sizeof(sndpacket),0,(struct sockaddr *)&serverStorage,addr_size) == -1){
				printf("error sending ACK\n");
			}
		}	
		else{
			// the ACK was for another seq #
			HEADER header;
			header.seq_ack = (rcvpacket.header.seq_ack+1)%2;
			header.length = 0;
			header.checksum = 0;
			PACKET sndpacket;
			sndpacket.header = header;
			strcpy(sndpacket.data,"");
			while(sendto(sock,&sndpacket,sizeof(sndpacket),0,(struct sockaddr *)&serverStorage,addr_size) == -1){
				printf("error sending ack.\n");
			}
		
		}
	}	
	fclose(f_out);
	close(sock);	
	return 0;
}
