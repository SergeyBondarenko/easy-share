#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <arpa/inet.h>
//#include <sys/socket.h>

#define BUFFSIZE 4096

int parseARGS(char **args, char *line);
void UploadFile(int clntSock, char *buffer);
void DownloadFile(int clntSock, char *buffer);
void SysCmd(int clntSock, char *buffer);

void HandleTCPClient(int clntSock)
{
	char buffer[BUFFSIZE];
	long bytes_recvd, all_bytes_recvd;

	// Get initital client stat msg.
	// Repeat while amount of all received Bytes equals 4096 Byte.
	memset(buffer, 0, sizeof(buffer));
	all_bytes_recvd = 0;
	while(all_bytes_recvd != sizeof(buffer)){
		bytes_recvd = recv(clntSock, (buffer + all_bytes_recvd), (sizeof(buffer) - all_bytes_recvd), 0);

		if(bytes_recvd < 0)
			DieWithError("recv() STAT msg failed!\n");
		if(bytes_recvd == 0){
			printf("Received STAT msg!\n");
			break;
		}

		all_bytes_recvd += bytes_recvd;
		printf("STAT: Received %ld B, remaining data = %ld B\n", bytes_recvd, (sizeof(buffer) - all_bytes_recvd));
	}

	// Read first characters of stat msg to determine action:
	// Upload, Download or Exec.
	if(!strncmp(buffer, "UPLOAD", 6))
		UploadFile(clntSock, buffer);
	else if(!strncmp(buffer, "DOWNLOAD", 8))
		DownloadFile(clntSock, buffer);
	else if(!strncmp(buffer, "EXEC", 4))
		SysCmd(clntSock, buffer);
	else
		printf("Wrong STAT message!\n");
	
}

int parseARGS(char **args, char *line)
{
   int tmp=0;
	// Parse line to get args[] elements by ":" delimeter
   args[tmp] = strtok( line, ":" );
   while ( (args[++tmp] = strtok(NULL, ":" ) ) != NULL );
   return tmp - 1;
}

void UploadFile(int clntSock, char *buffer)
{
	char *file_name, *header[BUFFSIZE];
	long bytes_recvd, bytes_written, file_size, all_bytes_recvd;
	FILE *aFile;

	// Parse received buffer for file name and file size
	parseARGS(header, buffer);
	file_name = header[1];
	file_size = atoi(header[2]);

	// Open a file stream in wite bin mode
	aFile = fopen(file_name, "wb");
	if(aFile == NULL)
		DieWithError("failed to open the file!\n");
	
   // Receive file via socket, place it in 4096 Byte array 
   // than write buffer content into file.
   // Repeat while amount of all received Bytes equals file size. 
	memset(buffer, 0, BUFFSIZE);
	all_bytes_recvd = 0;
	while((bytes_recvd = recv(clntSock, buffer, BUFFSIZE, 0)) > 0){
		all_bytes_recvd += bytes_recvd;
		printf("Received %ld B, remaining data = %ld B\n", bytes_recvd, (file_size - all_bytes_recvd));

		if((bytes_written = fwrite(buffer, sizeof(char), bytes_recvd, aFile)) < 0)
			DieWithError("fwrite() failed!\n");
	}

	// Close file stream
	fclose(aFile);	
}

void DownloadFile(int clntSock, char *sbuffer)
{
	int percent_sent;
	long bytes_sent, bytes_read, file_size, all_bytes_sent, bytes_left;
	char buffer[BUFFSIZE], *file_name, *header[BUFFSIZE];
	FILE *aFile;

	// Parse buffer for file name 
	parseARGS(header, sbuffer);
	file_name = header[1];

	// Open file stream in read bin mode
	aFile = fopen(file_name, "rb");
	if(aFile == NULL)
		DieWithError("failed to open the file!\n");

	// Shift file stream indicatore to get file size
	fseek(aFile, 0, SEEK_END);
	file_size = ftell(aFile);
	rewind(aFile);

	bytes_left = file_size;
	// Prepare DOWNLOAD stat msg with file name and size 	
	memset(buffer, 0, BUFFSIZE);
	sprintf(buffer, "DOWNLOAD:%s:%ld\r\n", file_name, file_size);

	// Send DOWNLOAD stat msg via socket.
	// Repeat while amount of all sent Bytes equals 4096 Bytes. 
	all_bytes_sent = 0;
	while(all_bytes_sent != BUFFSIZE){
		bytes_sent = send(clntSock, (buffer + all_bytes_sent), (BUFFSIZE - all_bytes_sent), 0);
		if(bytes_sent < 0)
			DieWithError("send STAT msg failed!\n");		

		all_bytes_sent += bytes_sent;
		printf("STAT: Sent %ld B, remaining data = %ld B\n", bytes_sent, (BUFFSIZE - all_bytes_sent));
	}

	// Loop until all Bytes of the file will be send
	while(1){
		memset(buffer, 0, BUFFSIZE);

		// Read file into buffer
		if((bytes_read = fread(buffer, sizeof(char), BUFFSIZE, aFile)) < 0)
			DieWithError("failed to read the file!\n");	

		// Send file over socket
		if(bytes_read > 0){
			if((bytes_sent = send(clntSock, buffer, bytes_read, 0)) < 0)
				DieWithError("fialed to send the file!\n");

			// Calc percentage and display status
			bytes_left -= bytes_sent;
			percent_sent = ((file_size - bytes_left) * 100) / file_size;
			printf("Sent %d%% (%ld B), remaining = %ld B\n", percent_sent, bytes_sent, bytes_left);
		}

		// Check the end of the file
		// if it is End - break.
		if(bytes_read < BUFFSIZE){
			if(feof(aFile))
				printf("End of file.\n");
			if(ferror(aFile))
				printf("Error reading!\n");
			break;
		}
	}
	
	// Close file stream
	fclose(aFile);	

}

void SysCmd(int clntSock, char *buffer)
{
//	char *cmd_name, *file_name, *header[BUFFSIZE];
//	char local_buffer[BUFFSIZE];
//	long bytes_sent, bytes_recvd;;
//	DIR *dir;
//	struct dirent *dp;
//
//	printf("%s\n", buffer);
//	parseARGS(header, buffer);
//	cmd_name = header[1];
//
//	if(!strcmp(cmd_name, "dir")){
//		dir = opendir (".");
//		if(!dir)
//			DieWithError("opendir() failed!\n");
//		else{
//			//char *cmd_buffer = malloc(BUFFSIZE);
//			//bzero(cmd_buffer, BUFFSIZE);
//
//			while ((dp=readdir(dir)) != NULL) {
//				if ( !strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") ){
//        		    // do nothing (straight logic)
//        		} else {
//        		    file_name = dp->d_name; // use it
//					strcat(local_buffer, file_name);
//					strcat(local_buffer, ":");
//					//sprintf(cmd_buffer, "%s", dp->d_name);
//					//printf("%s\n", cmd_buffer);
//        		    //printf("file_name: \"%s\"\n",file_name);
//        		}	
//			}
//
//			printf("%s\n", local_buffer);
//			if((bytes_sent = send(clntSock, local_buffer, sizeof(local_buffer), 0)) < 0)	
//				DieWithError("send() failed!\n");
//			bzero(local_buffer, sizeof(local_buffer));
//		}
//		closedir(dir);
//	}
}
