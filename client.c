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
void SysCmd(int sock, char *myCommand, char *myArgs);
void ShowExamples(void);

int main(int argc, char *argv[])
{
   int i, percent_sent, sock;                      // Socket descriptor
   struct sockaddr_in servAddr;  // Echo server address
   unsigned short servPort;      // Echo server port
   char *servIP;                    // Server IP addr
   char *myCommand, *myArgs, *commandOpt, *lfile, *rfile;

   // Check cmd args
   if(argc == 6 || argc == 5){
      servIP = argv[1];                // Server IP addr
      servPort = atoi(argv[2]);
      myCommand = argv[3];
		
		if(!strcmp(myCommand, "dir"))
			myArgs = argv[4];

      //if(argc == 6){
		if(!strcmp(myCommand, "upload") || !strcmp(myCommand, "download")){
         lfile = argv[4];
         rfile = argv[5];
      }
   } else {
		ShowExamples();
      exit(1);
   }

   // Prevent buffer overflow
   for(i = 0; i < argc; i++)
      if(strlen(argv[i]) > BUFFSIZE){
         printf("Too many characters in \"%s\" argument.\nYou are trying to overflow the buffer!\nExit.\n", argv[i]);
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
   else if(!strcmp(myCommand, "dir") && argc == 5)
      SysCmd(sock, myCommand, myArgs);
   else
		ShowExamples();
   

   // Close the socket
   close(sock);
   exit(0);

}

void ShowExamples(void)
{
	printf("Available commands: dir, upload, download.\nExamples:\n-----\n");
	printf("./client <Server IP> <Server PORT> dir <PATH>\n");
	printf("./client <Server IP> <Server PORT> upload localfile.txt remotefile.txt\n");
	printf("./client <Server IP> <Server PORT> download remotefile.txt localfile.txt\n-----\n");
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
	// Repeat until amount of all sent Bytes equals 4096 Bytes
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
            printf("Finished UPLOAD.\n");
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
	int percent_recvd;
   long all_bytes_sent, bytes_sent, bytes_read, bytes_left, file_size;
   long all_bytes_recvd, bytes_recvd, bytes_written;
   char buffer[BUFFSIZE];      // Buffer for echo string
	char *file_name, *header[BUFFSIZE];
   FILE *aFile;

	// Create status message for DOWNLOAD
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "DOWNLOAD:%s:%d\r\n", lfile, 1);

	// Send status msg to request file for DOWNLOAD.
	// Repeat until amount of all received Bytes equals 4096 Bytes.
	all_bytes_sent = 0;
   while(all_bytes_sent != sizeof(buffer)){
      bytes_sent = send(sock, (buffer + all_bytes_sent), (sizeof(buffer) - all_bytes_sent), 0);

      if(bytes_sent < 0)
         DieWithError("send() DOWNLOAD msg failed!\n");

      all_bytes_sent += bytes_sent;
   }

	// Receive stat message to request file size for DOWNLOAD.
	// Repeat until amount of all received Bytes equals 4096 Bytes.
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
      //printf("STAT: Received %ld B, remaining data = %ld B\n", bytes_recvd, (sizeof(buffer) - all_bytes_recvd));
   }

	// Parse received stat msg to get file name and size
	parseARGS(header, buffer);	
	//file_name = header[1]; 						// It is not used. Instead "rfile" used.
	file_size = atoi(header[2]);
	bytes_left = file_size;

	// Open file stream to write in bin mode
	aFile = fopen(rfile, "wb");
	if(aFile == NULL)
		DieWithError("failed to open the file!\n");
	
	// Receive file via socket, place it in 4096 Byte array 
	// than write buffer content into file.
	// Repeat until amount of all received Bytes equals file size. 
	memset(buffer, 0, sizeof(buffer));
	all_bytes_recvd = 0;
	while(all_bytes_recvd != file_size){
		bytes_recvd = recv(sock, buffer, sizeof(buffer), 0);
		if(bytes_recvd < 0)
			DieWithError("failed to receive the file!\n");		
		
		if((bytes_written = fwrite(buffer, sizeof(char), bytes_recvd, aFile)) < 0)
			DieWithError("failed to write into the file!\n");

		all_bytes_recvd += bytes_recvd;
		//printf("Received %ld B, remaining data = %ld B\n", bytes_recvd, (file_size - all_bytes_recvd));

      // Calc percentage and display status
      bytes_left -= bytes_recvd;
      percent_recvd = ((file_size - bytes_left) * 100) / file_size;
      printf("Received %d%% (%ld B), remaining = %ld B\n", percent_recvd, bytes_recvd, bytes_left);
	}	

	if(all_bytes_recvd == file_size)
		printf("Finished DOWNLOAD.\n");

	// Close file stream
	fclose(aFile);
}

// EXEC commands
void SysCmd(int sock, char *myCommand, char *myArgs)
{
	int i = 0;
	long bytes_recvd, all_bytes_recvd, bytes_sent, all_bytes_sent;
	char buffer[BUFFSIZE], *header[BUFFSIZE];

	// Create status message for EXEC command
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "EXEC:%s:%s:1\r\n", myCommand, myArgs); // 1 - to delimit args from \r\n

	// Send the status message
	all_bytes_sent = 0;
	while(all_bytes_sent != sizeof(buffer)){
		if((bytes_sent = send(sock, (buffer + all_bytes_sent), (sizeof(buffer) - all_bytes_sent), 0)) < 0)
			DieWithError("send() failed!\n");
		
		all_bytes_sent += bytes_sent;
	}

	memset(buffer, 0, sizeof(buffer));

	// Receive response for EXEC command
	all_bytes_recvd = 0;
	while(all_bytes_recvd != sizeof(buffer)){
		if((bytes_recvd = recv(sock, (buffer + all_bytes_recvd), (sizeof(buffer) - all_bytes_recvd), 0)) < 0)
			DieWithError("recv() failed!\n");
	
		if(bytes_recvd < 0)
			DieWithError("failed to receive the message from server!\n");		
		
		all_bytes_recvd += bytes_recvd;
	}

	memset(header, 0, BUFFSIZE);

	// Parse buffer to copy members to header array 
	parseARGS(header, buffer);

	// Print output
	while(header[i] != NULL){
		printf("%s\n", header[i]);
		i++;
	}

	// Clean both arrays
	memset(buffer, 0, sizeof(buffer));
	memset(header, 0, BUFFSIZE);
}

int parseARGS(char **args, char *line)
{
   int tmp = 0;
	// Parse line to get args[] elements by ":" delimeter
   args[tmp] = strtok(line, ":");
   while ((args[++tmp] = strtok(NULL, ":")) != NULL);
   return tmp - 1;
}

