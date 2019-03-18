// UDP_CLIENT.C
// COEN 146L--Lab4
// Francesca Narea

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "lab3.h"
#include <fcntl.h>

int main (int argc, char *argv[])
{
	int sock, portNum, nBytes, i;
	
	struct sockaddr_in serverAddr;
	socklen_t addr_size;

	char data_buff[10]; // temp for holding packet data
	uint8_t cksum = 0; // will hold calcualted cksum
	int seq_num = 0; // sequence number for packets that may be lost/sent out of order

	// need the IP to connect to server and its port to connect to the process running on the server
	if(argc != 5)
	{
		printf ("need the <port> <ip> <input> <output>\n");
		return 1;
	}

	// configure address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (atoi (argv[1]));
	inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
	memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof serverAddr;

	// create UDP socket
   	 if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf ("socket error\n");
		return 1;
	}

	// setup for timer
	int rv; // holds return value of timer
	struct timeval tv;
	fcntl(sock, F_SETFL, O_NONBLOCK);
	fd_set readfds;
	
	// open input file
	FILE *f_in = fopen(argv[3], "r");
	if(f_in == NULL){
		printf("error opening file.\n");
		return 1;
	}

	// sends input file name as a packet
	PACKET input_name;
	strcpy(input_name.data, argv[4]);	

	while(sendto(sock,&input_name,sizeof(input_name),0,(struct sockaddr *)&serverAddr,addr_size) == -1){
		printf("error sending file name: %s\n",argv[4]);
	}	

	// continues to read for input file until reaches end of file
	while(!feof(f_in)){  			
		// gets data from f_in via data_buff
		int n = fread(data_buff, 1,sizeof(data_buff),f_in);
		printf("Buffer data read: %s\n", data_buff);

		// packet to be filled with  buffer data
		// populating header
		HEADER header;
		header.seq_ack = seq_num;
		header.length = n;
		header.checksum = 0;
		
		// populate packet body
		PACKET sndpacket;
		sndpacket.header = header;
		strcpy(sndpacket.data,data_buff); // adding in input data from buffer to packet data
		cksum = check_packet_sum(&sndpacket); // calculating checksum
		sndpacket.header.checksum = cksum; // setting checksum in header
		printf("checksum calculated: %d\n", sndpacket.header.checksum);		

		// sending packet to server
		while(sendto(sock,&sndpacket,sizeof(sndpacket),0,(struct sockaddr *)&serverAddr,addr_size) == -1){
			printf("error sending packet to server. resending...\n");
		}
		
		// set timer
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		
		// getting timer return value
		rv = select(sock+1, &readfds, NULL, NULL, &tv);
		while(rv == 0){ 
			printf("rv == 0, retransmitting...\n");
			// when rv = 0, this signifies a timeout

			//retransmitting data
			sendto(sock,&sndpacket,sizeof(sndpacket),0,(struct sockaddr *)&serverAddr, addr_size);

			//restart timer, call select again 
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			FD_ZERO(&readfds);
			FD_SET(sock, &readfds);
			
			rv = select(sock+1, &readfds, NULL, NULL, &tv);
		}
		
		// receive acknowledgements
		PACKET rcvpacket;
		while((recvfrom(sock,&rcvpacket,sizeof(rcvpacket),0,NULL,NULL) > 0) && (rcvpacket.header.seq_ack == !seq_num)){
			// if received ack is not == seq number, must resend packet 
			printf("error receiving packet. will try to receive again. seq num: %d, given seq num: %d\n",seq_num, rcvpacket.header.seq_ack); 
			// will resend until the acknowledgement is correct
			while(sendto(sock,&sndpacket,sizeof(sndpacket),0,(struct sockaddr *)&serverAddr,addr_size) == -1){
				printf("error sending retransmit packet.\n");
			}
			//once out of while loop, retransmit worked successfully
		}
		//received correct ack, now onto next packet, so update sequence number
		seq_num = (seq_num+1)%2;
	}
	// close input file since data tranmission is complete
	fclose(f_in);

	//empty packet to send after done transmitting data
	HEADER header;
	header.seq_ack = 0;
	header.length = 0;
	header.checksum = 0;

	PACKET endpacket;
	endpacket.header = header;
	strcpy(endpacket.data,"");

	printf("created empty packet\n");
	
	int counter = 0;

	// limit the amount of resends of the empty packet	
	while(counter < 4 && (sendto(sock,&endpacket,sizeof(endpacket),0,(struct sockaddr *)&serverAddr,addr_size) == -1)){
		counter++;
		printf("error sending last packet to server.  will try again.\n");
	}
	
	close(sock);
	return 0;
}
