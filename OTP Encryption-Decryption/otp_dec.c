/*
 * Author: Tanner Quesenberry
 * Date: 8/18/17
 * References:
 * 	Class notes, Piazza posts, 4.2 Verified sending
 * 	https://stackoverflow.com/questions/1479386/is-there-afunction-in-c-that-will-return-the-index-of-a-char-in-a-char-array
 * 	https://stackoverflow.com/questions/9206091/going-through-a-text-file-line-by-line-in-c
 *      www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner
 *      www.linuxhowtos.org/C_C++/socket.htm?userrate=1
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/ioctl.h>

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256];
    
        // Was < 3
	if (argc < 4) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args

        // Get user specified files
        char* textFileName = argv[1];
        char* keyFileName = argv[2];
        // Create arrays to hold data, max file is about 70000.
        // If length were unknown, use dynamic allocation.
        char text[80000];
        char key[80000];
        memset(text, '\0', sizeof(text));
        memset(key, '\0', sizeof(key));

        // Open plaintext file
        FILE* textFD = fopen(textFileName, "r");
        if(textFD < 0){
            fprintf(stderr, "Unable to open %s\n", textFileName);
            exit(1);
        }

        // Read plaintext file into char array
        fgets(text, sizeof(text), textFD);
        //Close File
        fclose(textFD);
        // Locate and remove added newline
        char toLocate = '\n';
        char* ptr = strchr(text, toLocate);
        // index also serves as the character count
        int textCount = ptr - text;
        text[textCount] = '\0';

        // Open key file
        FILE* keyFD = fopen(keyFileName, "r");
        if(keyFD < 0){
            fprintf(stderr, "Unable to open %s\n", keyFileName);
            exit(1);
        }

        // Read key file into char array
        fgets(key, sizeof(key), keyFD);
        // Close file
        fclose(keyFD);
        // Locate and remove added newline, index also serves as char count
        ptr = strchr(key, toLocate);
        int keyCount = ptr - key;
        key[keyCount] = '\0';

        // Check that the character count of the key file > plaintext file
        if(keyCount < textCount){
            fprintf(stderr, "Error: Key file shorter than plaintext file.\n");
            exit(1);
        }

        // Check plaintext for illegal characters
        int i;
        for(i = 0; i < textCount; i++){
            if(text[i] != ' ' && !(text[i] >= 'A' && text[i] <= 'Z')){
                fprintf(stderr, "Error: Illegal character(s) in plaintext.\n");
                exit(1);
            }
        }
        // Check key for illegal characters
        for(i = 0; i < keyCount; i++){
            if(key[i] != ' ' && !(key[i] >= 'A' && key[i] <= 'Z')){
                fprintf(stderr, "Error: Illegal character(s) in key.\n");
                exit(1);
            }
        }
        // Then unable to find port, report error to stderr and exit with value 2

        // If otp_enc tries to connect to otp_dec_d, reject each other, report error to stderr and terminate.

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string  (was argv[2])
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0){
            fprintf(stderr, "CLIENT: ERROR opening socket\n");
	}

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to address
            fprintf(stderr, "CLIENT: ERROR connecting\n");
        }

	// Get input message from user
//	printf("CLIENT: Enter text to send to the server, and then hit enter: ");
//	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
//	fgets(buffer, sizeof(buffer) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
//	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds

        // Verify connection to otp_enc_d, send id, check response
        char verify[] = "otp_dec";
        charsWritten = send(socketFD, verify, strlen(verify), 0);
        // PUT SOME ERROR CHECKING HERE, DON"T FORGET
        memset(buffer, '\0', sizeof(buffer));
        charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
        if(strcmp(buffer, "accepted") != 0){
            fprintf(stderr, "Error: Connection to otp_dec_d denied.\n");
            exit(1);
        }

	// Send message to server
	charsWritten = send(socketFD, text, strlen(text), 0); // Write to the server, (text was buffer)
	if (charsWritten < 0) fprintf(stderr, "CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(text)) printf("CLIENT: WARNING: Not all data written to socket!\n");

        // Check that all data is out of buffer and sent
        // Used from the 4.2 Verified sending class notes
        int checkSend = -5;
        do {
            ioctl(socketFD, TIOCOUTQ, &checkSend);
        }while(checkSend > 0); // Loop until send buffer for socket is empty
        if(checkSend < 0)
            fprintf(stderr, "iotcl error"); // Check if stopped by error

        // Get confimation from server 
        memset(buffer, '\0', sizeof(buffer));
        charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);

        // Send key
        charsWritten = send(socketFD, key, strlen(key), 0);
        if(charsWritten < 0) fprintf(stderr, "CLIENT: ERROR writing to socket");
        if(charsWritten < strlen(key)) fprintf(stderr, "CLIENT: WARNING: Not all data writen to socket!\n");

        // Get ciphertext back
	// Get return message from server
	memset(text, '\0', sizeof(text)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, text, sizeof(text) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) fprintf(stderr, "CLIENT: ERROR reading from socket");
	printf("%s\n", text);

	close(socketFD); // Close the socket
	return 0;
}
