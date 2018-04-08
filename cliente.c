
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


int send_process(int sock){

	struct timeval tv;
	gettimeofday(&tv, NULL); 
	
	time_t finish;
	time_t begin;
	
	begin=tv.tv_usec;	

	srand(time(NULL));
	int burst = (rand() % 6)+1;
	int priority = (rand() % 5)+1;
	int wait_time = (rand() % 3)+3;
	int params[3];
	int reply;

	params[0] = burst;
	params[1] = priority;
	params[2] = wait_time;
	
	//Send a number
	if( send(sock , &params , sizeof(params) , 0) < 0)
	{
		puts("Send failed");
		return 1;
	}
	
	//Receive a reply from the server
	if( recv(sock , &reply , sizeof(reply) , 0) < 0)
	{
		puts("recv failed");
		return -1;
	}
	
	
	int PID = ntohl(reply);
	printf("Process with PID %d created. Arrived at %d seconds\n", PID, ARRIVAL);
	ARRIVAL = ARRIVAL + wait_time;	
	gettimeofday(&tv, NULL); 
	finish = tv.tv_usec;
	usleep((wait_time * 1000000)-(finish - begin));
	return PID;
}


int main(int argc , char *argv[])
{
	
	int sock;
	int params[2];
	int status = 0;
	struct sockaddr_in server;
	
    
    if (argc < 2 ){

		printf("IP Address needed\n");
        return 1;

    }        
    	
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
	
	puts("Connected\n");
	

	pthread_t key_reader;
	if(pthread_create(&key_reader, NULL, read_key, &status)){
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	//keep communicating with server
	while(status != 1)
	{
		int pid = send_process(sock);
		if (pid == -1){
			printf("Connection lost\n");
			close(sock);	
			return 1;
		}
	}

	close(sock);	
	pthread_join(key_reader, NULL);	
	
	return 0;
}

