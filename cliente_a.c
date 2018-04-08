
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <termios.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_add
#include <pthread.h>

int ADDRESS = 8888;
int ARRIVAL = 0;
int LOCAL_ID = -1;

char getch() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
                perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
                perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0)
                perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
                perror ("tcsetattr ~ICANON");
        return (buf);
}


void *read_key(void *status){

    int *status_ptr = (int *)status;
	
	while(*status_ptr != 1){	
		int ch = getch();
	
		if (ch < 0){
				clearerr(stdin);
			if (ferror(stdin)){
			}
		}

		if (ch == 27){
			
			++(*status_ptr);
			printf("\nFinishing...\n");
			fflush(stdout);
			
		}
	}
	return NULL;

};


void *send_process(void *vals){


	int *args = (int *) vals;
	struct timeval tv;
	gettimeofday(&tv, NULL); 
	
	time_t finish;
	time_t begin;
	
	begin=tv.tv_usec;	

	int sock = args[0];
	int lmin_b = args[1];
	int lmax_b = args[2];
	
	int lmin_w = args[3];
	int lmax_w = args[4];
	
		
	srand(time(NULL));
	int burst = (rand() % (lmax_b - lmin_b + 1))+lmin_b;
	int priority = (rand() % 5)+1;
	int wait_time = (rand() % (lmax_w - lmin_w +1))+lmin_w;
	
	int params[3];
	int reply;

	params[0] = burst;
	params[1] = priority;
	params[2] = wait_time;
	
	//Send a number
	if( send(sock , &params , sizeof(params) , 0) < 0)
	{
		puts("Connection to the server closed. Exiting the application...");
		return NULL;
	}
	
	//Receive a reply from the server
	if( recv(sock , &reply , sizeof(reply) , 0) <= 0)
	{
		puts("Connection to the server lost. Exiting the application...");
		return NULL;
	}
	
	
	int PID = ntohl(reply);
	if(PID == LOCAL_ID){
		printf("Closing connection...");
		return NULL;
	}
	LOCAL_ID = PID;
	printf("Process with PID %d created. Arrived at %d seconds\n", PID, ARRIVAL);
	ARRIVAL = ARRIVAL + wait_time;	
	gettimeofday(&tv, NULL); 
	finish = tv.tv_usec;
	usleep((wait_time * 1000000)-(finish - begin));
	return NULL;
}


int display_help(){
	printf("RUN THE PROGRAM WITH THE FOLLOWING FORMAT:\ncliente_c <server ip address> <burst min> <burst max> <wait min> <wait max>\nEXAMPLE: cliente_c 127.0.0.1 1 10 2 5\n");

}
int main(int argc , char *argv[])
{
	


	int sock;
	int status = 0;
	struct sockaddr_in server;
	
    
    if (argc < 6 ){

		display_help();
        return 1;

    }        

	int min_b = atoi(argv[2]);
	int max_b = atoi(argv[3]);
	int min_w = atoi(argv[4]);
	int max_w = atoi(argv[5]);

	if(min_b > max_b){
		printf("Burst limits invalid");
		return 1;	
	}
 	
	
	if(min_w > max_w){
		printf("Wait limits invalid");
		return 1;	
	}
 	
	int args[5];
    
	
    //Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");
	


	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_family = AF_INET;
	server.sin_port = htons( ADDRESS );

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return 1;
	}
	
	printf("Connected to %s\n",argv[1]);
	
	args[0] = sock;
	args[1] = min_b;
	args[2] = max_b;
	args[3] = min_w;
	args[4] = max_w;
	
	
	pthread_t key_reader;
	if(pthread_create(&key_reader, NULL, read_key, &status)){
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	//keep communicating with server
	while(status != 1)
	{
		pthread_t p;
		if(pthread_create(&p, NULL, send_process, &args)){
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
		pthread_join(p, NULL);	
				
		
	}

	close(sock);	
	pthread_join(key_reader, NULL);	
	
	return 0;
}

