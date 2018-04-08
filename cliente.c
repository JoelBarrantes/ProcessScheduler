/*
	C ECHO client example using sockets
*/
#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<sys/socket.h>	//socket
#include<arpa/inet.h>	//inet_addr

int main(int argc , char *argv[])
{
	int sock;
	int num, rec_num;
	struct sockaddr_in server;
	char message[1000] , server_reply[2000];
	
    
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
	server.sin_port = htons( 8080 );

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return 1;
	}
	
	puts("Connected\n");
	
	//keep communicating with server
	while(1)
	{
		printf("Enter number : ");
		scanf("%d" , &num);
		int num_send = htonl(num);
		
		//Send a number
		if( send(sock , &num_send , sizeof(num_send) , 0) < 0)
		{
			puts("Send failed");
			return 1;
		}
		
		//Receive a reply from the server
		if( recv(sock , &rec_num , sizeof(rec_num) , 0) < 0)
		{
			puts("recv failed");
			break;
		}
		
		puts("Multiplication result: ");
		printf("%d\n",ntohl(rec_num));
	}
	
	close(sock);
	return 0;
}
