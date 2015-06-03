#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFSIZE 4096
//#define TESTLOCAL "127.0.0.1"
// default port 31337

int parseARGS(char **args, char *line)
{
	int tmp = 0;
	args[tmp] = strtok(line, ":");
	while((args[++tmp] = strtok(NULL, ":")) != NULL);
	return tmp - 1;
}

int fileDOWNLOAD(int socketDESC, char *lfile, char *rfile)
{
	char remoteFILE[BUFFSIZE];
	char *header[BUFFSIZE];
	char *filename, *filesize;
	long sent_bytes, recv_bytes, total_bytes;
	FILE *recvFILE;
	
	sprintf(remoteFILE,"DOWNLOAD:%s:%s\r\n", lfile, rfile);
	
	total_bytes = BUFFSIZE;
	do {
		if((sent_bytes = send(socketDESC, remoteFILE, sizeof(remoteFILE), 0)) < 0)
			printf("fileDOWNLOAD() Failed to send file name info!\n");
		total_bytes -= sent_bytes;
	} while(total_bytes > 0);
	printf("%s\n", remoteFILE);
	
	total_bytes = BUFFSIZE;
	do {
		if((recv_bytes = recv(socketDESC, remoteFILE, sizeof(remoteFILE), 0)) < 0)
			printf("fileDOWNLOAD() Failed to receive file size info!\n");
		total_bytes -= recv_bytes;
	} while(total_bytes > 0);
	printf("%s\n", remoteFILE);
	
	remoteFILE[strlen(remoteFILE) - 2] = 0;
	parseARGS(header, remoteFILE);
	filename = header[1];
	filesize = header[2];
	
	printf("Filename: %s\n", filename);
	printf("Filesize: %d B\n", atoi(filesize));
	
	recvFILE = fopen(filename, "wb");
	
	total_bytes = atoi(filesize);
	long to_read = atoi(filesize);
	char *buffer = malloc(atoi(filesize));
	
	do{
		if((recv_bytes = recv(socketDESC, buffer, to_read, 0)) < 0)
			printf("fileDOWNLOAD() Failed to recv the file!\n");
		
		total_bytes -= recv_bytes;
		fwrite(buffer, 1, recv_bytes, recvFILE);
	   //printf("33[0;0H");
	   //printf( "\33[2J");
	   printf("Filename: %s\n", filename);
	   printf("Filesize: %ld B\n", (long)atoi(filesize));
	   printf("Total received: %ld%% (%ld B); Now received: %ld B\n---\n", 100-((total_bytes*100)/(long)atoi(filesize)), total_bytes, recv_bytes);
	} while(total_bytes > 0);

	fclose(recvFILE);

}

int fileUPLOAD(int socketDESC, char *lfile, char *rfile)
{
	char remoteFILE[BUFFSIZE];
	FILE *fileSEND;
	
	// Open local file to copy
	fileSEND = fopen (lfile, "rb");
	if(!fileSEND) {
	    printf("Error opening file\n");
	    return 1;
	} else {
	    long fileSIZE;
	    fseek(fileSEND, 0, SEEK_END);                               // Set the file position of the stream to THE END
	    fileSIZE = ftell(fileSEND);                                 // Returns current position (END) of the stream
	    rewind(fileSEND);                                           // Set the file position to the BEGIN
	
	    // Get info of a sending file and send to the server
	    sprintf(remoteFILE,"UPLOAD:%s:%ld\r\n", rfile, fileSIZE);
	    if(send(socketDESC, remoteFILE, sizeof(remoteFILE), 0) < 0)
	        printf("Failed to send file info!\n");
	
	    long bytes_sent = 0, total_sent = 0, to_send = fileSIZE, bytes_read;
	    long fileSLICE = fileSIZE/10;                               // Set slice to 10% of the file
	    char *buffer = malloc(fileSLICE);                           // Allocate memmory for the slice
	
	    // Read and send the file by slices, 10% each
	    do {
	        bytes_read = fread(buffer, 1, fileSLICE, fileSEND);
	        if((bytes_sent = send(socketDESC, buffer, bytes_read, 0)) < 0)
	            printf("Failed to send the file!\n");
	
	        printf("33[0;0H");
	        printf( "\33[2J");
	        printf("Filename: %s\n", lfile);
	        printf("Filesize: %ld B\n", fileSIZE);
	
	        total_sent += bytes_sent;
	        printf("Total sent: %ld%% (%ld B); Now sent: %ld B\n", (total_sent*100)/fileSIZE, total_sent, bytes_sent);
	        printf("---\n");
	
	        to_send -= bytes_sent;
	    } while(to_send != 0);                                      // Repeat the loop while 0 Bytes are left to send
	
	}
	
	fclose(fileSEND);                                               // Close the file
	
}

int fileTRANSFER(char *serverIP, char *PORT, char *CMD, char *lfile, char *rfile){

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
    memset(&serverADDRESS, 0, sizeof(serverADDRESS));           // Zero out structure
    serverADDRESS.sin_family = AF_INET;                         // Set internet address family
    serverADDRESS.sin_addr.s_addr = inet_addr(serverIP);        // Set server ip addr (in bin format)
    serverPORT = atoi(PORT);                                    // Convert port string to int
    serverADDRESS.sin_port = htons(serverPORT);                 // Set server port (in big endian format)

    // Establish connection to server
    if (connect(socketDESC, (struct sockaddr *) &serverADDRESS, sizeof(serverADDRESS)) < 0) {
        printf("Cannot connect\n");
        return 1;
    }

    // Execute commands
    if(!strcmp(CMD, "upload"))
        // Upload file to server
        fileUPLOAD(socketDESC, lfile, rfile);
    else if(!strcmp(CMD, "download"))
        // Download file from server
		  fileDOWNLOAD(socketDESC, lfile, rfile);
    else
        printf("Available commands: upload, download\n");

    // Close the socket
    close(socketDESC);

    return 0;
}

int main(int argc, char* argv[])
{
    // Verify syntax
    if(argc != 6){
        printf("Syntax: %s <server IP> <server PORT> <Command> <File> <New File>\n", argv[0]);
        return 1;
    }

    // Get values from arguments: Server Addr, Port, Local File, Dest File
    fileTRANSFER(argv[1], argv[2], argv[3], argv[4], argv[5]);

    return 0;
}
