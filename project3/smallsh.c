/*****************************************************************************************
 * Name: smallsh.c                                                                       *
 * Author: Francis C. Dailig                                                             *
 * CS 344 Operating Systems                                                              *
 * **************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <regex.h>
#include <signal.h>
#include <stdbool.h>


//Define maximum character and argument length
#define MAX_CHAR_LENGTH 2048
#define MAX_NUM_ARGS 512

//Define global variabls
int childExitStatus = -5, fgExitStatus = -5; //Hold child exit status for bg and fg procs
pid_t childPID, fgPID;  
pid_t pidArray[512];	//hold pids of child processes in background
int bgLocked = 0;	//0: normal, 1: bg locked
int numBgProcs = 0;     //Keep track of number of backgrounded processes
int bgTerminated = 0;  //Keep track to see if a background process terminated, 0::NO, >1::YES
int fg = 0;        //Keep track to see if there is fg process running
int fgTermSignal = -5; //Keeps track to see if fg proc was terminated by a signal
int status = 0;     //hodl status of fg proc if exited normally
pid_t  test;


//Define Signal Handlers
void catchSIGCHLD(int signo){
	if(numBgProcs > 0){
		test = waitpid(childPID, &childExitStatus, WNOHANG);
		if(test == childPID){
       			bgTerminated++;
			numBgProcs--;	//decrement number of background processes running
		}
	}
}

void catchSIGINT(int signo){
    if(fg > 0){ //SIGINT Was recieved before foreground process could complete
        fgPID = waitpid(-1, &fgExitStatus, 0);
        if(WIFEXITED(fgExitStatus) != 0){ //fg proc exited normally
            status = WEXITSTATUS(fgExitStatus); //Set status if exited normally
            fgTermSignal = -5;  //reset fgTermSignal 
        }
        if(WIFSIGNALED(fgExitStatus) != 0){ //fg process was killed by a signal
            if(WTERMSIG(fgExitStatus) == 2){
                char *message = "terminated by signal 2\n";
                write(STDOUT_FILENO, message, 23);
                fg = 0; //Reset fg
                fgTermSignal = 2; //Set fgTermSignal
                status = -5; //Reset status
            }
        }
    }
}

void catchSIGTSTP(int signo){
    //Check to see if there is a foreground process
    if(fg > 0){
        fgPID = waitpid(-1, &fgExitStatus, 0); //Wait for the foreground process to complete
        if(WIFEXITED(fgExitStatus) != 0){ //fg proc exited normally
            status = WEXITSTATUS(fgExitStatus);
            fgTermSignal = -5;
        }
        if(WIFSIGNALED(fgExitStatus) != 0){ //fg proc terminated by signal
            fgTermSignal = WTERMSIG(fgExitStatus);
            status = -5;
        }
    }
    if(bgLocked == 0){  //If bg not locked, then lock it
        bgLocked = 1;
        char *message = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 49);
        fflush(stdout);
    }else{ //bg is locked, unlock it
        bgLocked = 0;
        char *message1 = "Exiting foreground-only mode\n";
        write(STDOUT_FILENO, message1, 29);
        fflush(stdout);
    }
}

//Prototypes for functions
int parseInputBuffer(char userInput[], char inRedirect[], char outRedirect[], char command[],  char *argArray[], int *bg, char *path);
int setCommand(char token[], char command[], char *argArray[], char *path);
int spawnProcess(char command[], char *argArray[],  char inRedirect[], char outRedirect[], int *bg, int *status);
int checkBg(int *exitStatus, int *child, int *terminated);
int expandDollars(char input[]);
void checkFgProc();
void printStatus();


/**************************************************************************************************************************************
 * main()         														      *
 * This is the main function
 * ***********************************************************************************************************************************/


