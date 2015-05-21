#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

int DieWithError(char *errMSG, int listenSOCK);
int CreateTCPServerSocket(unsigned short port, struct sockaddr_in *serverADDR);
int parseARGS(char **args, char *line);
void *ClientRoutine(void *ptr);
int ReceiveFile(char *recvBUFF, int connectSOCKET);
int SendFile(char *recvBUFF, int connectSOCKET);

int main(int argc, char* argv[])
{
	int socketINDEX = 0;													// Index of client thread, max 512
	int listenSOCKET, connectSOCKET[512];							// Socket descriptors
	unsigned short servicePORT;										// Server port	
	socklen_t clientADDRESSLENGTH[512];								// Array of client addr length 	
	struct sockaddr_in clientADDRESS[512], serverADDRESS;		// Struct of client addr and server addr	
	pthread_t threads[512];												// Array of thread IDs, max 512 


	if(argc != 2){
		printf("Usage: %s <PORT>\n", argv[0]);
		return 1;	
	}
	servicePORT = atoi(argv[1]);										// Get service port

	// Server socket: create, bind, listen
	listenSOCKET = CreateTCPServerSocket(servicePORT, &serverADDRESS);
	
	clientADDRESSLENGTH[socketINDEX] = sizeof(clientADDRESS[socketINDEX]); // Set addr length of the current client

	while(1){
		// Wait for client to connect
		connectSOCKET[socketINDEX] = accept(listenSOCKET, (struct sockaddr *) &clientADDRESS[socketINDEX], &clientADDRESSLENGTH[socketINDEX]);
		if (connectSOCKET[socketINDEX] < 0) {
			DieWithError("Cannot accept connection\n", listenSOCKET);
		}

		// Create client thread and run client routine in the new thread
		if(pthread_create( &threads[socketINDEX], NULL, ClientRoutine, (void *) (intptr_t) connectSOCKET[socketINDEX]) != 0){
			DieWithError("Cannot create new thread\n", listenSOCKET);
		}

		printf("New thread created with ID: %ld\n", (long int) threads[socketINDEX]);

		// Limit number of threads in threads[] to 512
		// In future limit number of threads
		if(socketINDEX > 511) {
			socketINDEX = 0;
		} else {
			socketINDEX++;
		}
	}

	// Close socket
	close(listenSOCKET);
}

