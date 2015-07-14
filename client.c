#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RCVBUFSIZE 4096 // Size of receive buffer

void DieWithError(char *errorMessage); // Error handling function

int main(int argc, char *argv[])
{
	int i, percent_sent, sock;								// Socket descriptor
	struct sockaddr_in servAddr;	// Echo server address
	unsigned short servPort;		// Echo server port
	char *servIP;							// Server IP addr
	char *myCommand, *commandOpt, *lfile, *rfile;
	char *echoString;						//	String to send to echo server
	char buffer[RCVBUFSIZE];		// Buffer for echo string
	unsigned int echoStringLen, commandStringLen;		// Length of string to echo
	int bytesRcvd, totalBytesRcvd;		// Bytes (B) read in single recv() and total B read
	long bytes_recvd, bytes_sent, bytes_read, bytes_total, bytes_left, file_size;
	FILE *aFile;

	if((argc != 6)){
		fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>] <Command> <File> <New File>\n", argv[0]);
		exit(1);
	}

	servIP = argv[1];						// Server IP addr
	servPort = atoi(argv[2]);
	myCommand = argv[3];
	lfile = argv[4];
	rfile = argv[5];

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

	// Open file for read in binary mode
	aFile = fopen(lfile, "rb");
	if(!aFile)
		DieWithError("fopen() failed!\n");
	
	fseek(aFile, 0, SEEK_END);
	file_size = ftell(aFile);
	rewind(aFile);			

	memset(buffer, 5, sizeof(buffer));
	sprintf(buffer, "UPLOAD:%s:%ld\r\n", rfile, file_size);

	int all_bytes_sent = 0;
	while(all_bytes_sent != sizeof(buffer)){
		bytes_sent = send(sock, (buffer + all_bytes_sent), (sizeof(buffer) - all_bytes_sent), 0);

		if(bytes_sent < 0)
			DieWithError("send() UPLOAD msg failed!\n");

		all_bytes_sent += bytes_sent;
	}

	//char *file_buffer = (char *)malloc(chunk_size * sizeof(char));	
	//if(NULL == file_buffer){
	//	fprintf(stderr, "malloc failed\n");
	//	return(-1);
	//}

	int chunk_size = 256;
	char file_buffer[chunk_size];
	bytes_total = bytes_left = file_size;

	printf("\n-----\n");
	while(1){
		char file_buffer[chunk_size];
		memset(file_buffer, 0, sizeof(file_buffer));

		if((bytes_read = fread(file_buffer, sizeof(char), chunk_size, aFile)) < 0)
			DieWithError("fread() failed!\n");

		if(bytes_read > 0){
			if((bytes_sent = send(sock, file_buffer, bytes_read, 0)) < 0)
				DieWithError("send() after fread() failed!");
			
			bytes_left -= bytes_sent;
			percent_sent = ((bytes_total-bytes_left)*100)/bytes_total;
			printf("Sent %d%% (%ld B), remaining = %ld B\n", percent_sent, bytes_sent, bytes_left);
		}

		if(bytes_read < chunk_size){
			if(feof(aFile))
				printf("End of file.\n");
			if(ferror(aFile))
				printf("Error reading!\n");
			break;
		}
	}

	//memset(file_buffer, 0, sizeof(file_buffer));
	//while((bytes_read = fread(file_buffer, sizeof(char), sizeof(file_buffer), aFile)) > 0){
	//	if((bytes_sent = send(sock, file_buffer, bytes_read, 0)) < 0)
	//		DieWithError("send() after fread() failed!");

	//	bytes_left -= bytes_sent;
	//	percent_sent = ((bytes_total-bytes_left)*100)/bytes_total;

	//	printf("Sent %d%% (%ld B), remaining = %ld B\n", percent_sent, bytes_sent, bytes_left);
	//	memset(file_buffer, '0', sizeof(file_buffer));
	//}	

	fclose(aFile);
	//free(file_buffer);

	printf("\n");

	close(sock);
	exit(0);

}

void DieWithError(char *errorMsg)
{
	perror(errorMsg);
	exit(1);
}
