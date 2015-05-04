#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RCVBUFSIZE 32 // Size of receive buffer

void DieWithError(char *errorMessage); // Error handling function

int main(int argc, char *argv[])
{
	int sock;								// Socket descriptor
	struct sockaddr_in echoServAddr;	// Echo server address
	unsigned short echoServPort;		// Echo server port
	char *servIP;							// Server IP addr
	char *echoString;						//	String to send to echo server
	char echoBuffer[RCVBUFSIZE];		// Buffer for echo string
	unsigned int echoStringLen;		// Length of string to echo
	int bytesRcvd, totalBytesRcvd;		// Bytes (B) read in single recv() and total B read

	if((argc < 3) || (argc > 4)){
		fprintf(stderr, "Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n", argv[0]);
		exit(1);
	}
	
	servIP = argv[1];						// Server IP addr
	echoString = argv[2];				// String to echo

	if(argc == 4)
		echoServPort = atoi(argv[3]);	// Use given port if any 
	else
		echoServPort = 7;					// 7 is well known port for echo service

	// Create a reliable, stream socket using TCP
	if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed!");
	
	// Construct the server address structure
	memset(&echoServAddr, 0, sizeof(echoServAddr)); 	// Zero out structure
	echoServAddr.sin_family = AF_INET;						// Internet addr family
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);	// Server IP addr. inet_addr transforms IP addr to bin format
	echoServAddr.sin_port = htons(echoServPort);			// Server port. htons make sure bytes are stored in big endian 

	// Establish connection to echo server
	if(connect(sock, (struct sockaddr*) &echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("connect() failed!");

	echoStringLen = strlen(echoString);			// Determine input length	
	
	// Send string to the server
	if(send(sock, echoString, echoStringLen, 0) != echoStringLen)
		DieWithError("send() failed, sent a different number of bytes than expected");

	// Receive the same string back from the server
	totalBytesRcvd = 0;
	printf("Received: ");							// Setup to print the echoed string
	while(totalBytesRcvd < echoStringLen){
		// Receive up to buffer size (-1 to leave space for a null terminator)
		// bytes from the sender
		if((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
			DieWithError("recv() failed or connection closed prematurely");
		totalBytesRcvd += bytesRcvd;				// Keep tally of total bytes
		echoBuffer[bytesRcvd] = "\0";				// Terminate the string
		printf(echoBuffer);							// Print the echo buffer
	}
	
	printf("\n");

	close(sock);
	exit(0);
}

void DieWithError(char *errorMsg)
{
	perror(errorMsg);
	exit(1);
}
