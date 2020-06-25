
/***************************************************************************************************************************************************
 * Author: Francis C. Dailig                                                                                                                       *
 * Program: otp_enc                                                                                                                                *
 * Description: Program will take a plaintext  file and key file as argument.  It will send to otp_enc_d server for enccryption and output CT      *
 * ************************************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <regex.h>

#define CHUNK_SIZE 512

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

//Prototypes
int checkArgs(char plaintext[], char key[]);	//function to check the arguments
char* getPlaintext(char plaintext[]);		//function to get the plaintext
char* getKey(char key[]);			//function to get the key from the keyFile
int sendReceive(int socketFD, char message[]);	//function to send a message and receive response
int sentIntro(int socketFD);			//function to send message to server about who is connecting
char* getCiphertext(int socketFD);		//Function to receive ciphertext

int main(int argc, char *argv[]){
    int socketFD, portNumber, charsWritten, charsRead;      //Variables to set up socket
    struct sockaddr_in serverAddress;                       //struct for server address
    char *buffer;                                           //Buffer pointer for messages
    char *plaintext, *key;                                  //Variables to hold the key and plaintext from the user
    int buffLength;                                         //Length of buffer
    int status;     //Variable to check return values of functions
    
    //Usage: otp_enc <plaintet> <key> <port>
    if (argc < 4){ 
	    fprintf(stderr,"USAGE: %s <plaintext> <key> <port> \n", argv[0]); exit(1);
    } // Check usage & args

    checkArgs(argv[1], argv[2]);    //Function to check the arguments provided by the user
    plaintext = getPlaintext(argv[1]);  //If we make it here, input is valid, get plaintext key
    key = getKey(argv[2]);

    // Set up the server address struct
    memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");	//Connect to the local host, 127.0.0.1


    // Set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if(socketFD < 0){
	    fprintf(stderr, "Error opening socket\n");
	    exit(2);
    } 	

    // Connect to server
    if(connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {// Connect socket to address
	    fprintf(stderr, "Error connecting to address and port\n");
	    exit(2);
    }

    //Send the intro message server to ensure that otp_enc_d knows that otp_enc is communicating with it
    //status = sendReceive(socketFD, intro);
    status = sendIntro(socketFD);
    if(status != 1){
	    fprintf(stderr, "CLIENT: ERROR in mutual verification of clinet and serv\n");
	    exit(2);
    }

    //Prepare buffer with plaintext and key, then send
    //Set buffer to be 3x the size of the plaintext/key
    buffLength = strlen(key) + strlen(plaintext) + 10;
    buffer = (char*)malloc(sizeof(char) * buffLength);   //Allocated enough memory to hold both the plaintext and key
    memset(buffer, '\0', buffLength);
    sprintf(buffer, "%s$$%s$$@@", plaintext, key);                    //'@@' is a token used to signal the end of a string

    status = sendReceive(socketFD, buffer);
    if(status != 0){
	    fprintf(stderr, "CLIENT: Error: sendReceive() function\n");
	    exit(1);
    }

    close(socketFD);
    free(plaintext);
    free(key);
    free(buffer);
    return 0;
}

/***************************************************************************************************************************************************
 * int checkArgs(char plaintext[], char key[])                                                                                                     *
 * This proram will check to see if plaintext and key files contain valid characters.                                                              *
 * ************************************************************************************************************************************************/
