#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFSIZE 4096 // Size of receive buffer

int parseARGS(char **args, char *line);
void DieWithError(char *errorMessage); // Error handling function
void UploadFile(int sock, char *lfile, char *rfile);
void DownloadFile(int sock, char *lfile, char *rfile);
void SysCmd(int sock, char *myCommand);

int main(int argc, char *argv[])
{
   int i, percent_sent, sock;                      // Socket descriptor
   struct sockaddr_in servAddr;  // Echo server address
   unsigned short servPort;      // Echo server port
   char *servIP;                    // Server IP addr
   char *myCommand, *commandOpt, *lfile, *rfile;

   // Check cmd args
   if(argc == 6 || argc == 4){
      servIP = argv[1];                // Server IP addr
      servPort = atoi(argv[2]);
      myCommand = argv[3];
      if(argc == 6){
         lfile = argv[4];
         rfile = argv[5];
      }
   } else {
      fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>] <Command> <File> <New File>\n", argv[0]);
      exit(1);
   }

   // Create a reliable, stream socket using TCP
   if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      DieWithError("socket() failed!");

   // Construct the server address structure
   memset(&servAddr, 0, sizeof(servAddr));   // Zero out structure
   servAddr.sin_family = AF_INET;                  // Internet addr family
   servAddr.sin_addr.s_addr = inet_addr(servIP);   // Server IP addr. inet_addr transforms IP addr to bin format
   servAddr.sin_port = htons(servPort);         // Server port. htons make sure bytes are stored in big endian 

   // Establish connection to echo server
   if(connect(sock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
      DieWithError("connect() failed!");

   // Select a cmd command
   if(!strcmp(myCommand, "upload") && argc == 6)
      UploadFile(sock, lfile, rfile);
   else if(!strcmp(myCommand, "download") && argc == 6)
      DownloadFile(sock, lfile, rfile);
   else if(!strcmp(myCommand, "dir") && argc == 4)
      SysCmd(sock, myCommand);
   else{
      printf("Available commands: dir, upload, download.\nExamples:\n-----\n");
      printf("./client <Server IP> <Server PORT> dir\n");
      printf("./client <Server IP> <Server PORT> upload localfile.txt remotefile.txt\n");
      printf("./client <Server IP> <Server PORT> download remotefile.txt localfile.txt\n-----\n");
   }

   // Close the socket
   close(sock);
   exit(0);

}

void DieWithError(char *errorMsg)
{
   perror(errorMsg);
   exit(1);
}

void UploadFile(int sock, char *lfile, char *rfile)
{  
   int percent_sent, chunk_size = 256;
   long all_bytes_sent, bytes_sent, bytes_read, bytes_left, file_size;
   char buffer[BUFFSIZE];      // Buffer for echo string
   FILE *aFile;

   // Open file for read in binary mode
   aFile = fopen(lfile, "rb");
   if(!aFile)
      DieWithError("fopen() failed!\n");

   // Set pos indicator to the END of file 
   // and return curr pos      
   fseek(aFile, 0, SEEK_END);
   file_size = ftell(aFile);
   rewind(aFile);

   bytes_left = file_size;

   // Init buffer with 0s
   // then fill it with file data
   memset(buffer, 0, sizeof(buffer));
   sprintf(buffer, "UPLOAD:%s:%ld\r\n", rfile, file_size);   

	// Send UPLOAD stat msg with file name and size.
	// Repeat while amount of all sent Bytes equals 4096 Bytes
   all_bytes_sent = 0;
   while(all_bytes_sent != sizeof(buffer)){
      bytes_sent = send(sock, (buffer + all_bytes_sent), (sizeof(buffer) - all_bytes_sent), 0);

      if(bytes_sent < 0)
         DieWithError("send() UPLOAD msg failed!\n");

      all_bytes_sent += bytes_sent;
   }

   //// Allocate dyn memmory to store file before sending 
   //char *file_buffer = (char *)malloc(chunk_size * sizeof(char));  
   //if(NULL == file_buffer){
   // fprintf(stderr, "malloc failed\n");
   // return(-1);
   //}

   printf("\n-----\n");

   // Loop until all Bytes of the file will be send
   while(1){
      char file_buffer[chunk_size];
      memset(file_buffer, 0, sizeof(file_buffer));

      // Read file to file_buffer
      if((bytes_read = fread(file_buffer, sizeof(char), chunk_size, aFile)) < 0)
         DieWithError("fread() failed!\n");

      // Send file over socket
      if(bytes_read > 0){
         if((bytes_sent = send(sock, file_buffer, bytes_read, 0)) < 0)
            DieWithError("send() after fread() failed!");

         // Calc percentage and display status
         bytes_left -= bytes_sent;
         percent_sent = ((file_size - bytes_left) * 100) / file_size;
         printf("Sent %d%% (%ld B), remaining = %ld B\n", percent_sent, bytes_sent, bytes_left);
      }

      // Check the end of file
      // and if it is the end - break loop
      if(bytes_read < chunk_size){
         if(feof(aFile))
            printf("End of file.\n");
         if(ferror(aFile))
            printf("Error reading!\n");
         break;
      }
   }

   // Close the file
   fclose(aFile);
   //free(file_buffer);

   printf("\n");
}

void DownloadFile(int sock, char *lfile, char *rfile)
{
   //printf("Download func is under construction!\n");
   //exit(1);
   long all_bytes_sent, bytes_sent, bytes_read, bytes_left, file_size;
   long all_bytes_recvd, bytes_recvd, bytes_written;
   char buffer[BUFFSIZE];      // Buffer for echo string
	char *file_name, *header[BUFFSIZE];
   FILE *aFile;

	// Create status message for DOWNLOAD
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "DOWNLOAD:%s:%d\r\n", lfile, 1);

	// Send status msg to request file for DOWNLOAD.
	// Repeat while amount of all received Bytes equals 4096 Bytes.
	all_bytes_sent = 0;
   while(all_bytes_sent != sizeof(buffer)){
      bytes_sent = send(sock, (buffer + all_bytes_sent), (sizeof(buffer) - all_bytes_sent), 0);

      if(bytes_sent < 0)
         DieWithError("send() DOWNLOAD msg failed!\n");

      all_bytes_sent += bytes_sent;
   }

	// Receive stat message to request file size for DOWNLOAD.
	// Repeat while amount of all received Bytes equals 4096 Bytes.
	memset(buffer, 0, sizeof(buffer));
	all_bytes_recvd = 0;
   while(all_bytes_recvd != sizeof(buffer)){
      bytes_recvd = recv(sock, (buffer + all_bytes_recvd), (sizeof(buffer) - all_bytes_recvd), 0);

      if(bytes_recvd < 0)
         DieWithError("recv() STAT msg failed!\n");
      if(bytes_recvd == 0){
         printf("Received STAT msg!\n");
			break;
		}

      all_bytes_recvd += bytes_recvd;
      printf("STAT: Received %ld B, remaining data = %ld B\n", bytes_recvd, (sizeof(buffer) - all_bytes_recvd));
   }

	// Parse received stat msg to get file name and size
	parseARGS(header, buffer);	
	//file_name = header[1]; 						// It is not used. Instead "rfile" used.
	file_size = atoi(header[2]);

	// Open file stream to write in bin mode
	aFile = fopen(rfile, "wb");
	if(aFile == NULL)
		DieWithError("failed to open the file!\n");
	
	// Receive file via socket, place it in 4096 Byte array 
	// than write buffer content into file.
	// Repeat while amount of all received Bytes equals file size. 
	memset(buffer, 0, sizeof(buffer));
	all_bytes_recvd = 0;
	while(all_bytes_recvd != file_size){
		bytes_recvd = recv(sock, buffer, sizeof(buffer), 0);
		if(bytes_recvd < 0)
			DieWithError("failed to receive the file!\n");		
		
		if((bytes_written = fwrite(buffer, sizeof(char), bytes_recvd, aFile)) < 0)
			DieWithError("failed to write into the file!\n");

		all_bytes_recvd += bytes_recvd;
		printf("Received %ld B, remaining data = %ld B\n", bytes_recvd, (file_size - all_bytes_recvd));
	}	

	// Close file stream
	fclose(aFile);
}


void SysCmd(int sock, char *myCommand)
{
   printf("System commands func is under construction!\n");  
   exit(1);
	//long bytes_recvd, bytes_sent;
	//char buffer[BUFFSIZE];

	//// Create and send status message for DOWNLOAD
	//sprintf(buffer, "EXEC:%s:%d\r\n", myCommand, 1);
	//if((bytes_sent = send(sock, buffer, sizeof(buffer), 0)) < 0)
	//	DieWithError("send() failed!\n");

	//bzero(buffer, sizeof(buffer));

	//if((bytes_recvd = recv(sock, buffer, sizeof(buffer), 0)) < 0)
	//	DieWithError("recv() failed!\n");

	//printf("Files in . dir:\n%s\n", buffer);
	//bzero(buffer, sizeof(buffer));
}

int parseARGS(char **args, char *line)
{
   int tmp = 0;
	// Parse line to get args[] elements by ":" delimeter
   args[tmp] = strtok(line, ":");
   while ((args[++tmp] = strtok(NULL, ":")) != NULL);
   return tmp - 1;
}

