#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <regex.h>
#include <stdbool.h>
#include <pthread.h>

//Define Variables for the thread_t and mutex
pthread_t myThreadID;
pthread_mutex_t lock;

/**************************************************************************************
 * writeTime(void *arg)                                                                 *
 * This will be the function that will be threaded, it will write the current time to *
 * currentTime.txt                                                                    *
 * ***********************************************************************************/
void* writeTime(void *arg){
    //Define variables needed: string buffer, time_t, and File Pointer, struct time pointer
    char buffer[100];
    memset(buffer, '\0', 100); 	//Initialize the buffer to nulls
    time_t t;	//time_t variable
    struct tm *tmp; //Struct for tim
    FILE *fp; 	//File pointer variable
    //Make this thread run in an infinite loop
    //Then immediately try to obtain the lock, which will cause thread to block
    while(1){
        //Try to get lock, if it doesnt, it will wait for main to unlock
        pthread_mutex_lock(&lock);
        fp = fopen((char*)arg, "w+");

	//Get time
        t = time(NULL);
        tmp = localtime(&t);

        if(tmp == NULL){
            perror("localtime error.");
            exit(EXIT_FAILURE);
        }

        //Format 1:03PM, Tuesday, September 13, 2016
        //HH:MM, Day, Month, DD, YYYY
        //%I:%M%p, %A, %B, %d, %Y
        strftime(buffer, 100, "%I:%M%p, %A, %B %d, %Y", tmp);
        fprintf(fp, "%s\n", buffer);
        fclose(fp);

        //Unlock the mutex and sleep, allow main to get control
        pthread_mutex_unlock(&lock);
        sleep(2);
    }
    return NULL;
}
/**************************************************************************************
 * printTime()                                                                        *
 * This funciton will print content of the currentTime.txt file.                      *
 * ***********************************************************************************/
void printTime(){
    //Create File pointer and open currentTime.txt
    FILE *fp;
    fp = fopen("currentTime.txt", "r");

    //Buffer to hold the contents of the line
    char line[100];
    memset(line, '\0', 100);	//Initialize to nulls

    //Print out each line in the currentTime.txt file
    while(fgets(line, 100, fp)){
        printf("%s\n", line);
    }

    //flose file
    fclose(fp);
}

/**************************************************************************************
 * getNewestDir(char newestDir[])                                                     *
 * Function will search through the current directory to find the newest rooms dir.   *
 * ***********************************************************************************/
int getNewestDir(char newestDir[]){
    DIR *d;	//Directory pointer variable
    struct dirent *dir;	//Directory struct
    struct stat tmp, newest; //stat variables used to compare time modified
    struct tm *tm;	//tm struct
    int status, x = 0;  //Variable to hold return value of regexec() function
    regex_t regex;	//Regex varaible

    //Create regex to look for rooms
    status = regcomp(&regex, "dailigf\.rooms\.[[:digit:]+]", 0);

    //Open the current directory, which is "."
    d = opendir(".");
    if(d){
        //Iterate through all files in the directory
        while((dir = readdir(d)) != NULL){

	    //Searching for directory that match the regular expression
            status = regexec(&regex, dir->d_name, 0, NULL, 0);

            if(status == 0){
		// x is a sentintel  value to see if we're looking at the first file
		// if it is the first file, then tmp and newest hold information for the same directory
                if(x == 0){
                    stat(dir->d_name, &tmp);	//Set tmp to the first directory
                    stat(dir->d_name, &newest); //Set newest to the first directory
                    strcpy(newestDir, dir->d_name); //Set the newestDir buffer to the first directory looked at
                    x++; //increment to ensure we don't do this if statement again
                }
                stat(dir->d_name, &tmp);	//set tmp the current directory being looked at
                //if difftime > 0, tmp is the newest, then set newest to tmp.
                if(difftime(tmp.st_mtime, newest.st_mtime) > 0){ 
                    newest = tmp;	//Set newest to tmp, since tmp is the newest directory
                    strcpy(newestDir, dir->d_name); //Set the newestDir buffer to newest directory
                }
            }

        }
    }else{
        fprintf(stderr, "Could not open current directory\n");
        perror("Error in getNewestDir().");
	regfree(&regex);
	closedir(d);
        exit(1);
    }

    regfree(&regex);
    closedir(d);
    return 0;
}

/**************************************************************************************
 * getStartFile(char dirName[], char fileName[])                                      *
 * Function will get starting location.                                               *
 * ***********************************************************************************/
