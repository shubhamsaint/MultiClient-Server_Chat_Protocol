#include <sys/types.h>
#include <sys/socket.h>  //includes a number of definitions of structures needed for sockets
#include <netinet/in.h>  //contains constants and structures needed for internet domain addresses
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>


// When client receives a message from server pthread_create executed and put message on the console
void *Receive_msg(void *sock)
{
	int sckt = *((int *)sock);
	char msg[500];
	int len;
	while((len = recv(sckt,msg,500,0)) > 0) {
		msg[len] = '\0';
		fputs(msg,stdout);
		memset(msg,'\0',sizeof(msg));
	}
}

int main(int argc, char *argv[])
{
	//sockaddr structure is used to define a socket address which is used in the bind(), connect()
	struct sockaddr_in their_addr;

	//store the client socket
	int nwsock;

	//server socket from which message is received and address size
	int sckt;
	int addrsize;

	//Port number of server on which client is connected
	int portno;

	//pthread sender and receiver thread
	pthread_t send,recv;

	//msg sent to the other clients
	char msg[500];

	//name of the sender concatinate with the client message
	char senderName[100];

	//Full message with sendername + "message"  ex: Alan:Hello   ---> senderName: Alan
	char res[600];

	//Stores the ip of the server client is connected 
	char ip[INET_ADDRSTRLEN];
	int len;

	if(argc > 3) {
		printf("too many arguments");
		exit(1);
	}

	//Get portno from the user
	portno = atoi(argv[2]);

	//Copy sender name entered by user into senderName
	strcpy(senderName,argv[1]);

	//Get my sock, socket is of stream socket
	//AF_INET refers to addresses from the internet, IP addresses specifically
	nwsock = socket(AF_INET,SOCK_STREAM,0);
	memset(their_addr.sin_zero,'\0',sizeof(their_addr.sin_zero));

	//Sock_addr socket creation for server
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(portno);
	their_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//connect to the server(their_addr) using nwsock
	if(connect(nwsock,(struct sockaddr *)&their_addr,sizeof(their_addr)) < 0) {
		printf("connection not esatablished");
		exit(1);
	}

	//Return the ip address of the server on which client is connected
	inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
	printf("Wants to connect to %s,Type Yes\n",ip);
	
	//Pthread_create creates a function call 
	pthread_create(&recv,NULL,Receive_msg,&nwsock);

	//Creating message to send to the other clients through server
	while(fgets(msg,500,stdin) > 0) {
		strcpy(res,senderName);
		strcat(res,":");
		strcat(res,msg);

		len = write(nwsock,res,strlen(res));
		if(len < 0) {
			perror("message not sent");
			exit(1);
		}
		memset(msg,'\0',sizeof(msg));
		memset(res,'\0',sizeof(res));
	}
	pthread_join(recv,NULL);
	close(nwsock);
}
