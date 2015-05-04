
Project Description:

Architecture based on ARM Trustzone.
The client just sends a request to the server for a file which is stored on the server.
The server is logically divided into two: Normal Server and Secure Server.
Normal Server acts like a proxy between the client and secure server.
Normal Server initiates the socket connection with client using SSL and another socket connection with secure server using SSH/SSL.
The secure server reads the request from normal server and encrypts the file with a key.
The encrypted file is sent over SSH/SSL from secure to normal server and back to the client over SSL.
The client decrypts the encrypted file using the same key.

The key is available in both client and secure server.

Note: Mainly focused on Ubuntu OS. Use of openssl and c/c++ programming language
