#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>

#define TESTLOCAL "127.0.0.1"

// default port 31337

int DieWithError(char *msg, int socket)
{
	printf("%s\n", msg);
	close(socket);
	return 1;
}

int parseARGS(char **args, char *line)
{
	int tmp = 0;
	args[tmp] = strtok(line, ":");
	while((args[++tmp] = strtok(NULL, ":")) != NULL);
	return tmp - 1;
}

int FileUpload(int socketDESC, char *lfile, char *rfile)
{
	int ch;
	int count1=1,count2=1, percent;
	char toSEND[1];
	char remoteFILE[4096];
	FILE *file_to_send;

	// Open local file to copy
	file_to_send = fopen (lfile,"r");
	if(!file_to_send) {
		printf("Error opening file\n");
		close(socketDESC);
		return 0;
	} else {
		long fileSIZE;
		fseek(file_to_send, 0, SEEK_END);								// Set the file position of the stream to THE END 
		fileSIZE = ftell(file_to_send);									// Returns current position (END) of the stream
		rewind(file_to_send);												// Set the file position to the BEGIN

		// Get info of a sending file and send to the server
		sprintf(remoteFILE,"UPLOAD:%s:%ld\r\n", rfile, fileSIZE); 
		send(socketDESC, remoteFILE, sizeof(remoteFILE), 0);

		// Send file, charcter by character
		percent = fileSIZE / 100;											// Calculate one percent from the total file size
		while((ch=getc(file_to_send))!=EOF){							// Get characters one by one until EOF
			toSEND[0] = ch;
			send(socketDESC, toSEND, 1, 0);								// Send character to the server
			if( count1 == count2 ) {										// Print sending status if the next 1% has been sent	
				printf("33[0;0H");
				printf( "\33[2J");
				printf("Filename: %s\n", lfile);
				printf("Filesize: %ld KB\n", fileSIZE / 1024);
				printf("Percent : %d%% ( %d KB)\n",count1 / percent ,count1 / 1024);
				count1+=percent;												// Increment count1 to 1%
			}
			count2++;															// Increment count2 up to count1 size (up to 1% of the file size)

		}
	}
	fclose(file_to_send);												// Close the file
	close(socketDESC);													// Close the socket
	return 0;
}

int FileDownload(int socketDESC, char *lfile, char *rfile)
{
	int received = 0;
	char *filename, *filesize;
	char *header[4096]; 
	char recvBUFF[4096], remoteFILE[4096];
	FILE *recvFILE;

	sprintf(remoteFILE, "DOWNLOAD:%s\r\n", lfile);
	if(!send(socketDESC, remoteFILE, sizeof(remoteFILE), 0)){
		DieWithError("FileDownload send() failed!\n", socketDESC);
	}

	if(recv(socketDESC, recvBUFF, sizeof(recvBUFF), 0)){
		printf("%s\n", recvBUFF);		

		recvBUFF[strlen(recvBUFF) - 2] = 0;
		parseARGS(header, recvBUFF);
		filename = header[1];
		filesize = header[2];

		printf("Filename: %s\n", filename);
		printf("Filesize: %d KB\n", atoi(filesize) / 1024);

		recvBUFF[0] = 0;
		recvFILE = fopen(rfile, "w");
		while(1){
			if(recv(socketDESC, recvBUFF, 2, 0) != 0){
				fwrite(recvBUFF, sizeof(recvBUFF[0]), 1, recvFILE);
				received++;
				recvBUFF[0] = 0;
			} else {
				printf("Done %s [ %d of %s Bytes]\n", rfile, received, filesize);
				return 0; 
			}
		}
	} else {	
		DieWithError("FileDownload recv() failed!\n Server droppped connection.", socketDESC);
	}

}

int FileSendReceive(char *serverIP, char *PORT, char *CMD, char *lfile, char *rfile)
{
	unsigned short serverPORT;
	int socketDESC;
	struct sockaddr_in serverADDRESS;

	// Create a reliable stream socket using TCP
	socketDESC = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketDESC < 0) {
		printf("Cannot create socket\n");
		return 1;
	}
	
	// Construct server address structure
	memset(&serverADDRESS, 0, sizeof(serverADDRESS));			// Zero out structure
	serverADDRESS.sin_family = AF_INET;								// Set internet address family
	serverADDRESS.sin_addr.s_addr = inet_addr(serverIP);		// Set server ip addr (in bin format)
	serverPORT = atoi(PORT);											// Convert port string to int
	serverADDRESS.sin_port = htons(serverPORT);					// Set server port (in big endian format)

	// Establish connection to server
	if (connect(socketDESC, (struct sockaddr *) &serverADDRESS, sizeof(serverADDRESS)) < 0) {
		printf("Cannot connect\n");
		return 1;
	}

	// Run option
	if(!strcmp(CMD, "upload")){
		FileUpload(socketDESC, lfile, rfile);
	} else if(!strcmp(CMD, "download")){
		FileDownload(socketDESC, lfile, rfile);
	} else {
		printf("Possible commands: upload, download\n");
		return 1;
	}

return 0;
}

int main(int argc, char* argv[])
{
	// Get values from arguments: Server Addr, Port, Local File, Dest File

	if(argc != 6){
		printf("Usage: %s <SERVER IP ADDR> <PORT> <upload/download> <FILE> <NEW FILE>\n", argv[0]);
		return 1;
	}

	FileSendReceive(argv[1], argv[2], argv[3], argv[4], argv[5]);

	return 0;
}
