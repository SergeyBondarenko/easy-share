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

	memset(buffer, 0, sizeof(buffer));

	// Receive message from client
	//if((bytes_recvd = recv(clntSock, buffer, sizeof(buffer), 0)) < 0)
	//	DieWithError("recv() failed\n");

	int all_bytes_recvd = 0;
	while(all_bytes_recvd != sizeof(buffer)){
		bytes_recvd = recv(clntSock, (buffer + all_bytes_recvd), (sizeof(buffer) - all_bytes_recvd), 0);

		if(bytes_recvd < 0)
			DieWithError("recv() UPLOAD msg failed!\n");
		if(bytes_recvd == 0)
			printf("Received UPLOAD status msg!\n");

		all_bytes_recvd += bytes_recvd;
		printf("STAT: Received %ld B, remaining data = %ld B\n", bytes_recvd, (sizeof(buffer) - all_bytes_recvd));
	}

	if(!strncmp(buffer, "UPLOAD", 6)){
		parseARGS(header, buffer);
		file_name = header[1];
		file_size = atoi(header[2]);

		//char *file_buffer = malloc(file_size);
		//char file_buffer[file_size];
		//bzero(file_buffer, file_size);
		//int chunk_size = 256;
		//char file_buffer[chunk_size];
		//memset(file_buffer, 0, sizeof(file_buffer)); 
		//bytes_total = file_size;

		aFile = fopen(file_name, "wb");
		if(aFile == NULL)
			DieWithError("failed to open the file!\n");
	
		//char file_buffer[file_size];
		memset(buffer, 0, sizeof(buffer));

		all_bytes_recvd = 0;
		//while(all_bytes_recvd != sizeof(file_buffer)){
		//	bytes_recvd = recv(clntSock, (file_buffer + all_bytes_recvd), (sizeof(file_buffer) - all_bytes_recvd), 0);

		//	if(bytes_recvd < 0)
		//		DieWithError("recv() UPLOAD msg failed!\n");
		//	if(bytes_recvd == 0)
		//		printf("Received UPLOAD status msg!\n");

		//	all_bytes_recvd += bytes_recvd;
		//	printf("UPLOAD: Received %ld B, remaining data = %ld B\n", bytes_recvd, (sizeof(file_buffer) - all_bytes_recvd));

		//	if((bytes_wrote = fwrite((file_buffer + all_bytes_recvd), sizeof(char), bytes_recvd, aFile)) < 0)
		//		DieWithError("fwrite() failed!\n");
		//}

		while((bytes_recvd = recv(clntSock, buffer, sizeof(buffer), 0)) > 0){
			all_bytes_recvd += bytes_recvd;
			printf("Received %ld B, remaining data = %ld B\n", bytes_recvd, (file_size - all_bytes_recvd));

			if((bytes_wrote = fwrite(buffer, sizeof(char), bytes_recvd, aFile)) < 0)
				DieWithError("fwrite() failed!\n");
		}

		//while((bytes_recvd = recv(clntSock, file_buffer, sizeof(file_buffer), 0)) > 0){
		//	bytes_total -= bytes_recvd;
		//	printf("Received %ld B from file's data, remaining data = %ld\n", bytes_recvd, bytes_total);

		//	if((bytes_wrote = fwrite(file_buffer, sizeof(char), bytes_recvd, aFile)) < 0)
		//		DieWithError("fwrite() failed!\n");
		//}

		fclose(aFile);	
		//free(file_buffer);
	
	} else {
		printf("Not UPLOAD\n");
	}
	
	
}
