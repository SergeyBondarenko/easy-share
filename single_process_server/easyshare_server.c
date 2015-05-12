#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RCVBUFSIZE 62
#define OUTBUFSIZE 62
#define MAXPENDING 5			// Max outstanding connection requests

void DieWithError(char *errMsg);			// Error handling function
void HandleTCPClient(int clntSocket);	// TCP client handling func

int main(int argc, char *argv[])
{
	int servSock, clntSock;					//	Socket descriptors for server and client
	struct sockaddr_in echoServAddr;		//	Server addr
	struct sockaddr_in echoClntAddr;		// Client addr
	unsigned short echoServPort;			// Server port
	unsigned int clntLen;					// Length of client addr data structure

	if(argc != 2){
		fprintf(stderr, "Usage %s <Server Port>\n", argv[0]);
		exit(1);
	}

	echoServPort = atoi(argv[1]);			// Local port

	// Create socket for incomming connection
	if((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed!");

	// Construct local address structure
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echoServAddr.sin_port = htons(echoServPort);

	// Bind the local addr
	if((bind(servSock, (struct sockaddr*) &echoServAddr, sizeof(echoServAddr))) < 0)
		DieWithError("bind() failed!");

	// Listen to incomming connection
	if(listen(servSock, MAXPENDING) < 0)
		DieWithError("listen() failed!");

	for(;;){
		// Set the size of in-out parameter
		clntLen = sizeof(echoClntAddr);

		// Wait for client to connect
		if((clntSock = accept(servSock, (struct sockaddr*) &echoClntAddr, &clntLen)) < 0)
			DieWithError("accept() failed!");

		// clntSock is connected to a client
		printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

		HandleTCPClient(clntSock);	
	}

	exit(0);
}


void DieWithError(char *errMsg)			// Error handling function
{
	printf("%s\n", errMsg);
	exit(1);
}

void HandleTCPClient(int clntSocket)	// TCP client handling func
{
	char echoBuffer[RCVBUFSIZE], outBuffer[OUTBUFSIZE];
	int i, recvMsgSize, sendMsgSize;
	char cmd_list[] = "list";
	char cmd_date[] = "date";
	char exmp_list[] = "Example list: a.out  cstyle_str  cstyle_str.c  cstyle_str.cpp  string_class  string_class.cpp";
	char exmp_date[] = "Example date: Tue May 12 09:15:00 CEST 2015";

	// Clean buffer before saving data into
	for(i = 0; i < RCVBUFSIZE; i++)
		outBuffer[i] = 0;

	// Receive message from client
	if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
		DieWithError("recv() failed!");
	
	// Match commands and fill buffer with output
	if(strcmp(echoBuffer, cmd_list) == 0){
		printf("%s\n", exmp_list);
		strcpy(outBuffer, exmp_list);
	} else if(strcmp(echoBuffer, cmd_date) == 0){
		printf("%s\n", exmp_date);
		strcpy(outBuffer, exmp_date);
	} else {
		printf("%s\n", echoBuffer);
		strcpy(outBuffer, echoBuffer);
	}

	sendMsgSize = strlen(outBuffer);

	// Send received string and receive again until end of transmission
	while(recvMsgSize > 0){					// 0 - end of transmission
	//while(sendMsgSize > 0){					// 0 - end of transmission
		// Echo message back to the client
		if(send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize)
		//if(send(clntSocket, echoBuffer, sendMsgSize, 0) != sendMsgSize)
			DieWithError("send() failed!");

		if(send(clntSocket, outBuffer, sendMsgSize, 0) != sendMsgSize)
		//if(send(clntSocket, echoBuffer, sendMsgSize, 0) != sendMsgSize)
			DieWithError("send2() failed!");

		// See if there is more data to receive
		//recvMsgSize = 0;
		if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
			DieWithError("recv2() failed!");
	}

	close(clntSocket);
}
