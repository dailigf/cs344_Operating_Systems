/********************************************************************************************************************************************************************
 * Author: Francis C. Dailig																	    *
 * Program: otp_dec_d																                    *
 * Description:  Program will run as daemon listening for connections.  It will receive a ciphertext and key, encrypt, then send back the plaintext.                *
 * Notes:  Received idea to use threads, mutex, and from the following link: https://www.youtube.com/watch?v=Pg_4Jz8ZIH4&t=187s                                     *
 * My Queue data structure was taken from my data Structure Class worksehh 18, 22 APR 2018.                                                                         *
 * *****************************************************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include "queue.h"

#define MAX_CONNECTIONS 5
#define NUM_THREADS 5
#define CHUNK_SIZE 512

//Global Variables
pthread_t tid[NUM_THREADS];	 //Array to hold thread IDs
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;		 //Mutex Lock
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;	//pthread condition variable
struct queue *availSocks;	 //Queue to hold available scokets


void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
void* threadRoutine(void *arg);	//thread function prototype
int isClientValid(int socketFD); //Function used to check if the client is otp_end
char* getMessage(int socketFD); //Get the message from the client
char* getPlaintext(char message[]); //function to parse out the plaintext
char* getKey(char message[]); //function to parse out the Key
char* encryptMessage(char plaintext[], char key[]);
int sendCiphertext(char ciphertext[], int socketFD);
char* decryptMessage(char plaintext[], char key[]);	//function to get the plaintext message

int main(int argc, char *argv[]){
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;
	int result;	

	//Check usage and args
	if (argc != 2){
	       	fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1);
       	} // Check usage & args

	//Initialize the queue
	availSocks = (struct queue*)malloc(sizeof(struct queue));
	queueInit(availSocks);

	//Create Threads
	int i;
	for(i = 0; i < NUM_THREADS; i++){
		result = pthread_create(&tid[i], NULL, threadRoutine, NULL);
	}
	

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect

	while(1){
		// Accept a connection, blocking if one is not available until one connects
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");
		if(establishedConnectionFD){
			pthread_mutex_lock(&lock);	//Obtain the lock
			enQueue(availSocks, establishedConnectionFD);	//Add the list of connections to the queue
			pthread_cond_signal(&condition_var);
			pthread_mutex_unlock(&lock);	//Release the lock
		}

	}

	return 0; 
}

/********************************************************************************************************************************************************************
 *void* threadRoutine(void *arg)                                                            								            *
 *thread function will try acquire lock check the quiet of vailable socket descriptors, if one is a vailabel it will receive the key and plaintext, encrypt         *
 *and then send back the ciphertext.                                                                                                                                *
 * *****************************************************************************************************************************************************************/
void* threadRoutine(void *arg){
	int socketFD;	//variable to hold the establishedConnection File Descriptor
	char *message, *plaintext, *key, *ciphertext;	//Variable to hold the received mesage
	int charsRead;	//variable to hold the number fo characters read
	int numOfChunks; //Used to keep track of how big the buffer holding the final message is
	//regex_t regex, otp_dec;	//Variable to hold regex
	//regcomp(&regex, "@@", 0);	//'@@' is used to signa the end of the received message
//	regcomp(&otp_dec, "otp_dec@@", 0); //this will be used to verify that the client sending messages is otp_enc
	while(1){
		pthread_mutex_lock(&lock);	//Acquire the lock
		socketFD = deQueue(availSocks);
		if(socketFD == -5){	//Queue is empty
			pthread_cond_wait(&condition_var, &lock);	//Wait until signaled that a connection is on queue
			socketFD = deQueue(availSocks);		//Try to get a socket
		}
		pthread_mutex_unlock(&lock);	//Release the lock

		switch(isClientValid(socketFD)){
			case 0:	//Invalid Client Connecting or Error reading from the socket, close client connection
				fprintf(stderr, "ERROR: Invalid client or cannot read from socket\n");
				close(socketFD);
				break;
			case 1: //Valid Client
				message = getMessage(socketFD);		//Get the Message from the Client
				char *token = strtok(message, "$$");	//Get the ciphertext from the message using token "$$" to parse
				ciphertext = (char*)malloc(sizeof(char) * strlen(message));
				strcpy(ciphertext, token);

				//plaintext = getPlaintext(message);	//
				//printf("Message for Client: %s\tLength: %d\n", message, strlen(message));
				//key = getKey(message);
				token = strtok(NULL, "$$");		//Use second token call to get the key
				key = (char*)malloc(sizeof(char) * strlen(token));
				strcpy(key, token);
				plaintext = decryptMessage(ciphertext, key);
				sendPlaintext(plaintext, socketFD);
				free(message);
				free(key);
				free(plaintext);
				free(ciphertext);
				break;
			default: //Something went wrong
				fprintf(stderr, "ERROR: Something went wrong, closing client socket\n");
				close(socketFD);
				break;
				
		}
	}

}
/********************************************************************************************************************************************************************
 *int isClientValid(int socketFD)                                                            								            *
 *Function will be used to check if the client connecting is otp_enc.  0 - False; 1 - True                                                                          *
 * *****************************************************************************************************************************************************************/
