#include <stdio.h>

#define BUFFSIZE 4096

void HandleTCPClient(int clntSock)
{
	char buffer[BUFFSIZE];
	int recvMsgSize;


	// Receive message from client
	if((recvMsgSize = recv(clntSock, buffer, BUFFSIZE, 0)) < 0)
		DieWithError("recv() failed!");
	
	// Send received string and receive again until end of transmission
	while(recvMsgSize > 0){
		// Echo message back to the client
		if(send(clntSock, buffer, recvMsgSize, 0) != recvMsgSize)
			DieWithError("send() failed!");

		// See if there is more data to receive
		if((recvMsgSize = recv(clntSock, buffer, BUFFSIZE, 0)) < 0)
			DieWithError("recv2() failed!");
	}
}