int checkArgs(char plaintext[], char key[]){
    struct stat ptStat, keyStat;    //This will be used to get the stats of the plaintext and keyfiles
    int status;     //Variable to hold return value of functions being used
    long int ptLength, keyLength;   //Variable to hold the length of key and plaintext 
    char *ptBuff, *keyBuff, *tmpBuff;     //Buffer which will be used to hold the plaintext and key strings
    FILE *ptFile, *keyFile;     //File Pointers to plaintext and key files
    regex_t goodString; //Create regex for a good string


    //Check to see if we can open both the plaintext file or the key file
    if( (access(plaintext, R_OK) != 0 || access(key, R_OK)) != 0 ){
	    fprintf(stderr, "Error opening %s or %s\n", plaintext, key);
	    exit(1);
    }

    //Get Length of the plaintext
    status = stat(plaintext, &ptStat);
    if(status == 0){
        ptLength = ptStat.st_size * 2;  //Get size in bytes of the file, should be the lengt
        ptBuff = (char*)malloc(sizeof(char) * ptLength);
        //Open the plaintext file and read in the plaintext into the ptBuff
        ptFile = fopen(plaintext, "r");   //Open the plaintext file
        fgets(ptBuff, ptLength, ptFile);    //Read in the contents of the plaintext file
	fclose(ptFile);
    }else{
        fprintf(stderr, "Error opening %s\n", plaintext);
        exit(1);
    }


    //Get Length of key
    status = stat(key, &keyStat);
    if(status == 0){
        keyLength = keyStat.st_size * 2;
        keyBuff = (char*)malloc(sizeof(char) * keyLength);
        keyFile = fopen(key, "r");
        fgets(keyBuff, keyLength, keyFile);
	fclose(keyFile);
    }else{
        fprintf(stderr, "Error opening %s\n", key);
        exit(1);
    }

    //compare the lengths of strings in each of the files
    if( strlen(ptBuff) > strlen(keyBuff) ){
	fprintf(stderr, "Size of the Plaintext exceeds the size of the key\n");
        exit(1);
    }


    //Check to see if they plaintext or key containt bad characters
    regcomp(&goodString, "^[A-Z ]*\n$", 0); //Regex for string that only containt UPPER CASE and SPACE and ends with newline
    status = regexec(&goodString, ptBuff, 0, NULL, 0);
    if(status != 0){
	    fprintf(stderr, "Invalid Characters were used in the plaintext, exiting\n");
	    exit(1);
    }

    status = regexec(&goodString, keyBuff, 0, NULL, 0);
    if(regexec(&goodString, keyBuff, 0, NULL, 0) != 0){
	    fprintf(stderr, "Invalid Characters were used in the key, exiting\n");
	    exit(1);
    }


    regfree(&goodString);
    free(ptBuff);
    free(keyBuff);
    return 0;
}

/***************************************************************************************************************************************************
 * char* getPlaintext(char plaintext[])                                                                                                            *
 * This will get the plaintext string in the plaintext file                                                                                        *
 * ************************************************************************************************************************************************/
char* getPlaintext(char plaintext[]){
	char *tempBuff;
	FILE* fp;
	struct stat statbuff;
	long int length;

	fp = fopen(plaintext, "r");	//Open the plaintext file
	stat(plaintext, &statbuff);	//Stat to get the size of file
	length = statbuff.st_size * 2;	//Get size of file
	tempBuff = (char*)malloc(sizeof(char) * length); //Allocate memory for tempBuff
	fgets(tempBuff, length, fp);	//Copy the string in the file to tempBuff
	fclose(fp);			//Close the file
	tempBuff[strlen(tempBuff) -1] = '\0';	//Remove the newline
	return tempBuff;		//Return the "cleaned up" string
}
/***************************************************************************************************************************************************
 * char* getKey(char key[])                                                                                                                        *
 * This will get the plaintext key in the key file                                                                                                 *
 * ************************************************************************************************************************************************/
char * getKey(char key[]){
	char *tempBuff;
	FILE* fp;
	struct stat statbuff;
	long int length;

	fp = fopen(key, "r");		//Open file
	stat(key, &statbuff);		//Stat to get size
	length = statbuff.st_size;	//Get size of file
	tempBuff = (char*)malloc(sizeof(char) * length);	//Allocate memory
	fgets(tempBuff, length, fp);	//Copy the key into the tempBuff
	fclose(fp);			//Close the file
	key[strlen(key) - 1] = '\0';	//Remove newline
	return tempBuff;		//Return the key
}
/***************************************************************************************************************************************************
 * int sendRecieve(char message[], char buffer[])                                                                                                  *
 * This function will send and recieve a message                                                                                                   *
 * ************************************************************************************************************************************************/
