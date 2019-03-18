// Client
// Francesca Narea

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

#define LEN_BUFF 5
int main (int, char *[]);

int main (int argc, char *argv[])
{
    FILE *src;
    int sockfd = 0, n = 0;
    char buff[LEN_BUFF];
    struct sockaddr_in serv_addr;  
    if (argc != 5)
    {
		printf ("Usage: %s <ip of server>, %s  <port>, %s <input>, %s <output> \n",argv[1], argv[2], argv[3], argv[4]);
		return 1;
    } 
    
    // set up
    memset (buff, '0', sizeof (buff));
    memset (&serv_addr, '0', sizeof (serv_addr)); 

    // open socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
		printf ("socket error\n");
		return 1;
    } 

    // set address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons (atoi(argv[2])); 

    if (inet_pton (AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
		printf ("inet_pton error\n");
		return 1;
    } 

    // connect
    if (connect (sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
    {
		printf ("connect error\n");
		return 1;
    } 
 
   // send server filename
   write(sockfd,argv[4],strlen(argv[4])+1);
   
   // recieve null character from server
   read(sockfd,buff,1);
   if (*buff != '\0'){
   	printf("Server did not respond after filename output sent.");
	return 1;
   }

   // server responded with null character, so recieved filename output 
   printf("Server sent null character.\n",buff);

   // open source file
   if((src = fopen(argv[3],"rb")) == NULL){
	printf("Cannot open %s.\n",argv[3]);
	return 1;
  }
   
   int items_read = 1;
   while (items_read != 0){
	// reading from src file
	items_read = fread(buff,sizeof(char),LEN_BUFF,src);
	// client is writing the bytes read to server
	write(sockfd, buff, items_read);
   } 
   // completed transferring the file, so close the input file
   fclose(src);

   // completed interacting with server, so close client socket
   close (sockfd);
   return 0;
}