int getStartFile(char dirName[], char fileName[]){
    regex_t regex, rmType, type; 	//Varaibles to hold regex for this function
    int status;				//Variable to keep track of status of regexex()
    DIR *dp;	//Directory Pointer
    FILE *fp;	//File Pointer
    char buffer[100];	//Character buffer
    struct dirent *dir;	//dirent struct *
    status = regcomp(&regex, "^[.]", 0);	//regex to see if file examine is  not"." or ".."
    status = regcomp(&rmType, "ROOM TYPE", 0);  //regex to serach for "ROOM TYPE" string in the file
    status = regcomp(&type, "START_ROOM", 0);   //regex to search for the START ROOMM

    //open the dirName directory
    dp = opendir(dirName);

    //While to iterate through each file in directory
    while((dir = readdir(dp)) != NULL ){
        //Select a room that is '.' or '..'
        if( regexec(&regex, dir->d_name, 0, NULL, 0) ){
            sprintf(fileName, "%s/%s", dirName, dir->d_name); //Set filename 

            fp = fopen(fileName, "r");	//open the filename
            if(fp == NULL){
                printf("Error opening file: %s\n", fileName);
                perror("Error in getStart File().");
		closedir(dp);
		regfree(&regex);
		regfree(&rmType);
		regfree(&type);
                return 1;
            }

            //Check to see if the file selected is the Start Room
            //Iterate through the contents of the file, check to see if it is the Start Room.
            while(fgets(buffer, 100, fp) != NULL){
                status = regexec(&rmType, buffer, 0, NULL, 0); //checking to see if the line has the "ROOM TYPE" string

                //Compare the Room Type at the end of the file
                if(status == 0){
                    if(regexec(&type, buffer, 0, NULL, 0) == 0){ //Checking to see if ROOM TYPE is the START_ROOM
			fclose(fp);
			closedir(dp);
			regfree(&regex);
			regfree(&rmType);
			regfree(&type);
                        return 0;
                    }
                }

            }
        }
    }
    regfree(&regex);
    regfree(&rmType);
    regfree(&type);
    closedir(dp);
    fclose(fp);
    return 1;
}
/**************************************************************************************
 * isThisTheEnd(char fileName[])                                                      *
 * Function Will check to see if we are in the end room.                              *
 * ***********************************************************************************/
bool isThisTheEnd(char fileName[]){
    FILE *fp;		//File pointer
    char buffer[100];	//Char buffer array
    memset(buffer, '\0', 100); //Initialize to NULLS
    regex_t rmType, endRoom;  //Variables for regex needed for thsi functions
    int status;		//Variable to check return of regexec()

    regcomp(&rmType, "ROOM TYPE", 0);  //Regex to search for "ROOM TYPE" String
    regcomp(&endRoom, "END_ROOM", 0);  //Regex to search for "END_ROOM" string

    fp = fopen(fileName, "r");	//open the filename pased to function

    //Iterate through each line in file
    while(fgets(buffer, 100, fp) != NULL){
        //If we're at the line in the file we want to examine, i.e ROOM TYPE:
        if(regexec(&rmType, buffer, 0, NULL, 0) == 0){
	    //Creating a token variable with the ":" delimeter
	    //First time this execute it will be "ROOM TYPE"
            char *token = strtok(buffer, ":");

	    //Second time this execute token will hold the actual room type
            token = strtok(NULL, " :");

	    //Checking to see if the buffer has the "END_ROOM" string
            if(regexec(&endRoom, buffer, 0, NULL, 0) == 0){
		fclose(fp);
		regfree(&rmType);
		regfree(&endRoom);
                return true;
            }else{
		fclose(fp);
		regfree(&rmType);
		regfree(&endRoom);
                return false;
            }
        }
    }
    fclose(fp);
    regfree(&rmType);
    regfree(&endRoom);
    return false;
}
/**************************************************************************************
 * getLocation(char locationBuffer[])                                                 *
 * function will return the name of the room.                                         *
 * ***********************************************************************************/
int getLocation(char fileName[], char locationBuffer[]){
    FILE *fp;	//File Pointer
    char buffer[50];	//Char buffer
    memset(buffer, '\0', 50);	//Initialize buffer to NULLS

    fp = fopen(fileName, "r");	//Open the filename
    fgets(buffer, 50, fp);	//Only need to get the first line

    //strtok to get the actualy location name in the file
    //First time strtok executes it will be the "ROOM NAME" string
    char *token = strtok(buffer, ":");
    //Second time strtok executes it will be the actual room name
    token = strtok(NULL, " ");

    //copy the location name to the location Buffer argument
    strcpy(locationBuffer, token);
    fclose(fp);
    return 0;
}
/**************************************************************************************
 * getConnection(char connectionBuff[], char fileLocation[])                          *
 * Function will return the connections in the room                                   *
 * ***********************************************************************************/