int main(void){
    //declare and initialtion sigaction structs that will be used for the redefined signal handlers
	struct sigaction SIGCHLD_action = {0}, SIGINT_action = {0}, SIGTSTP_action = {0};

	//Set up SIGCHLD Signal Handerl
	SIGCHLD_action.sa_handler = catchSIGCHLD;
	sigfillset(&SIGCHLD_action.sa_mask);
	SIGCHLD_action.sa_flags = 0;
	sigaction(SIGCHLD, &SIGCHLD_action, NULL);

    //Set up SIGINT Signal Handler
    SIGINT_action.sa_handler = catchSIGINT;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    //Set up SIGSTP Signal Handler
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);


	char inputBuffer[MAX_CHAR_LENGTH];	//inputBuffer
	char command[MAX_CHAR_LENGTH];		//This wil hold globbal path for a command
	char *argArray[MAX_NUM_ARGS];		//argument Array
	char inRedirect[MAX_CHAR_LENGTH], outRedirect[MAX_CHAR_LENGTH];		//These will be used to hold the input output file names if proviced
	int bg = 0;			//flag to see if user wants to background a process, 0: foreground, 1: background
	int *pidArr[512];			//pidArray to hold PIDS that have been backgrounded
    char *path = getenv("PATH");    //Get Path environment variable
    int selection; //Hold selection of users

	regex_t regex_exit, regex_cd, regex_status, regex_blank, regex_comment, regexdollars;	//regex variable
	regcomp(&regex_exit, "^exit", 0);	//create regex for "exit"
	regcomp(&regex_cd, "^cd", 0);		//create regex for "cd"
	regcomp(&regex_status, "^status[[:space:]?&?]", 0);	//create regex for "status"
	regcomp(&regex_blank, "[a-zA-Z0-9_]",0);    //regex will be used to test for a blank space
	regcomp(&regex_comment, "#", 0);    //regex will be used to test for a comment
	regcomp(&regexdollars, "\\$\\$", 0); //Check for expansion

	//Main will continue prompting for input 
	do{
		//Initialize input buffer each time before asking for input
		memset(inputBuffer, '\0', MAX_CHAR_LENGTH);
		memset(inRedirect, '\0', MAX_CHAR_LENGTH);
		memset(outRedirect, '\0', MAX_CHAR_LENGTH);
		memset(command, '\0', MAX_CHAR_LENGTH);

        //Check status of Background during every iteration of the do-while loop
        if(bgTerminated > 0 && childPID != 0){
            checkBg(&childExitStatus, &childPID, &bgTerminated);
        }


		printf(": ");
		fflush(stdout);
		fgets(inputBuffer, MAX_CHAR_LENGTH, stdin);
		//Check for expansion
		if(regexec(&regexdollars, inputBuffer, 0, NULL, 0) == 0){
			expandDollars(inputBuffer);
		}
		//check to see what kind of command the user entered
		//different status values will be used for a switch statement
		if(regexec(&regex_cd, inputBuffer, 0, NULL, 0) == 0){ //Check to see if user entered 'cd'
			selection = 0;
		}else if(regexec(&regex_status, inputBuffer, 0, NULL, 0) == 0){ //Check to see if users entered 'status'
			selection = 1;
		}else if(regexec(&regex_exit, inputBuffer, 0, NULL, 0) == 0){ //Check to see if user entered 'exit'
			selection = 3;
		}else if(regexec(&regex_blank, inputBuffer, 0, NULL, 0) != 0 || regexec(&regex_comment, inputBuffer, 0, NULL, 0) == 0){ //See if user entered a nothing or '#'
			selection = 4;
		}else{ selection = 2; }

		switch(selection){
			case 0: ;//User want to change directory 
				//check to see if they passed in a directory to change to
				char *token;
				token = strtok(inputBuffer, " \n");
				token = strtok(NULL, " \n");
				if(token == NULL){
					//Get the Home Environment Variable
					char *homedir = getenv("HOME");
					fflush(stdout);
					chdir(homedir);
				}else{
					token[strlen(token)] = '\0'; 	//Need to remove newline character at end of path
					if(chdir(token) != 0){
						perror("chdir() failed");
						fflush(stdout);
					}

				}
				break;

			case 1: //User want to check status of most recent foreground process
               			printStatus();
				break;
			case 2: //User wante to enter a non built-in command
				//int parseInputBuffer(char userInput[], char *inRedirect, char *outRedirect, char command[],  char *argArray[], int *bg);
				parseInputBuffer(inputBuffer, inRedirect, outRedirect, command, argArray, &bg, path);
				spawnProcess(command, argArray, inRedirect, outRedirect, &bg, &status);
				break;
			case 3: //User wantes to exit, wait for all BG processes to complete
                		while(numBgProcs > 0){ //Wait until all BG processes are complete
                    			waitpid(-1, &childExitStatus, 0);
					numBgProcs--;
                		}
				break;
			case 4:	//User entered a '#' or blank, it will be ignored and loop back up.
				break;
			default: 
				break;

		}
			
	}while(regexec(&regex_exit, inputBuffer, 0, NULL, 0) != 0); //loop will continue until user 'exit's


	//Free all the regex's
	regfree(&regex_status);
	regfree(&regex_cd);
	regfree(&regex_exit);
	regfree(&regex_comment);
	regfree(&regex_blank);
	return 0;
}


