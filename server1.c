#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int is_connected(int sock)
{
    unsigned char buf;
    int err = recv(sock,&buf,1,MSG_PEEK);
    return err == -1 ? 0 : 1;
}

//Client socket structure, track fo client socknmbr,ip and username 
struct cltinf {
	int socknmbr;
	char ip[INET_ADDRSTRLEN];
	char username[100];
};

//Pthread mutex initializer
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Client status , socknmbr stored in the arrays
int clients[100];
int clientStatus[100] = {0};  //check client is connected or not
int clientFirstStep[100] = {0};
int clientUserIdVerify[100] = {0};
int numberOfAttemps[100] = {0};
int n = 0;

//Messages exchange between client and server
char msgUsername[] ="admin";
char msgHello[] = "Hello";
char msgCredential[] = "Enter ID\n";
char msgConnected[] = "Connection succesfull, now can start chatting\n";
char msgConfirmation[] = "Confirm?";
char msgWarning[] = "Warning ! Wrong ID\n";
char password[] = "asdf1234";
char msgReverify[] = "User ID is wrong! \nStart with Hello again.\n";
char msgWrongPassword[] = "Wrong Password!\n"; 
char msgEnterPassword[] = "Enter Password\n";
char msgPressEnter[] = "Press Enter->";
char msgPassworrdAttemp1[] = "Try Agian..  1 Attempt left!\nEnter Password:";
char msgPassworrdAttemp2[] = "Try Again..  2 Attempts left!\nEnter Password:";


//Send message only to a particular client
void sendtoClient(char *msg, int curr)
{
	pthread_mutex_lock(&mutex);
	char res[100];
	//Append "Server:" at the starting of every message send my server to client
	char server[] =  "Server";
	strcpy(res,server);
	strcat(res,":");
	strcat(res,msg);
	send(curr,res,strlen(res),0);
	pthread_mutex_unlock(&mutex);
}