int sendReceive(int socketFD, char message[]){
    //char *buffer;       //buffer to hold the complete message
    char buffer[70000];
    int numOfChunks = 150;    //indicate number of chunks that the buffer requires
    char chunk[CHUNK_SIZE], *tempBuff;  //buffer to hold messages recieved from the socket
    int charsWritten, charsRead;	//Variable to track number of characters written
    regex_t theEnd;     //Variable for regex
    regcomp(&theEnd, "@@", 0);  //'@@' will be used to signal to the receiver to stop receiving

    //buffer = (char*)malloc(sizeof(char) * CHUNK_SIZE);  //allocate memory for the buffer
    memset(buffer, '\0', 70000);		//initizliat buffer to all NULLs

    //Send message to the server
    charsWritten = send(socketFD, message, strlen(message), 0);
    if(charsWritten < 0){
	    fprintf(stderr, "CLIENT: Error writing to socket\n");
    }

    if(charsWritten < strlen(message)){
	    fprintf(stderr, "CLIENT: WARNING: Not all data writtedn to socket\n");
    }

    //Do-while loop to keep recieveing until the end of message is  received
    do{
	    memset(chunk, '\0', CHUNK_SIZE);                                  //Initialize chunk to NULLs
	    charsRead = recv(socketFD, chunk, CHUNK_SIZE - 1, 0); // Read data from the socket, leaving \0 at end
	    if(charsRead <= 0){
		    fprintf(stderr, "CLIENT: Error reading from socket\n");
		    exit(1);
	    }
	    //Check to see if buffer needs to be resized
	    if(strlen(chunk) + strlen(buffer) >= numOfChunks * CHUNK_SIZE){
		    //Create a tempBuffer to hold the current buffer
		    tempBuff = (char*)malloc(sizeof(char) * strlen(buffer));
		    memset(tempBuff, '\0', strlen(buffer));	//Initialize tempBuff to NULLs
		    strcpy(tempBuff, buffer);			//Copy current string of the buffer into tempBuff

		    //Free and resize the buffer
		    //free(buffer);
		    numOfChunks++;	//Increment number of chunks
		    //buffer = (char*)realloc(buffer,  numOfChunks * CHUNK_SIZE);
		    sprintf(buffer, "%s", tempBuff);	//Copy tempBuff into re-size buffer
		    free(tempBuff);	//Free tempbuff
		    tempBuff = NULL;
	    }

	    sprintf(buffer, "%s%s", buffer, chunk);	//Append the chunk to the buffer
    }while(regexec(&theEnd, chunk, 0, NULL, 0) != 0);
    char *token = strtok(buffer, "@@");
    printf("%s\n", buffer);


    //free(buffer);	//free buffer
    regfree(&theEnd);	//free regex
    return 0;

}
/***************************************************************************************************************************************************
 * int sendIntro(int socketFD)                                                                                                                     *
 * This function will send an intro message to the server                                                                                          *
 * ************************************************************************************************************************************************/
int sendIntro(int socketFD){
	char *intro = "otp_enc@@";	//Server will require this message, other wise it will close the connection
	char *buffer;
	int charsWritten, charsRead;	//Used to check how many bytes were written/read
	int retVal;
	buffer = (char*)malloc(sizeof(char) * CHUNK_SIZE);	//allocate memory for buffer
	memset(buffer, '\0', CHUNK_SIZE);

	regex_t serverInfo;
	regcomp(&serverInfo, "otp_enc_d@@", 0);	//Client expects this message from ther server

	charsWritten = send(socketFD, intro, strlen(intro), 0); 	//Send intro message to the server
	if(charsWritten < 0){	//Error writing to socket
		fprintf(stderr, "CLIENT: ERROR Writing to socket\n");
		retVal = 0;
	}else{			//Success in writing to socket
		charsRead = recv(socketFD, buffer, CHUNK_SIZE - 1, 0);	//Receive message from the server
		if(charsRead < 0){	//Error receiving message from socket
			fprintf(stderr, "CLIENT: ERROR Reading from socket\n");
			retVal = 0;
		}else{	//Success reading meassage from socket
			if(regexec(&serverInfo, buffer, 0, NULL, 0) == 0){	//Server is otp_enc_d
				retVal = 1;
			}else{ 
				retVal = 0;		//Server is not otp_enc_d
			}
		}
	}

	regfree(&serverInfo);
	free(buffer);
	return retVal;
}