/********************************************************************************************************************
 *parseInputBuffer(char userInput[], char *inRedirect, char *outRedirect, char *usrArguments, int *bg)              *
 *This function will be used to parse through users arguments
********************************************************************************************************************/
int parseInputBuffer(char userInput[], char inRedirect[], char outRedirect[], char command[],  char *argArray[], int *bg, char path[]){
    //Create a string for "/dev/null"
    char *devNull = "/dev/null";
	//Need these regex's to see if the user provided input/output redirection
	regex_t regex1, regex2, regex3, regex4;
	regcomp(&regex1, "<", 0);
	regcomp(&regex2, ">", 0);
	regcomp(&regex3, "&", 0);
	regcomp(&regex4, "\\$\\$", 0);

    int nmatch = 1; //for regexec, set number of matches
    regmatch_t pmatch[nmatch]; //for regex, this will hold the offset of the matching substring

    //Check to see if the userInput needs to be expanded
    if(regexec(&regex4, userInput, 0, NULL, 0) == 0){
        expandDollars(userInput);
    }

	//User input should, at most have three sections: the command, file input, and file output
	char *tempBuffer[6];	//Create a array of pointer to hold at least 3 sections of code, just to be safe
	int buffIndex = 0;	//Used to iterate through the array
	int i;
	for(i = 0; i < buffIndex; i++){	//Initialze to Null Pointers
		tempBuffer[i] = NULL;
	}

    //reset bg
    *bg = 0;
	//Check to see if background flag was provided
	if(regexec(&regex3, userInput, nmatch, pmatch, 0) == 0){
        	*bg = 1; //set bg flag
        	userInput[pmatch[0].rm_so] = '\0'; //place null value where & was found
	}

	//Use strtok to iterate through the user input, this strtok iteration will look for section of userinput
	//The Delimteers will be '<', '>', '\n'
	char *temp = malloc(sizeof(char) * strlen(userInput)); 	//Use this for tokenization, preserver original input
	sprintf(temp, "%s", userInput);

	char *token = strtok(temp, "\n<>");	//First token will be section that hold the command that user wants to run
	tempBuffer[0] = malloc(sizeof(char) * strlen(token));  //position 0 will hold the command
	strcpy(tempBuffer[0], token);
	buffIndex++;

	//This regex will test to see if there is an input '<' redirect
	if(regexec(&regex1, userInput, 0, NULL, 0) == 0){
		strcpy(temp, userInput);
		token = strtok(temp, "<");	//token is at string before the input file
		token = strtok(NULL, " >\n");	//token should be the input file
        if(token == NULL){ //If NULL, set input file to /dev/null
            tempBuffer[1] = (char*)malloc(sizeof(char) * strlen(devNull));
            strcpy(tempBuffer[1], devNull);
        }else{
		    tempBuffer[1] = malloc(sizeof(char) * strlen(token));	//Position 0 will hold the input file
		    strcpy(tempBuffer[1], token);
        }
	strcpy(inRedirect, tempBuffer[1]);	//Copy the file name to inRedirect
	buffIndex++;
	}

	//This regex will test see if there is an output redirec
	if(regexec(&regex2, userInput, 0, NULL, 0) == 0){
		strcpy(temp, userInput);
		token = strtok(temp, ">");	//token is at string before '>'
		token = strtok(NULL, " <\n");	//token should be the output file
        if(token == NULL){
            tempBuffer[2] = (char*)malloc(sizeof(char) * strlen(devNull));
            strcpy(tempBuffer[2], devNull);
        }else{
		    tempBuffer[2] = malloc(sizeof(char) * strlen(token));
		    strcpy(tempBuffer[2], token);
        }
		strcpy(outRedirect, tempBuffer[2]);
		buffIndex++;
	}

	setCommand(tempBuffer[0], command, argArray, path);		//Call function to setup the command and arguments
	//for(i = 1; i < buffIndex; i++){
	//	if(tempBuffer[i] != NULL){
	//		tempBuffer[i] = NULL;
	//	}
	//}

	regfree(&regex1);
	regfree(&regex2);
    regfree(&regex3);
    regfree(&regex4);
	return 0;

}

