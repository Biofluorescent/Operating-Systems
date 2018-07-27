/*
 * Author: Tanner Quesenberry
 * Date: 8/18/17
 * References:
 * 	Class notes, Piazza posts (specifically technique outline for encrypting)
 * 	https://stackoverflow.com/questions/1479386/is-there-afunction-in-c-that-will-return-the-index-of-a-char-in-a-char-array
 * 	https://stackoverflow.com/questions/9206091/going-through-a-text-file-line-by-line-in-c
 *      www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner
 *      www.linuxhowtos.org/C_C++/socket.htm?userrate=1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

void encrypt();
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
char buffer[80000];
char filter[80000];

int main(int argc, char *argv[])
{
	int listenSocketFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
//	char buffer[80000];
	struct sockaddr_in serverAddress, clientAddress;
        char toAccept[] = "otp_enc";
        char allow[] = "accepted";
        char deny[] = "deny";


	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) fprintf(stderr, "ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		fprintf(stderr, "ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections


        // Create a newSocket to receive info from
        int receiveSocketFD;
        
        while(true){

	    // Accept a connection, blocking if one is not available until one connects
       	    sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
	    receiveSocketFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
	    if (receiveSocketFD < 0) fprintf(stderr, "ERROR on accept");

            // Fork off a child for this connection
            pid_t childPID = fork();

            // Process the child
            if(childPID == 0){
                // Accept only from otp_enc
                memset(buffer, '\0', sizeof(buffer));
                charsRead = recv(receiveSocketFD, buffer, 79999, 0);
                if(strcmp(buffer, toAccept) != 0){
                    charsRead = send(receiveSocketFD, deny, sizeof(deny), 0); // Send rejection
                    fprintf(stderr, "Wrong client connection to server. Rejected.\n");
                    
                }else{
                    charsRead = send(receiveSocketFD, allow, sizeof(allow), 0); // Send acceptance
                }


	        // Get the message from the client
	        memset(buffer, '\0', sizeof(buffer));
	        charsRead = recv(receiveSocketFD, buffer, 79999, 0); // Read the client's message from the socket
	        if (charsRead < 0) fprintf(stderr, "ERROR reading from socket");
	        // printf("SERVER: I received this from the client: \"%s\"\n", buffer);

                // Send confirmation
                charsRead = send(receiveSocketFD, allow, sizeof(allow), 0);

                // Receive key
                memset(filter, '\0', sizeof(filter));
                charsRead = recv(receiveSocketFD, filter, 79999, 0);
                if(charsRead < 0) fprintf(stderr, "ERROR reading from socket");
                // printf("SERVER: I received this key from client: %s\n", filter);

                // Encrypt data
                encrypt();

	        // Send a Success message back to the client
	        //charsRead = send(receiveSocketFD, "I am the server, and I got your message", 39, 0); // Send success back
	        charsRead = send(receiveSocketFD, buffer, sizeof(buffer), 0);
	        if (charsRead < 0) fprintf(stderr, "ERROR writing to socket");
	        close(receiveSocketFD); // Close the existing socket which is connected to the client
            }else{
                // If not the child, continue and wait for a new client connection
                continue;
            }
        }

	close(listenSocketFD); // Close the listening socket
	return 0; 
}

void encrypt(){
    char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    // et = encrypt text, ek = encrypt key
    int i, et, ek;
    // t = text letter, k = key letter
    char t, k;
    // Find end of string
    char* ptr = strchr(buffer, '\0');
    int length = ptr - buffer;

    // Encrypt the length of the string
    for(i = 0; i < length; i++){
        // Get text letter
        t = buffer[i];
        if(t < 'A'){
            et = 26;
        }else{
            et = t - 65;
        }

        // Get key letter
        k = filter[i];
        if(k < 'A'){
            ek = 26;
        }else{
            ek = k - 65;
        }

        // Make sure it is in letters array
        et = et + ek;
        if(et > 26){
            et = et - 27;
        }
        // Encrypt
        buffer[i] = letters[et];
    }
}
