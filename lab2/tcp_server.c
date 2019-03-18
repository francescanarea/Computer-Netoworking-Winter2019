// Server
// Francesca Narea

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

int main (int, char *[]); 
#define LEN_BUFF 10

int main (int argc, char *argv[])
{
     // error if user does not enter the port #
     if(argc != 2){
        printf("Usage: <port #>");
        return 1;
     }
   
    // set up
    FILE *dst;
    char *p; 
    int listenfd = 0, sock = 0, n;
    struct sockaddr_in serv_addr; 
    char buff[LEN_BUFF];

    memset (&serv_addr, '0', sizeof (serv_addr));
    memset (buff, '0', sizeof (buff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    serv_addr.sin_port = htons (atoi(argv[1])); 

    // create socket, bind, and listen
   
    // creating socket
    if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) < 0){
		printf ("socket error\n");
		return 1;
	}
	// binding the server to a particular address
	if (bind (listenfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr)) < 0){
		printf ("bind error\n");
		return 1;
	}

	// if theres too many pending connections, then listening denied
	if (listen (listenfd, 10) < 0){
		printf ("listen error\n");
		return 1;
	}

	// accept and interact
        if ((sock = accept (listenfd, (struct sockaddr*)NULL, NULL)) < 0){
			printf ("accept error\n");
			return 1;
		} 

	// recieve file destination name
	if(read(sock,buff,sizeof(buff)) < 0){
		printf("Filename error\n");
		return 1;
	}
	// sending null character back to client to respond saying that recieved file 
	if(write(sock,"\0",1) < 0){
		printf("Error sending null character");
		return 1;
	}

	// open file dst
	if((dst = fopen(buff,"wb")) == NULL){
		printf("Error opening destination file");
		return 1;
	}
	// recieve data from src file, then write to dst file
	while((n = read(sock,buff,LEN_BUFF)) > 0){
		fwrite(buff,sizeof(char),n,dst);
	}
	// completed writing to dst so close file
        fclose(dst);

	// interaction with client complete so close server socket
	close(sock);
	
     }
