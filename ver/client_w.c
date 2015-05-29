#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TESTLOCAL "127.0.0.1"

// default port 31337

int fileSEND(char *serverIP, char *PORT, char *lfile, char *rfile){

    unsigned short serverPORT;
    int socketDESC;
    int ch;
    int count1=1,count2=1, percent;
    char toSEND[1];
    char remoteFILE[4096];
    struct sockaddr_in serverADDRESS;
    struct hostent *hostINFO;
    FILE *file_to_send;

    // Create a reliable stream socket using TCP
    socketDESC = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketDESC < 0) {
        printf("Cannot create socket\n");
        return 1;
    }

    // Construct server address structure
    memset(&serverADDRESS, 0, sizeof(serverADDRESS));           // Zero out structure
    serverADDRESS.sin_family = AF_INET;                             // Set internet address family
    serverADDRESS.sin_addr.s_addr = inet_addr(serverIP);        // Set server ip addr (in bin format)
    serverPORT = atoi(PORT);                                            // Convert port string to int
    serverADDRESS.sin_port = htons(serverPORT);                 // Set server port (in big endian format)

    // Establish connection to server
    if (connect(socketDESC, (struct sockaddr *) &serverADDRESS, sizeof(serverADDRESS)) < 0) {
        printf("Cannot connect\n");
        return 1;
    }

    // Open local file to copy
    file_to_send = fopen (lfile, "rb");
    if(!file_to_send) {
        printf("Error opening file\n");
        close(socketDESC);
        return 0;
    } else {
        long fileSIZE;
        fseek(file_to_send, 0, SEEK_END);                               // Set the file position of the stream to THE END
        fileSIZE = ftell(file_to_send);                                 // Returns current position (END) of the stream
        rewind(file_to_send);                                               // Set the file position to the BEGIN

        // Get info of a sending file and send to the server
        sprintf(remoteFILE,"FBEGIN:%s:%ld\r\n", rfile, fileSIZE);
        send(socketDESC, remoteFILE, sizeof(remoteFILE), 0);

        char *buffer = malloc(fileSIZE);
        fread(buffer, 1, fileSIZE, file_to_send);

        write(socketDESC, buffer, fileSIZE);

        //// Send file, charcter by character
        //percent = fileSIZE / 100;                                         // Calculate one percent from the total file size
        //while((ch=getc(file_to_send))!=EOF){                          // Get characters one by one until EOF
        //  toSEND[0] = ch;
        //  send(socketDESC, toSEND, 1, 0);                             // Send character to the server
        //  if( count1 == count2 ) {                                        // Print sending status if the next 1% has been sent
        //      printf("33[0;0H");
        //      printf( "\33[2J");
        //      printf("Filename: %s\n", lfile);
        //      printf("Filesize: %ld B\n", fileSIZE);
        //      printf("Percent : %d%% ( %d B)\n",count1 / percent ,count1);
        //      count1+=percent;                                                // Increment count1 to 1%
        //  }
        //  count2++;                                                           // Increment count2 up to count1 size (up to 1% of the file size)

        //}
    }

    fclose(file_to_send);                                               // Close the file
    close(socketDESC);                                                  // Close the socket

return 0;
}

int main(int argc, char* argv[])
{
    // Get values from arguments: Server Addr, Port, Local File, Dest File
    fileSEND(argv[1], argv[2], argv[3], argv[4]);

    return 0;
}