/********************************************************************************************************************
*int setCommand(char *token, char command[], char *argArray[])                                                       *
*This function will set up the global path of the command and argument array.                                       *
********************************************************************************************************************/
int setCommand(char token[], char command[], char *argArray[], char *path){
	//tempBuffer to hold the string in the token, preserver token
	char *userInput = malloc(sizeof(char) * strlen(token));
	strcpy(userInput, token);

	char *tempBuffer = malloc(sizeof(char) * strlen(userInput));
	strcpy(tempBuffer, userInput);

	//Get the PATH environment variable
	char *path2 = (char*)malloc(sizeof(char) * strlen(path));
    strcpy(path2, path);

	//create another token to iterate through the userInput
	char *token1 = strtok(tempBuffer, " \n");

	//Set the first argument of the argArray[] to the command
	int arrIndex = 0;	//index for the argArray[]
	argArray[arrIndex] = malloc(sizeof(char) * strlen(token1));
	strcpy(argArray[arrIndex], token1);
	arrIndex++;

	//Check to see if the command is a recognize *nix command
	//need tokenzie the path
	char *token2 = strtok(path2, ":");
	int ret = 0;  //return value for access()
	while(ret != 1 && token2 != NULL){
		sprintf(command, "%s/%s", token2, token1);

		if(access(command, X_OK) == 0 ){
			ret = 1;
		}
		token2 = strtok(NULL, ":");
	}
	

	//Create regex to find newline
	regex_t newline;
	size_t nmatch = 1;
	regmatch_t pmatch[nmatch];
	regcomp(&newline, "\n", 0);

	//Iterate through token1 to get the arguments
	char *tempBuffer2 = malloc(sizeof(char) * strlen(userInput));
	strcpy(tempBuffer2, userInput);
	token1 = strtok(tempBuffer2, " \n");			//Setup token 1 again
	token1 = strtok(NULL, " \n");
	while(token1 != NULL){
		argArray[arrIndex] = malloc(sizeof(char) * strlen(token1));
		strcpy(argArray[arrIndex], token1);
		arrIndex++;
		token1 = strtok(NULL, " \n");
	}

	//Make final argument in argArray NULL
	argArray[arrIndex] = NULL;
	arrIndex++;

//	regfree(&newline);
	if(tempBuffer != NULL){
		free(tempBuffer);
	}
	if(tempBuffer2 != NULL){
		free(tempBuffer2);
	}
	return 0;
}
/********************************************************************************************************************
* int spawnProcess(char command[], char inRedirect[], char outRedirect[], int *bg, int *status)                     *
* This function will spawn a new process                                                                            *
********************************************************************************************************************/
int spawnProcess(char command[], char *argArray[], char inRedirect[], char outRedirect[], int *bg, int *status){
	pid_t spawnPID;	//PID for new process
	int exitStatus; //to hold child exit status, for parent to keep track if process is in foreground
    int execStatus; //Varible to hold return value of exec
    int sourceFD, targetFD; //Variables to hold file descriptor of input/output files

	spawnPID = fork();

	//Switch statement for spawnPID
	switch(spawnPID){
		case -1:  //Error
			perror("Unable to fork a new child, returning from this command");
			fflush(stdout);
			return 1;
			break;
		case 0:  //Child was successfully created
                	signal(SIGTSTP, SIG_IGN);    //All child processes will ignore SIGSTP
                	if(*bg == 1 && bgLocked == 0){ //If the child is set to run in the background, set to ignore SIGINT signals
                    		signal(SIGINT, SIG_IGN);
               		 }
			if(strlen(inRedirect) == 0 && strlen(outRedirect) == 0){ //No input or output redirect given, just run command
				execvp(command, argArray);
			}else if(strlen(inRedirect) > 0 && strlen(outRedirect) ==0){ //input redirect given, but not output redirect
                    		//Open the input file for read
                    		sourceFD = open(inRedirect, O_RDONLY);
                    		if(sourceFD == -1){
                        	perror("Could not open input file");
                        	fflush(stdout);
                        	exit(1);
                    		}
                   		 //Set up dup2();
                    		dup2(sourceFD, 0);
                    		execvp(command, argArray);
			}else if(strlen(inRedirect) == 0 && strlen(outRedirect) > 0){ //output redirect given, but no input redirect
                    		targetFD = open(outRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    		if(targetFD == -1){
                        	perror("Error opening output file");
                        	fflush(stdout);
                        	exit(1);
                    	}

                    		//Set up dup2();
                    		dup2(targetFD, 1);
                    		execvp(command, argArray);
			}else{  //Both inpt and output redirect given
                    		//Open the input file for
                    		sourceFD = open(inRedirect, O_RDONLY);
                    		if(sourceFD == -1){
                        		perror("Could not open input file");
                        		fflush(stdout);
                        		exit(1);
                    		}
                    		//Open the output file
                    		targetFD = open(outRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    		if(targetFD == -1){
                        		perror("Error opening output file");
                        		fflush(stdout);
                        		exit(1);
                    		}
                    		//Setup redirects before execvp()
                    		dup2(sourceFD, 0);
                    		dup2(targetFD, 1);
                    		execvp(command, argArray);
			}
                	perror("Error Executing the Command");
			exit(1);
			break;
		default: //Parent process
			switch(bgLocked){
				case 0: //Normal Behavior for background
					if(*bg == 0){ //child is in foreground
                        			fg = spawnPID;  //Set this, if signal terminates fg, parent will check to see if > 0
						fgPID = waitpid(spawnPID, &fgExitStatus, 0);
                        			checkFgProc();
                        			fg = 0;		//Let parent know that fg process completed without a signal
						break;
					}else{	//child is in background
						//Parent continues running
						printf("background pid is %d\n", spawnPID);
						childPID = spawnPID;
						fflush(stdout);
						numBgProcs++; //Increment the number of processes running in background						
						break;
					}
				case 1: //All processes running in foreground
					printf("in locked bg mode\n");
                    			fg = spawnPID;
					fgPID = waitpid(spawnPID, &fgExitStatus, 0);
                   			checkFgProc();   //Function to check the exit status of the foreground process
                   			fg = 0;
					break;
				default: break;
			}
	}

	return 0;
}

/********************************************************************************************************************
*int expandDollars(char input[])                                                                                    *
* This will expand dollar signs the PID in userinput                                                                *
********************************************************************************************************************/
int expandDollars(char input[]){
    pid_t pid = getpid();
    char *tempBuff = (char*)malloc(sizeof(char) * 2048);  //Need a temp buffer big enough to hold the expanded string
    char *tempArr = (char*)malloc(sizeof(char) * strlen(input)); //tempBuffer to hold a copy for the original input, needed for tokenization
    memset(tempBuff,'\0', 2048);
    memset(tempArr,'\0', strlen(input));

    strcpy(tempArr, input);

    //Regex to check for newlines
    regex_t regex;
    int nmatch = 1;
    regmatch_t pmatch[nmatch];
    regcomp(&regex, "\n", 0);


    char *token = strtok(tempArr, "$$");
    while(token != NULL){
        sprintf(tempBuff, "%s%s%d", tempBuff, token, pid);
        if(regexec(&regex, tempBuff, nmatch, &pmatch, 0) == 0){
            tempBuff[pmatch[0].rm_so] = '\0'; //set '\n' to '\0'
        }
        token = strtok(NULL, "$$");
    }

    memset(input, '\0', strlen(input));
    strcpy(input, tempBuff);

    free(tempBuff);
    free(tempArr);
    return 0;

}
/********************************************************************************************************************
* void checkBackgroundProc()                                                                                        *
* This will print the status of the most recent background process that died.                                       *
********************************************************************************************************************/
int checkBg(int *exitStatus, int *child, int *terminated){
    if(bgTerminated > 0){ //terminated > 0 means, that a child processs stopped running
   	 //Check Exit status of bg proces
    	if(WIFEXITED(childExitStatus) != 0){ //child process exited normally
        	printf("background pid %d is done: exit value %d\n", *child,  WEXITSTATUS(childExitStatus));
		fflush(stdout);
		bgTerminated--;
		return 0;
    	}

    	if(WIFSIGNALED(childExitStatus) != 0){ //bg process was killed by signal
		printf("background pid %d is done: terminated by signal %d\n", *child, WTERMSIG(childExitStatus));
		fflush(stdout);
		bgTerminated--;
		return 0;
    	}
    }
    return 0;
}
/********************************************************************************************************************
* void checkFgStatus()                                                                                              *
* This will print the status of the most recent foreground  process that died.                                      *
********************************************************************************************************************/
void checkFgProc(){
    //Check Exit status of Fg process
    if(WIFEXITED(fgExitStatus) != 0){ //fg process exited normally
        //Set status to fg exit value
        status = WEXITSTATUS(fgExitStatus);
        fgTermSignal = - 5;
    }

    if(WIFSIGNALED(fgExitStatus) != 0){ //fg process was killed by signal
        fgTermSignal = WTERMSIG(fgExitStatus);
        status = -5;

    }
}

/********************************************************************************************************************
* void printStatus()                                                                                                *
* will be used to print either status or signal terminated of the last fg process.                                  *
********************************************************************************************************************/
void printStatus(){
    if(status >=  0){ //If status is not -5, it exited normally
        printf("exit value %d\n", status);
        fflush(stdout);
    }

    if(fgTermSignal > 0){  //If signal is > -5, it was terminated by a signal
        printf("terminated by signal %d\n", fgTermSignal);
        fflush(stdout);
    }
}