//Messages received from client , receive by the server and send to all clients
void sendtoallClient(char *msg,int curr,char *username)
{
	int i;
	pthread_mutex_lock(&mutex);
	printf("%s:send a message\n",username);
	for(i = 0; i < n; i++) {
		//Message send to all clients except the sender
		if(clients[i] != curr) {
			if(send(clients[i],msg,strlen(msg),0) < 0) {
				perror("sending failure");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}

//Start/create Receive message thread when a client sent a request to 
//connect to the server and a message to send to all clients
void *rcvmsg(void *sock)
{
	struct cltinf cl = *((struct cltinf *)sock);
	char msg[500];
	int len;
	int i;
	int j;

	int k;
	int f=0;

	// if len == 0 that means client is disconnected 
	while((len = recv(cl.socknmbr,msg,500,0)) > 0) {
		msg[len] = '\0';
		//Get the client number , to track the clients status i.e.,it is connected/in connecting state
		for(i = 0; i < n; i++) {
		if(clients[i] == cl.socknmbr) {
			break;
			}
		}
		char recMsg[100];
		char username[100];
		j=0;
		int u;
		//Take the message out from the msg string received
		// ex: client sent:   Alan: Hello
		//Extract Hello from the message received and store it in recMsg
		//extract username from the message and store it in username
		for(int k=0;k<len;k++)
		{
			if(f==1){
				recMsg[j] = msg[k];
				j++;
			}
			else
			{
				username[k] = msg[k];
			}
			if(msg[k] == ':'){
				f=1;
				u = k;
			}
		}
		username[u] = '\0';

		strcpy(cl.username,username);

		//Check if client is connected send message to all connected clients
		if(clientStatus[i] == 1 && j>1) {
			sendtoallClient(msg,cl.socknmbr,username);
		}
		f = 0;
		recMsg[j-1] = '\0';

		// Check if client is not authented yet i.e in connecting state
		// check if message is empty or not
		if(clientStatus[i] == 0 && j>1)
		{
			int isHello = strcmp(msgHello,recMsg);
			int isCredential = strcmp(msgUsername,recMsg);
			int isPassword = strcmp(password,recMsg);
			//printf("isCredential %d\n",isCredential);
			
			//After userID ask for password
		    if(isPassword == 0 && clientUserIdVerify[i] == 1)
			{
				sendtoClient(msgConnected,cl.socknmbr);
				printf("Password Successfully verified! Client connected:%s\n",cl.ip);
				clientStatus[i] = 1;
			}
			
			//If Password is wrong
			if(isPassword != 0 && clientUserIdVerify[i] == 1 && clientStatus[i] == 0)
			{
				sendtoClient(msgWrongPassword,cl.socknmbr);
				
				clientFirstStep[i] = 0;		
				numberOfAttemps[i]++;
				if(numberOfAttemps[i] == 1)
					sendtoClient(msgPassworrdAttemp2,cl.socknmbr);
				if(numberOfAttemps[i] == 2)
					sendtoClient(msgPassworrdAttemp1,cl.socknmbr);
				if(numberOfAttemps[i] == 3)
				{
					numberOfAttemps[i] = 0;
					clientUserIdVerify[i] = 0;	
				}
				printf("Password is Wrong!\n");			
			}

			//After "Hello" message, send the userID to authenticate
			if(isCredential == 0 && clientFirstStep[i] == 1)
			{
				//printf("Username Received From Client\n");
				sendtoClient(msgEnterPassword,cl.socknmbr);
				printf("Userid verified\n");
				clientUserIdVerify[i] = 1;		
			}

			// UserID was entered wrong
			if(isCredential != 0 && clientFirstStep[i] == 1 && clientUserIdVerify[i] == 0)
			{
				sendtoClient(msgReverify,cl.socknmbr);
				printf("Userid is wrong!\n");
				clientFirstStep[i] = 0;		
			}

			//if message send by the client is Hello i.e. client is ready to authenticate
			if(strcmp(msgHello,recMsg)==0)
			{
				//printf("Hello Received From Client\n");
				sendtoClient(msgCredential,cl.socknmbr);
				clientFirstStep[i] = 1;	
			}

			if(clientStatus[i] == 1)
			{
				clientFirstStep[i] = 0;
				clientUserIdVerify[i] = 0;	
				numberOfAttemps[i] = 0;			
			}
		}
		
		memset(msg,'\0',sizeof(msg));
	}

	//If client is disconnected then delete the socket and update client status
	pthread_mutex_lock(&mutex);
	printf("%s with username :%s is disconnected\n",cl.ip,cl.username);
	for(i = 0; i < n; i++) {
		if(clients[i] == cl.socknmbr) {
			j = i;
			while(j < n-1) {
				clients[j] = clients[j+1];
				clientStatus[j] = clientStatus[j+1];
				numberOfAttemps[j] = numberOfAttemps[j+1];
				j++;
			}
		}
	}
	n--;
	pthread_mutex_unlock(&mutex);
}
int main(int argc,char *argv[])
{
//sockaddr structure is used to define a socket address which is used in the bind(), connect()
	struct sockaddr_in my_addr, their_addr;

	//store the client socket
	int my_sock;

	//server socket from which message is received and address size
	int their_sock;
	int their_addr_size;

	//Port number of server on which client is connected
	int portno;

	//pthread sender and receiver thread
	pthread_t sendt,recvt;

	char msg[500];
	int len;

	//Store client info in the client structure
	struct cltinf cl;
	char ip[INET_ADDRSTRLEN];

	//Take input > PortNo
	if(argc > 2) {
		printf("too many arguments");
		exit(1);
	}
	portno = atoi(argv[1]);

	//Initialise the socket
	my_sock = socket(AF_INET,SOCK_STREAM,0);

	//Socket creation
	memset(my_addr.sin_zero,'\0',sizeof(my_addr.sin_zero));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(portno);

	//Start the server at 127.0.0.1
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	their_addr_size = sizeof(their_addr);

	//bind the socket address 
	if(bind(my_sock,(struct sockaddr *)&my_addr,sizeof(my_addr)) != 0) {
		printf("binding unsuccessful");
		exit(1);
	}

	//Listen at the ip and portno
	if(listen(my_sock,5) != 0) {
		printf("listening unsuccessful");
		exit(1);
	}

	printf("Server is listening at %d\n",portno);

	while(1) {
		//Now wait for a new connection
		//If there  is a connection request, accept
		if((their_sock = accept(my_sock,(struct sockaddr *)&their_addr,&their_addr_size)) < 0) {
			printf("accept unsuccessful");
			exit(1);
		}


		//Mutex lock, so that till it is disconnectd
		pthread_mutex_lock(&mutex);

		//Get socket /client info wanted to connect/connected
		inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
		printf("%s :Wants to connect to the server\n",ip);
		cl.socknmbr = their_sock;

		//Send a message to client to send hello to start the connection request
		char msgVerify[] = "Hello from Server, Send Hello to start the authentication process\n";
		
		//Send message to the client	
		send(cl.socknmbr,msgVerify,sizeof(msgVerify),0);
		strcpy(cl.ip,ip);
		clients[n] = their_sock;
		n++;

		//create the function thread to receive and send message
		pthread_create(&recvt,NULL,rcvmsg,&cl);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}
