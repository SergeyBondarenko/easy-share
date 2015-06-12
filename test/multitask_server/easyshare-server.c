// Fork contrained version

#include "TCPServer.h"
#include <sys/wait.h>

void ProcessMain(int servSock);

int main(int argc, char *argv[])
{
   int servSock;          // Sockets descriptors
   unsigned short servPort;      // Server port
   pid_t processID;                 // Process ID from fork()
   unsigned int processLimit;    // Number of child processes to create
   unsigned int processCt;       // Process counter

   if(argc != 3){                   // Test for correct number of args
      fprintf(stderr, "Usage %s <Server Port> <Fork Process Limit>\n", argv[0]);
      exit(1);
   }  

   servPort = atoi(argv[1]);
   processLimit = atoi(argv[2]);

   servSock = CreateTCPServerSocket(servPort);

   for(processCt=0; processCt < processLimit; processCt++){
      // Fork child process
      if((processID = fork()) < 0)
         DieWithError("fork() failed!");
      else if(processID == 0)
         ProcessMain(servSock);
   }
}

void ProcessMain(int servSock)
{
   int clntSock;

   for(;;){
      clntSock = AcceptTCPConnection(servSock);
      printf("with child process: %d\n", (unsigned int) getpid());
      //HandleTCPClient(clntSock);
      HandleTCPClientFull(clntSock);
   }  
} 