int isClientValid(int socketFD){
	regex_t regex;	//Regex Variable
	regcomp(&regex, "otp_dec@@", 0);	//regex for the first message that otp_enc will send
	char chunk[CHUNK_SIZE];
	memset(chunk, '\0', CHUNK_SIZE);
	int charsRead, retVal;
	char *vrfyMessage = "otp_dec_d@@";	//Verification message that server will send to otp_enc

	//Receive message 
	charsRead = recv(socketFD, chunk, CHUNK_SIZE - 1, 0);
	if(charsRead < 0){
		fprintf(stderr, "ERROR reading from socket\n");
		retVal = 0;
	}else{
		if(regexec(&regex, chunk, 0, NULL, 0) == 0){
			retVal = 1;
		}else{
			retVal = 0;
		}
	}

	//Send Server verification message
	charsRead = send(socketFD, vrfyMessage, strlen(vrfyMessage), 0); //Send Verification Message
	if(charsRead < 0){
		fprintf(stderr, "ERROR writing to socket\n");
	}

	regfree(&regex);
	return retVal;
}
/********************************************************************************************************************************************************************
 *char* getMessage(int socketFD)                                                            								            *
 *Function will be used to get the Message from the Client.                                                                                                         *
 * *****************************************************************************************************************************************************************/
char* getMessage(socketFD){
	char *chunk, *buffer, *tempBuff;	//variables used to get the message
	int numOfChunks = 1;	//Used to keep track of the current size of the buffer
	int charsRead;
	regex_t regex;		//regex variable

	regcomp(&regex, "@@$", 0);
	
	chunk = (char*)malloc(sizeof(char) * CHUNK_SIZE);
	buffer = (char*)malloc(sizeof(char) * CHUNK_SIZE * numOfChunks);
	memset(chunk, '\0', CHUNK_SIZE);
	memset(buffer, '\0', CHUNK_SIZE * numOfChunks);

	do{
	        memset(chunk, '\0', CHUNK_SIZE);
		charsRead = recv(socketFD, chunk, CHUNK_SIZE -1, 0);
		if(charsRead < 0){
			fprintf(stderr, "ERROR Reading plaintext and key from socket\n");
			free(chunk);
			free(buffer);
			regfree(&regex);
			return NULL;
		}

		//Check to see if the buffer needs to be resized
		if( (strlen(buffer) + strlen(chunk)) >= (numOfChunks * CHUNK_SIZE) ){	//Need to resize the buffer
			tempBuff = (char*)malloc(sizeof(char) * numOfChunks * CHUNK_SIZE);	//Allocate memory for the tempBuff
			memset(tempBuff, '\0', numOfChunks * CHUNK_SIZE);
			strcpy(tempBuff, buffer);	//Copy the contents of the buffer into the tempBuff
			free(buffer);	//Free the buffer

			//Resize the buffer
			numOfChunks++;
			buffer = (char*)malloc(sizeof(char) * numOfChunks * CHUNK_SIZE);
			memset(buffer, '\0', numOfChunks * CHUNK_SIZE);
			strcpy(buffer, tempBuff);	//Copy the original string into the buffer
			free(tempBuff);			//Free the tempBuff
		}
		//Concatenate buffer and chunk
		sprintf(buffer, "%s%s", buffer, chunk);
	}while(regexec(&regex, chunk, 0, NULL, 0) != 0);

	free(chunk);
	regfree(&regex);
	return buffer;
}
/********************************************************************************************************************************************************************
 *char* getPlaintext(char message[])                                                         								            *
 *Function will get the plaintext from the message.                                                                                                                 *
 * *****************************************************************************************************************************************************************/
char* getPlaintext(char message[]){
	char *temp1, *temp2;
	int memSize;
	/****
	if(strlen(message) >= 1024){
		memSize = strlen(message);
	}else{
		memSize = 1024;
	}
	temp1 = (char*)calloc(memSize, sizeof(char));
	printf("memSize: %d\n", memSize);

	strcpy(temp1, message);

	char *token = strtok(temp1, "$$");
	temp2 = (char*)malloc(sizeof(char) * memSize);
	strcpy(temp2, token);
	token = strtok(NULL, "@@");
	free(temp1);
	printf("in getPlaintext, temp1: %s. Freeing temp1\n", temp1);
	printf("Exiting getPlaintext\n");
	return temp2;
	*******/
	char *token = strtok(message, "$$");
	return token;
}
/********************************************************************************************************************************************************************
 *char* getKey(char message[])                                                             								            *
 *Function will get the key from the message.                                                                                                                       *
 * *****************************************************************************************************************************************************************/