int getConnection(char connectionBuff[], char fileLocation[]){
    //Create regex for the Word "CONECTION", will be used when searchign the file
    regex_t connection;
    regcomp(&connection, "CONNECTION", 0);

    //variable for File Pointer and a character buffer for fgets
    FILE *fp;
    char buffer[100];
    memset(buffer,'\0', 100);

    //Iterate through each line of the file with fgets
    //When we find a line with "CONNECTIONS", add that line to the connectionBuff[]
    fp = fopen(fileLocation, "r");
    while(fgets(buffer, 100, fp) != NULL){
        if(regexec(&connection, buffer, 0, NULL, 0) == 0){
            //Create a strtoken to only get the Room name
            char *token = strtok(buffer, ":");
            token = strtok(NULL, " ");
            //Remove the \n from the token
            token[strlen(token) - 1] = '\0';
            strcat(connectionBuff, token); //Adding the name of the room connectionBuff array
            strcat(connectionBuff, ", ");  //Add " ' " after the name
        }
    }
    //Remove the last ',' from the buffer
    connectionBuff[strlen(connectionBuff) - 2] = '\0';
    regfree(&connection);
    fclose(fp);
    return 0;
    
}
/**************************************************************************************
 * getCurrentType(char roomType[], char fileLocation[])                               *
 * Function will return the current room type.                                        *
 * ***********************************************************************************/
int getCurrentType(char roomType[], char fileLocation[]){
    //create a regex that will be used to search for "ROOM TYPE"
    regex_t rmType;
    regcomp(&rmType, "ROOM TYPE", 0);

    //variable for File Pointer and character buffer for fgets
    FILE *fp;
    char buffer[64];
    memset(buffer, '\0', 64);

    //Iterate through each line of the file with fgets
    //When we find a line with "ROOM TYPE", return the TYPE
    fp = fopen(fileLocation, "r");
    while(fgets(buffer, 64, fp) != NULL){
        if(regexec(&rmType, buffer, 0, NULL, 0) == 0){
            //Create a strtok to only get the Room Type
            char *token = strtok(buffer, ":");
            token = strtok(NULL, " ");
            strcpy(roomType, token);
        }
    }
    fclose(fp);
    regfree(&rmType);
    return 0;
}
/**************************************************************************************
 * printFileContents(char dirName[], char fileName[])                                 *
 * Diagnostic Function to print out the contents of each file in the directory.       *
 * ***********************************************************************************/
int printFileContents(char dirName[], char fileName[]){
    char buffer[100];           //For each line Read
    char dirAndFile[32];
    memset(buffer,'\0', 100);
    memset(dirAndFile, '\0', 32);

    FILE *fp;	//File pointer
    DIR *dp;	//Directory pointer
    int retD, retP;	//variable to hold return values

    //sprintf(dirAndFile, "%s/%s", dirName, fileName);
    //printf("Opening File: %s\n", dirAndFile);

    fp = fopen(fileName, "r");
    if(fp == NULL){
        perror("Error opening file.");
        return 1;
    }

    //Get each line in file
    while( fgets(buffer, 100, fp) != NULL ){
        printf("%s", buffer);
        memset(buffer, '\0', 100);
    }

    fclose(fp);
    return 0;
}

