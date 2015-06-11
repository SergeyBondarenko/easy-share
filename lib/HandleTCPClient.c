#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFSIZE 4096

int parseARGS(char **args, char *line){
   int tmp=0;
   args[tmp] = strtok( line, ":" );
   while ( (args[++tmp] = strtok(NULL, ":" ) ) != NULL );
   return tmp - 1;
}

void HandleTCPClient(int clntSock)
{
	char buffer[BUFFSIZE], *file_name, *header[BUFFSIZE];
	long bytes_recvd, bytes_wrote, file_size, bytes_total;
	FILE *aFile;

	// Receive message from client
	if((bytes_recvd = recv(clntSock, buffer, sizeof(buffer), 0)) < 0)
		DieWithError("recv() failed\n");

	if(!strncmp(buffer, "UPLOAD", 6)){
		parseARGS(header, buffer);
		file_name = header[1];
		file_size = atoi(header[2]);

		//char *file_buffer = malloc(file_size);
		char file_buffer[file_size];
		bzero(file_buffer, file_size);
		bytes_total = file_size;

		aFile = fopen(file_name, "wb");
	
		while((bytes_recvd = recv(clntSock, file_buffer, file_size, 0)) > 0){
			bytes_wrote = fwrite(file_buffer, sizeof(char), bytes_recvd, aFile); 
			bytes_total -= bytes_wrote;
			printf("Received %ld B from file's data, remaining data = %ld\n", bytes_recvd, bytes_total);
		}

		//bytes_recvd = 0;
		////if((bytes_recvd = recv(clntSock, file_buffer, file_size, 0)) < 0)
		////	DieWithError("recv() failed\n");
		//printf("\n-----\n");
		//while(((bytes_recvd = recv(clntSock, file_buffer, file_size, 0)) > 0) && (bytes_total > 0)){
		//	fwrite(file_buffer, sizeof(char), bytes_recvd, aFile);
		//	bytes_total	-= bytes_recvd;
		//	printf("Received %ld B from file's data, remaining data = %ld\n", bytes_recvd, bytes_total);
		//}

		fclose(aFile);	
		//free(file_buffer);
	

	}
	
	
	
}