char* getKey(char message[]){
//	char temp1[2048], *temp2;

//	strcpy(temp1, message);

	char *token = strtok(message, "$$");

	//temp2 = (char*)malloc(sizeof(char) * strlen(token));
	//strcpy(temp2, token);
	return token;
}
/********************************************************************************************************************************************************************
 *char* encrypteMessage(char plaintext[], char key[])                                        								            *
 *Function will encrypt plaintext with the key                                                                                                                      *
 * *****************************************************************************************************************************************************************/
char* encryptMessage(char plaintext[], char key[]){
	int length = strlen(plaintext) + 10;
	char *cipherText = (char*)malloc(sizeof(char) * length);
	memset(cipherText, '\0', length);

	
	int pt_Char, key_Char, ct_Char;
	int i;
	for(i = 0; i < strlen(plaintext); i++){
		pt_Char = (int)plaintext[i];
		//Check to see if the plaintext char is a space
		if(pt_Char == 32){
			pt_Char = 91;
		}
		key_Char = (int)key[i];
		//Check to see if the key_Char is a space
		if(key_Char == 32){
			key_Char = 91;
		}

		//Adjust pt_Char and ct_Char by subtracting 65, 0 - 25 = A - Z; 26 = (space)
		pt_Char = pt_Char - 65;
		key_Char = key_Char - 65;

		//Get the ct_Char
		ct_Char = pt_Char + key_Char;
		ct_Char = ct_Char % 27;
		//Add 65 to ct_Char to get the Ascii character
		ct_Char = ct_Char + 65;
		if(ct_Char == 91){
			ct_Char = 32;
		}
		cipherText[i] = (char)ct_Char;
	}
	//Add newline to end of ciphertex
	return cipherText;
}
/********************************************************************************************************************************************************************
 *void  sendCipherText(char ciphertext[], int socketFD)                                      								            *
 *Function will send ciphertext to the client                                                                                                                       *
 * *****************************************************************************************************************************************************************/
int sendCiphertext(char ciphertext[], int socketFD){
	char *buffer;
	int length = strlen(ciphertext) + 4;
	int charsWritten, retVal;
	buffer = (char*)malloc(sizeof(char) * length);
	memset(buffer, '\0', length);

	sprintf(buffer, "%s@@", ciphertext);
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);
	if(charsWritten < 0){
		fprintf(stderr, "SERVER: ERROR: Writing Ciphertext to socket\n");
		retVal = 0;
	}else{
		retVal = 1;
	}

	free(buffer);
	return retVal;
}

/********************************************************************************************************************************************************************
 *char* decryptMessage(char ciphtertext[], char key[])                                        								            *
 *Function will decrypt ciphertext with the key                                                                                                                     *
 * *****************************************************************************************************************************************************************/
char* decryptMessage(char ciphertext[], char key[]){
	int length = strlen(ciphertext) + 10;
	char *plaintext = (char*)malloc(sizeof(char) * length);
	memset(plaintext, '\0', length);

	
	int pt_Char, key_Char, ct_Char;
	int i;
	for(i = 0; i < strlen(ciphertext); i++){
		ct_Char = (int)ciphertext[i];
		//Check to see if the plaintext char is a space
		if(ct_Char == 32){
			ct_Char = 91;
		}
		key_Char = (int)key[i];
		//Check to see if the key_Char is a space
		if(key_Char == 32){
			key_Char = 91;
		}

		//Adjust ct_Char and key_Char by subtracting 65, 0 - 25 = A - Z; 26 = (space)
		ct_Char = ct_Char - 65;
		key_Char = key_Char - 65;

		//Get the pt_Char
		pt_Char = ct_Char - key_Char;
		pt_Char = pt_Char % 27;
		//Modulo equation: pt_Char = (pt_Char < 0) ? pt_Char - 27 : pt_Char + 27
		if(pt_Char < 0){
			pt_Char = pt_Char + 27;
		}
		//Add 65 to ct_Char to get the Ascii character
		pt_Char = pt_Char + 65;
		if(pt_Char == 91){
			pt_Char = 32;
		}
		plaintext[i] = (char)pt_Char;
	}
	return plaintext;
}
/********************************************************************************************************************************************************************
 *void  sendPlaintext(char plaintext[], int socketFD)                                      								            *
 *Function will send plaintext to the client                                                                                                                        * 
 * *****************************************************************************************************************************************************************/
int sendPlaintext(char plaintext[], int socketFD){
	char *buffer;
	int length = strlen(plaintext) + 4;
	int charsWritten, retVal;
	buffer = (char*)malloc(sizeof(char) * length);
	memset(buffer, '\0', length);

	sprintf(buffer, "%s@@", plaintext);
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);
	if(charsWritten < 0){
		fprintf(stderr, "SERVER: ERROR: Writing plaintext to socket\n");
		retVal = 0;
	}else{
		retVal = 1;
	}

	free(buffer);
	return retVal;
}