int main(void){
    //Arrays to hold information for the director, startFile, current File being examined, current location
    //Connections in a room, current room type, and user input.
    //Initialize the array to '\0'
    char directory[32];
    char startFile[32];
    char currentFile[32];
    char tempFile[32];
    char location[32];
    char connectionBuffer[100];
    char currRmType[16];
//    char userBuffer[64];
    char *userBuffer = calloc(64, sizeof(char));
    memset(directory, '\0', 32);
    memset(startFile, '\0', 32);
    memset(currentFile, '\0', 32);
    memset(tempFile, '\0', 32);
    memset(location, '\0', 32);
    memset(connectionBuffer, '\0', 100);
    memset(currRmType, '\0', 16);
    memset(userBuffer, '\0', 64);

    size_t bufferSize = 64;  //Need this for getline() argument
    int status;
    int numberOfSteps = 0;   //Used to count number of steps user took

    //REGEX for END_ROOM and for TIME
    regex_t endRoom, tmRegex;
    regcomp(&endRoom, "END_ROOM", 0);
    regcomp(&tmRegex, "[T|t]ime", 0);

    //Dynamic char array for to hold the current path travelled
    int pathSz = 1024;	//Initially the char buffer holding the string names of rooms travelled
    int currPathSz = 0;
    char *path = calloc(pathSz, sizeof(char));
    char *tmp;  //needed if we have to resize path

    //String for currentTime.txt
    char timeFile[] = "currentTime.txt";


    //Before beginning the game, lock the mutex, create the thread for writeTime()
    if(pthread_mutex_init(&lock, NULL) != 0){
        printf("mutex init failed\n");
        return 1;
    }
    pthread_mutex_lock(&lock);
    pthread_create(&myThreadID, NULL, writeTime, (void*) timeFile);

    //Before entering the do-while loop for the game, find the newest directory and set the start
    //Set the Start location, the available connection, and the Room type.
    //Set the path and increment the path count.
    getNewestDir(directory);
    getStartFile(directory, startFile);
    getLocation(startFile, location);
    getConnection(connectionBuffer, startFile);
    getCurrentType(currRmType, startFile);

    //Need to Iterate through the do-while loop until the END_ROOM is Reached
    do{
        printf("CURRENT LOCATION: %s", location);
        printf("POSSIBLE CONNECTIONS: %s.\n", connectionBuffer);

        //Ask user to enter a location to go to
        //Continue asking until they enter a valid locatoin
        do{
           // printf("CURRENT LOCATION: %s\n", location);
            //printf("POSSIBLE CONNECTIONS: %s\n", connectionBuffer);
            printf("WHERE TO? >");
            getline(&userBuffer, &bufferSize, stdin);
            printf("\n");

	    //Only do this if the user entered a string of length > 1
	    if(strlen(userBuffer) > 1){
	    	userBuffer[strlen(userBuffer) -1] = '\0';
	    }

            regex_t userInput;  	//regex for user input
            status = regcomp(&userInput, userBuffer, 0);	//compose regex for user input
            status = regexec(&userInput, connectionBuffer, 0, NULL, 0);	//Check to see if the user input matches connection of the current roomm
            if(regexec(&tmRegex, userBuffer, 0, NULL, 0) == 0){      //Test to see if user entered "time"
                //If user entered time, unlock the mutex and sleep, allowing thread to execute
                pthread_mutex_unlock(&lock);
		sleep(1);
                //Lock the mutex
                pthread_mutex_lock(&lock);
                //Print contents of the file
                printTime();
		regfree(&userInput);
            }else if(status != 0){
                printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
        	printf("CURRENT LOCATION: %s", location);
        	printf("POSSIBLE CONNECTIONS: %s.\n", connectionBuffer);
		regfree(&userInput);
            }else{
		regfree(&userInput);
		continue; 
	    }
	    regfree(&userInput);
        }while(status != 0);	//Continue looping until user enters a valid room

        //Check to see whether the path buffer needs to be resized
	//If it does double the size, and free the previous buffer
        if((currPathSz + strlen(userBuffer)) >= pathSz){
            pathSz = 2 * pathSz;
            tmp = calloc( pathSz, sizeof(char) );
            strcpy(tmp, path);
            free(path);
            path = tmp;
            tmp = NULL;
        }
        //Restore the \n in the userBuffer
        userBuffer[strlen(userBuffer)] = '\n';
        //Add to the path
        sprintf(path, "%s%s", path, userBuffer); 	//concatenate the current path string with room entered by the user
        currPathSz = currPathSz + strlen(userBuffer);	//Account for the size of the new path string
        numberOfSteps++;

        //If we get out of the do-while loop they entered a valid location
        //Set the currentFile to path to the folder
        userBuffer[strlen(userBuffer) - 1] = '\0';	//Remove the \n from userBuffer
        sprintf(currentFile, "%s/%s", directory, userBuffer);

        //Clear the used before before using them in the functions
        memset(location, '\0', 32);
        memset(connectionBuffer, '\0', 100);
        memset(currRmType, '\0', 16);

        //Set the current location, the connections available, and the room type
        getLocation(currentFile, location);
        getConnection(connectionBuffer, currentFile);
        getCurrentType(currRmType, currentFile);
    }while(regexec(&endRoom, currRmType, 0, NULL, 0) != 0);	//Keep looping until we find end of the room
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", numberOfSteps);
    printf("%s", path);
    free(path);		//Deallocate memory on heap
    free(userBuffer);	//Deallocate memory on heap
    regfree(&endRoom);
    regfree(&tmRegex);
    pthread_mutex_destroy(&lock);	//Destroy mutex
    return 0;
}