int CreateTCPServerSocket(unsigned short port, struct sockaddr_in *serverADDR)
{
	int listenSOCKET;
	struct sockaddr_in serverADDRESS;		// Struct of server addr (local copy)
	serverADDRESS = *serverADDR;				// Make a local copy of the serverADDR struct

	// Create socket for incomming connection	
	if((listenSOCKET = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		DieWithError("Cannot create socket\n", listenSOCKET);
	}

	// Construct local address structure
	memset(&serverADDRESS, 0, sizeof(serverADDRESS));			// Zero out server address structure
	serverADDRESS.sin_family = AF_INET;								// Internet address family
	serverADDRESS.sin_addr.s_addr = htonl(INADDR_ANY);			// Any incomming interface in bigendian format
	serverADDRESS.sin_port = htons(port);					// Set service port in bigendian format 

	// Bind to local address
	if (bind(listenSOCKET, (struct sockaddr *) &serverADDRESS, sizeof(serverADDRESS)) < 0) {
		DieWithError("Cannot bind socket\n", listenSOCKET);
	}
	
	// Mark the socket to listen for the incomming connections
	if(listen(listenSOCKET, 5) < 0){
		DieWithError("Cannot listen to socket\n", listenSOCKET);
	}

	*serverADDR = serverADDRESS; 										// Copy the local copy of serverADDRESS struct back to original

	return listenSOCKET;
}

int DieWithError(char *errMSG, int listenSOCK)
{
	printf("%s\n", errMSG);
	close(listenSOCK);
	return 1;
}


int parseARGS(char **args, char *line){
	int tmp=0;
	args[tmp] = strtok( line, ":" );                           // Set token to ":"
	// Fill up "args" array with parts of the "line" string (replacing ":" with NULL)
	while ( (args[++tmp] = strtok(NULL, ":" ) ) != NULL );	  
	return tmp - 1;
}

int SendFile(char *recvBUFF, int connectSOCKET)
{
	int ch;
	int count1=1, count2=1, percent;
	char *filename, *filesize;				
	char toSEND[1];
	char *header[4096];						// Array to store file info (filename, filesize)
	char remoteFILE[4096];
	FILE *sendFILE;
	
	recvBUFF[strlen(recvBUFF) - 2] = 0;				// Mark end of string by eliminating last '/n'
	parseARGS(header, recvBUFF);						// Parse recvBUFF and copy fields in header[]
	filename = header[1];
	filesize = header[2];

	// Open local file to send to client
	sendFILE = fopen(filename, "r");
	if(!sendFILE){
		printf("Error openning file\n");
		close(connectSOCKET);
		return 1;
	} else {
		long fileSIZE;
		fseek(sendFILE, 0, SEEK_END);
		fileSIZE = ftell(sendFILE);
		rewind(sendFILE);

		// Push info about downloading file
		sprintf(remoteFILE, "DOWNLOAD:%s:%ld\r\n", filename, fileSIZE); // Send with the same filename
		if(!send(connectSOCKET, remoteFILE, sizeof(remoteFILE), 0)){
			DieWithError("Server failed to send file for the client\n", connectSOCKET);
		}
		
		//// Send file character by character
		//percent = fileSIZE / 100;
		//while(() != EOF){}	

	}
}

int ReceiveFile(char *recvBUFF, int connectSOCKET)
{
	char *filename, *filesize;				
	char *header[4096];						// Array to store file info (filename, filesize)
	FILE * recvFILE;							// File pointer
	int received = 0;							// Bytes counter

	recvBUFF[strlen(recvBUFF) - 2] = 0;				// Mark end of string by eliminating last '/n'
	parseARGS(header, recvBUFF);						// Parse recvBUFF and copy fields in header[]
	filename = header[1];
	filesize = header[2];

	printf("Filename: %s\n", filename);
	printf("Filesize: %d KB\n", atoi(filesize) / 1024);

	recvBUFF[0] = 0;												// Zero out header[]
	recvFILE = fopen ( filename,"w" );						// Open new file recvFILE for write
	while(1){
		// Receive file from client caracter by caracter and write it it recvBUFF[]
		if( recv(connectSOCKET, recvBUFF, 1, 0) != 0 ) {
			fwrite (recvBUFF , sizeof(recvBUFF[0]) , 1 , recvFILE );		// Write one received caracter to file recvFILE
			received++;																	// Increase Bytes counter by one	
			recvBUFF[0] = 0;															// Erase the caracter from buffer
		} else {
			// Print status after the transmission had been ended
			printf("Done: %s [ %d of %s Bytes]\n", filename, received, filesize);
			return 0;
		}
	}
}

void *ClientRoutine(void *ptr){
	int connectSOCKET = (intptr_t) ptr;			// Client socket, 'intptr_t' type has the same size of the 'ptr' pointer 
	char recvBUFF[4096];						// Buffer for data

	while(1){
		// Receive message from client (filename, filesize) and write it to recvBUFF[]
		if( recv(connectSOCKET, recvBUFF, sizeof(recvBUFF), 0) ){
			if(!strncmp(recvBUFF,"UPLOAD",6)) {
				ReceiveFile(recvBUFF, connectSOCKET);
			} else if(!strncmp(recvBUFF,"DOWNLOAD",8)){
				SendFile(recvBUFF, connectSOCKET);
			} else {
				// If we didn't receive initialization message from clients (filename, filesize)
				printf("Client dropped connection\n");
			}
		}
	return 0;
	}
}
