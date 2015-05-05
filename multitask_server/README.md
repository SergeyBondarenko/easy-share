<b>To compile fork version:</b>

<code>gcc -g -o easyshare-server-fork easyshare-server-fork.c CreateTCPServerSocket.c AcceptTCPConnection.c DieWithError.c HandleTCPClent.c </code>

<b>To compile fork version with constrained multitasking:</b>

gcc -g easyshare-server-fork-limitp.c AcceptTCPConnection.c CreateTCPServerSocket.c DieWithError.c HandleTCPClent.c TCPServer.h -o easyshare-server-fork-limitp

<b>To compile thread version:</b>

gcc -pthread -g -o easyshare-server-thread easyshare-server-thread.c CreateTCPServerSocket.c AcceptTCPConnection.c DieWithError.c HandleTCPClent.c

<b>OS:</b>
Linux version 3.13.0-51-generic (buildd@lamiak) 
(gcc version 4.8.2 (Ubuntu 4.8.2-19ubuntu1) ) #84-Ubuntu SMP Wed Apr 15 12:08:34 UTC 2015

