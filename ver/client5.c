#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RCVBUFSIZE 255 // Size of receive buffer

void DieWithError(char *errorMessage); // Error handling function

int main(int argc, char *argv[])
{
	int sock;								// Socket descriptor
	struct sockaddr_in servAddr;	// Echo server address
	unsigned short servPort;		// Echo server port
	char *servIP;							// Server IP addr
	char *myCommand, *commandOpt;
	char *echoString;						//	String to send to echo server
	char buffer[RCVBUFSIZE];		// Buffer for echo string
	unsigned int echoStringLen, commandStringLen;		// Length of string to echo
	
	int bytesRcvd, totalBytesRcvd;		// Bytes (B) read in single recv() and total B read

	if((argc < 3) || (argc > 5)){
		fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>] <Command>\n", argv[0]);
		exit(1);
	}

// List of Commands:
//// time
//// echo
//// cli
//// download <file name>
//// upload <file name>
	
	servIP = argv[1];						// Server IP addr
	//echoString = argv[2];				// String to echo

	if(argc >= 4){
		servPort = atoi(argv[2]);	// Use given port if any 
		myCommand = argv[3];
		if(argc == 5)
			commandOpt = argv[4];
	} else
		servPort = 7;					// 7 is well known port for echo service

	// Create a reliable, stream socket using TCP
	if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed!");
	
	// Construct the server address structure
	memset(&servAddr, 0, sizeof(servAddr)); 	// Zero out structure
	servAddr.sin_family = AF_INET;						// Internet addr family
	servAddr.sin_addr.s_addr = inet_addr(servIP);	// Server IP addr. inet_addr transforms IP addr to bin format
	servAddr.sin_port = htons(servPort);			// Server port. htons make sure bytes are stored in big endian 

	// Establish connection to echo server
	if(connect(sock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
		DieWithError("connect() failed!");

	//echoStringLen = strlen(echoString);			// Determine input length	
	commandStringLen = strlen(myCommand);			// Determine input length	
	
	// Send string to the server
	if(send(sock, myCommand, commandStringLen, 0) != commandStringLen)
		DieWithError("send() failed, sent a different number of bytes than expected");

	// Receive the same string back from the server
	totalBytesRcvd = 0;
	printf("Received: ");							// Setup to print the echoed string
	while(totalBytesRcvd < commandStringLen){
		// Receive up to buffer size (-1 to leave space for a null terminator)
		// bytes from the sender
		if((bytesRcvd = recv(sock, buffer, RCVBUFSIZE - 1, 0)) <= 0)
			DieWithError("recv() failed or connection closed prematurely");
		totalBytesRcvd += bytesRcvd;				// Keep tally of total bytes
		buffer[bytesRcvd] = "\0";				// Terminate the string
		printf("%s\n", buffer);							// Print the echo buffer
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
