#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define PORT 31337

int parseARGS(char **args, char *line){
    int tmp=0;
    args[tmp] = strtok( line, ":" );                           // Set token to ":"
    // Fill up "args" array with parts of the "line" string (replacing ":" with NULL)
    while ( (args[++tmp] = strtok(NULL, ":" ) ) != NULL );
    return tmp - 1;
}


void *client(void *ptr){
    int connectSOCKET = (intptr_t) ptr;         // Client socket, 'intptr_t' type has the same size of the 'ptr' pointer
    char recvBUFF[4096];                        // Buffer for data
    char *filename, *filesize;
    FILE * recvFILE;                            // File pointer
    int received = 0;                           // Bytes counter
    //char tempstr[4097];
    char *header[4096];                     // Array to store file info (filename, filesize)


    //while(1){
        // Receive message from client (filename, filesize) and write it to recvBUFF[]
        if( recv(connectSOCKET, recvBUFF, sizeof(recvBUFF), 0) ){
            if(!strncmp(recvBUFF,"FBEGIN",6)) {
                recvBUFF[strlen(recvBUFF) - 2] = 0;             // Mark end of string by eliminating last '/n'
                parseARGS(header, recvBUFF);                        // Parse recvBUFF and copy fields in header[]
                filename = header[1];
                filesize = header[2];

                printf("Filename: %s\n", filename);
                printf("Filesize: %d B\n", atoi(filesize));
        }

            recvFILE = fopen(filename, "wb");

            int bytes_read = 0, to_read = atoi(filesize);
            char *buffer = malloc(atoi(filesize));

            do {
                bytes_read = read(connectSOCKET, buffer, to_read);
                printf("To read: %d\n", to_read);
                printf("Data read: %d\n", bytes_read);
                to_read -= bytes_read;

                fwrite(buffer, 1, bytes_read, recvFILE);
            } while(to_read != 0);

            fclose(recvFILE);

        //recvBUFF[0] = 0;                                              // Zero out header[]
        //recvFILE = fopen ( filename,"wb" );                       // Open new file recvFILE for write
        //while(1){
        //  // Receive file from client caracter by caracter and write it it recvBUFF[]
        //  if( recv(connectSOCKET, recvBUFF, 1, 0) != 0 ) {
        //      fwrite (recvBUFF , sizeof(recvBUFF[0]) , 1 , recvFILE );        // Write one received caracter to file recvFILE
        //      received++;                                                                 // Increase Bytes counter by one
        //      recvBUFF[0] = 0;                                                            // Erase the caracter from buffer
        //  } else {
        //      // Print status after the transmission had been ended
        //      printf("Done: %s [ %d of %s Bytes]\n", filename, received, filesize);
        //      return 0;
        //    }
        //  }
        //  return 0;
        } else {
        // If we didn't receive initialization message from clients (filename, filesize)
        printf("Client dropped connection\n");
        }
    return 0;
    //}  // End of while(1)
}


int main(int argc, char* argv[])
{
    int socketINDEX = 0;                                                    // Index of client thread, max 512

    int listenSOCKET, connectSOCKET[512];                           // Socket descriptors
    unsigned short servicePORT;                                     // Server port
    socklen_t clientADDRESSLENGTH[512];                             // Array of client addr length
    struct sockaddr_in clientADDRESS[512], serverADDRESS;       // Array of client addr and server addr
    pthread_t threads[512];                                             // Array of thread IDs, max 512

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
        pthread_create( &threads[socketINDEX], NULL, client, (void *) (intptr_t) connectSOCKET[socketINDEX]);
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
}
                              