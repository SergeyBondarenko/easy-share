#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define PORT 31337
#define BUFFSIZE 4096

int parseARGS(char **args, char *line)
{
	int tmp=0;
	args[tmp] = strtok( line, ":" );                           // Set token to ":"
	// Fill up "args" array with parts of the "line" string (replacing ":" with NULL)
	while ( (args[++tmp] = strtok(NULL, ":" ) ) != NULL );
	return tmp - 1;
}

int fileDOWNLOAD(int connectSOCKET, char *infoBUFF)
{

	char *lfilename, *rfilename, *filesize;
	FILE *sendFILE;                             // File pointer
	char *header[BUFFSIZE];                     // Array to store file info (filename, filesize)
	char localFILE[BUFFSIZE];
	
	infoBUFF[strlen(infoBUFF) - 2] = 0;         // Mark end of string by eliminating last '/n'
	parseARGS(header, infoBUFF);                // Parse infoBUFF and copy fields in header[]
	lfilename = header[1];
	rfilename = header[2];
	
	printf("Filename: local -> %s remote -> %s\n", lfilename, rfilename);
	
	sendFILE = fopen(lfilename, "rb");
	if(!sendFILE){
		printf("fileDOWNLOAD() Error openning file!\n");
		return 1;
	} else {
		long fileSIZE, sent_bytes, total_bytes, read_bytes;
		fseek(sendFILE, 0, SEEK_END);
		fileSIZE = ftell(sendFILE);
		rewind(sendFILE);
	
		sprintf(localFILE, "DOWNLOAD:%s:%ld\r\n", rfilename, fileSIZE);
		
		total_bytes = 0;
		while(total_bytes != sizeof(localFILE)){
			if((sent_bytes = send(connectSOCKET, localFILE, sizeof(localFILE), 0)) < 0)
				printf("fileDOWNLOAD() Failed to send file info!\n");
			total_bytes += sent_bytes;
		}
	
		total_bytes = fileSIZE;
		long fileSLICE = fileSIZE/10; 
		char *buffer = malloc(fileSLICE);
		do{
			read_bytes = fread(buffer, 1, fileSLICE, sendFILE);
			if((sent_bytes = send(connectSOCKET, buffer, read_bytes, 0)) < 0)
				printf("fileDOWNLOAD() failed to send file!\n");

			total_bytes -= sent_bytes;
	      printf("To send: %ld\n", total_bytes);
	      printf("Data sent: %ld\n", sent_bytes);
	      printf("---\n");
			//printf("Total sent: %ld%% (%ld B); Now sent: %ld B\n", (total_bytes*100)/fileSIZE, total_bytes, sent_bytes);
		} while(total_bytes != 0);
		
	}
		
	fclose(sendFILE);
	
}

void fileUPLOAD(int connectSOCKET, char *infoBUFF)
{
	char *filename, *filesize;
	FILE *recvFILE;                             // File pointer
	char *header[BUFFSIZE];                     // Array to store file info (filename, filesize)
	
	infoBUFF[strlen(infoBUFF) - 2] = 0;         // Mark end of string by eliminating last '/n'
	parseARGS(header, infoBUFF);                // Parse infoBUFF and copy fields in header[]
	filename = header[1];
	filesize = header[2];
	
	printf("Filename: %s\n", filename);
	printf("Filesize: %d B\n", atoi(filesize));
	
	recvFILE = fopen(filename, "wb");
	
	int bytes_read = 0, to_read = atoi(filesize);
	char *buffer = malloc(atoi(filesize));
	
	// Receive remote file and write it into local file
	do {
	    if((bytes_read = recv(connectSOCKET, buffer, to_read, 0)) < 0)
	        printf("Failed to recv the file!\n");
	
	    printf("To read: %d\n", to_read);
	    printf("Data read: %d\n", bytes_read);
	    printf("---\n");
	    to_read -= bytes_read;
	
	    fwrite(buffer, 1, bytes_read, recvFILE);
	} while(to_read != 0);                      // Loop while there is nothing to receive
	
	fclose(recvFILE);
	
}

void *ThreadRoutine(void *ptr){
    int connectSOCKET = (intptr_t) ptr;         // Client socket, 'intptr_t' type has the same size of the 'ptr' pointer
    char infoBUFF[BUFFSIZE];                        // Buffer for data

    // Receive message from client (filename, filesize) and write it to infoBUFF[]
    if( recv(connectSOCKET, infoBUFF, sizeof(infoBUFF), 0) ){
        if(!strncmp(infoBUFF,"UPLOAD",6))
            fileUPLOAD(connectSOCKET, infoBUFF);
        else if(!strncmp(infoBUFF,"DOWNLOAD",8))
				fileDOWNLOAD(connectSOCKET, infoBUFF);
    } else {
    // If we didn't receive initialization message from clients (filename, filesize)
    printf("Client dropped connection\n");
    }

    return 0;
}


int main(int argc, char* argv[])
{
    int socketINDEX = 0;                                                    // Index of client thread, max 512

    int listenSOCKET, connectSOCKET[512];                           // Socket descriptors
    unsigned short servicePORT;                                     // Server port
    socklen_t clientADDRESSLENGTH[512];                             // Array of client addr length
    struct sockaddr_in clientADDRESS[512], serverADDRESS;       // Array of client addr and server addr
    pthread_t threads[512];                                             // Array of thread IDs, max 512

	 if(argc != 2){
		printf("Syntax: %s <Port>\n", argv[0]);
		return 1;
	 }

    // Create socket for incomming connection
    listenSOCKET = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSOCKET < 0) {
        printf("Cannot create socket\n");
        close(listenSOCKET);
        return 1;
    }

    // Construct local address structure
    serverADDRESS.sin_family = AF_INET;                             // Internet address family
    serverADDRESS.sin_addr.s_addr = htonl(INADDR_ANY);          // Any incomming interface in bigendian format
    servicePORT = atoi(argv[1]);                                        // Get service port
    serverADDRESS.sin_port = htons(servicePORT);                    // Set service port in bigendian format

    // Bind to local address
    if (bind(listenSOCKET, (struct sockaddr *) &serverADDRESS, sizeof(serverADDRESS)) < 0) {
        printf("Cannot bind socket\n");
        close(listenSOCKET);
        return 1;
    }

    // Mark the socket to listen for the incomming connections
    listen(listenSOCKET, 5);

    clientADDRESSLENGTH[socketINDEX] = sizeof(clientADDRESS[socketINDEX]); // Set addr length of the current client

    while(1){
        // Wait for client to connect
        connectSOCKET[socketINDEX] = accept(listenSOCKET, (struct sockaddr *) &clientADDRESS[socketINDEX], &clientADDRESSLENGTH[socketINDEX]);
        if (connectSOCKET[socketINDEX] < 0) {
            printf("Cannot accept connection\n");
            close(listenSOCKET);
            return 1;
        }

        // Create client thread and run "client" routine in the new thread
        pthread_create( &threads[socketINDEX], NULL, ThreadRoutine, (void *) (intptr_t) connectSOCKET[socketINDEX]);
        //pthread_create( &threads[socketINDEX], NULL, client, connectSOCKET[socketINDEX]);

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

	return 0;
}
