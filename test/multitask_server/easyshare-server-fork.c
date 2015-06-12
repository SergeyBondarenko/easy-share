#include "TCPServer.h"
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	int servSock, clntSock;				// Sockets descriptors
	unsigned	short servPort;		// Server port
	pid_t processID;						// Process ID from fork()
	unsigned int childProcCount = 0;	// Number of child process

	if(argc != 2){							// Test for correct number of args
		fprintf(stderr, "Usage %s <Server Port>\n", argv[0]);
		exit(1);
	}

	servPort = atoi(argv[1]);

	servSock = CreateTCPServerSocket(servPort);

	for(;;){
		clntSock = AcceptTCPConnection(servSock);
		// Fork child process
		if((processID = fork()) < 0)
			DieWithError("fork() failed!");
		else if(processID == 0){
			close(servSock);				//	Child closes listening socket
			HandleTCPClient(clntSock);
			exit(0);							// Child process terminates	
		}

		printf("with child process: %d\n", (int) processID);
		close(clntSock);					// Parent close child socket descriptor
		childProcCount++;					// Increment number of child processes

		while(childProcCount){			// Clean up all zombies (unused childs)
			processID = waitpid((pid_t) -1, NULL, WNOHANG); 	// Nonblocking wait
			if(processID < 0)
				DieWithError("waitpid() failed!");
			else if(processID == 0)										// No zombie
				break;
			else
				childProcCount--;											// Cleaned up after a child
		}
	}
}
