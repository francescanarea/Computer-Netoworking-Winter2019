#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <limits.h>
#include <arpa/inet.h>
#include <stdlib.h>
typedef struct{
	char ip[50];
	int port;
}Machine;

#define update_min_wait 10
#define update_max_wait 20

int N = 4; //default
int costs[100][100];
int neighbor_costs[100];
int id;
int port;
int sock;
int rcv_data[3]; // host1, host2, weight
int snd_data[3]; // host1, host2, weight
Machine machines[100];
int update_num=0;
pthread_mutex_t lock;

void parse_files(FILE* costs_file, FILE* machines_file);
void *receive_updates();
void *link_state();
void send_data();
int receive_data(int port);
void user_input_cost();
int minDistance(int dist[], int visited[]);


int main(int argc, char* argv[]){	
	srand(time(NULL));
    if (argc != 5){
		printf ("Usage: %s <id> <n_machines> <costs_file> <hosts_file> \n",argv[0]);
	}
	//scan data into memory
	sscanf(argv[1],"%d",&id);
	sscanf(argv[2],"%d",&N);

    //open costs & machines files
	FILE *costs_file, *machines_file;
	costs_file = fopen(argv[3], "r");
	machines_file = fopen(argv[4], "r");

    //init mutex to synchronize access
	pthread_mutex_init(&lock, NULL);
    
    //populate costs and machines files
	parse_files(costs_file, machines_file);

	port = machines[id].port;

	//init neighbor cost table
	int i;
	for(i = 0; i<N; i++){
		neighbor_costs[i] = costs[id][i];
	}
	struct sockaddr_in myAddr;
	struct sockaddr_storage stor;
	socklen_t addr_size, other_addr_size;

	// init 
	myAddr.sin_family = AF_INET;
	myAddr.sin_port = htons(port);
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset((char *)myAddr.sin_zero, '\0', sizeof(myAddr.sin_zero));  
	addr_size = sizeof(stor);

	//create socket
	if((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("Socket error.\n");
		return 1;
	}

	// bind
	if (bind(sock,(struct sockaddr *)&myAddr, sizeof (myAddr)) != 0){
		printf ("Bind error.\n");
		return 1;
	}

	//init threads
	pthread_t receive_thr;
	pthread_t link_thr;

    pthread_create(&receive_thr, NULL, receive_updates, NULL);
	pthread_create(&link_thr, NULL, link_state, NULL);

	
	for(i=0;i<2;i++){
		user_input_cost();
		sleep(10);
	}
}

void parse_files(FILE* costs_file, FILE* machines_file){
    //costs file
	int i;
    
	for(i=0; i<N; i++){	
		int n;		
		int j;
		for(j=0; j<N; j++){
			if ((n = (fscanf(costs_file,"%d",&costs[i][j]))) != 1)
				break;
			printf("%d ", costs[i][j]);
		}
		printf("\n");
	}
    int n;
	//hosts file
	for(i=0; i<N; i++){
		if ((n =(fscanf(machines_file,"%s %d", (machines[i].ip), &(machines[i].port)))) < 1)
			break;
		printf("%s %d \n", (machines[i].ip), (machines[i].port));
	}
	return;
}

void *receive_updates(){
	while(1){

		receive_data(port);

		int host1 = ntohl(rcv_data[0]);
		int host2 = ntohl(rcv_data[1]);
		int weight = ntohl(rcv_data[2]);

		pthread_mutex_lock(&lock);
		costs[host1][host2] = weight;
		costs[host2][host1] = weight;

		int i;
		for(i=0; i<N; i++){			
			int j;
			for(j=0; j<N; j++){
				printf("%d ", costs[i][j]);
			}
			printf("\n");
		}
		pthread_mutex_unlock(&lock);
	}
}

void *link_state(){
	time_t last_update;
	last_update = time(NULL);
	int tmp_costs[N][N];

	while(1){		
		int threshold = rand()%(update_max_wait - update_min_wait) + update_min_wait;
		//calculate least costs
        if ((time(NULL) - last_update) > threshold){
			//its currently overriding the cost matrix so just need to have neighbor cost table 
			int dist[N]; 
			int visited[N];
//			int tmp_costs[N][N];
			int i,source;
            
			pthread_mutex_lock(&lock);
		//	source = id;
			for(source=0; source<N; source++){
				//initial values
				for (i=0; i< N; i++)
					dist[i] = INT_MAX, visited[i] = 0;

				dist[source] = 0; // distance to self is 0

				int count;
				for (count = 0; count < N-1; count++){	
					int u = minDistance(dist, visited);
					visited[u] = 1;
					int v;

					for (v = 0; v < N; v++){
						if (visited[v]==0 && costs[u][v] && dist[u] != INT_MAX && dist[u]+costs[u][v] < dist[v]){
		        				dist[v] = dist[u] + costs[u][v];
                       				 }
                   			 }
				}
			
				// put changes to mem
//				printf("Update cost table %d: ",source);
				for (i=0; i<N; i++){
//					printf("%d ",dist[i]);
					tmp_costs[source][i] = dist[i];
					tmp_costs[i][source] = dist[i];
				}
				printf("\n");
			}
			printf("new table for node %d: ",id);
			for(i=0; i<N; i++){
				printf("%d ", tmp_costs[id][i]);
			}
				
			printf("\n");
			pthread_mutex_unlock(&lock);
			last_update = time(NULL);
		}
	}
}

int minDistance(int dist[], int visited[]){
   // Initialize min value
   int min = INT_MAX, min_index;
   int v;
   for (v = 0; v < N; v++)
     if (visited[v] == 0 && dist[v] < min)
         min = dist[v], min_index = v;
   return min_index;
}

void send_data(){
	int sock;
	struct sockaddr_in destAddr[N];
	socklen_t addr_size[N];

	// configure address
	int i;
	for (i=0; i<N; i++){
		destAddr[i].sin_family = AF_INET;
		destAddr[i].sin_port = htons (machines[i].port);
		inet_pton (AF_INET, machines[i].ip, &destAddr[i].sin_addr.s_addr);
		memset (destAddr[i].sin_zero, '\0', sizeof (destAddr[i].sin_zero));  
		addr_size[i] = sizeof destAddr[i];
	}

	/*Create UDP socket*/
	sock = socket (PF_INET, SOCK_DGRAM, 0);
	for (i=0; i<N; i++){
		if (i != id)
			sendto (sock, &snd_data, sizeof(snd_data), 0, (struct sockaddr *)&(destAddr[i]), addr_size[i]);
	}
}

int receive_data(int port){
	int nBytes = recvfrom (sock, &rcv_data, sizeof(rcv_data), 0, NULL,NULL);
	printf("received update\n");
	return 0;
}

void user_input_cost(){
	int node;
	int cost_update;

	printf("Update:<node #><cost_update>\n");
	scanf("%d %d",&node,&cost_update); //given user input, will change the cost and update table
	pthread_mutex_lock(&lock);

	costs[id][node] = cost_update;
	costs[node][id] = cost_update;

//	neighbor_costs[node] = cost_update;
	snd_data[0] = htonl(id);
	snd_data[1] = htonl(node);
	snd_data[2] = htonl(cost_update);
	send_data();

	update_num++;

	printf("New neighbor costs: ");
	//prints out initial user input update
	int i;
	for(i=0; i<N; i++){
		printf("%d ", costs[id][i]);
	}
	printf("\n");

	/*
	int i;
	for(i=0; i<N; i++){			
		int j;
		for(j=0; j<N; j++){
			printf("%d ", costs[i][j]);
		}
		printf("\n");
	}
	*/

	pthread_mutex_unlock(&lock);
	if(update_num == 2){
		sleep(30);
		return;
	}
}
