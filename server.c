/*
    C socket server example
*/
 
#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
 
int main(int argc , char *argv[])
{
		
	int client_num;
	int num;
     

    int socket_desc , client_sock , c , read_size;
    struct sockaddr_in server , client;
    
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8080 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 1);
     

	
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
   
	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0)
	{
	    perror("accept failed");
	    return 1;
	}
	puts("Connection accepted");

	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0)
	{
	    perror("accept failed");
	    return 1;
	}
	puts("Connection accepted");
	 
	 
	//Receive a number from client
	while( (read_size = recv(client_sock , &client_num , sizeof(client_num) , 0)) > 0 )
	{
		int num_show = ntohl(client_num);
		printf("%d",num_show);
		fflush(stdout);
		int send_num = htonl(num_show*2);
	    //Send the message(multiplication) back to client
	    write(client_sock , &send_num , sizeof(send_num));
	}
	 
	if(read_size == 0)
	{
	    puts("Client disconnected");
	    fflush(stdout);
	}
	else if(read_size == -1)
	{
	    perror("recv failed");
	}   
	}

    return 0;
}
