#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>


/**************************************************************************************************************************
 * main()                                                                                                                 *
 * This is the mian function                                                                                              *
 * ***********************************************************************************************************************/
int main(int argc, char* argv[]){
	long int keyLength = strtol(argv[1], NULL, 0); //Variable to hold the length of the key
	char *key;

	if(keyLength == 0){
		//print to to stderr and exit 1
		fprintf(stderr, "%s is not a valid number\n", argv[1]);
		return 1;
	}else{
		FILE* fp;	//File pointer, write key to this file
		fp = fopen("keyFile.txt", "w+");	//open keyFile.txt
		key = (char*)malloc(sizeof(char) * keyLength);	//Allocate space ofr buffer that will hold the kye

		int i, tmp; //i - iterate through loop, tmp to hold the random value
		for(i = 0; i < keyLength; i++){
			tmp = rand() % (91 + 1 - 65) + 65;	//create a random number from 65 - 91,  if 91 rolled, it will be space
			if(tmp == 91){
				tmp = 32;	//if 91 is rolled, it indicates space, set tmp to ascii value 32, which is space
			}
			key[i] = (char)tmp;	//insert the random value in the key buffer
		}

		key[keyLength] = '\n';		//Make last value \n
		fprintf(fp, "%s", key);		//Wirte the key to the file
		printf("%s", key);
		free(key);
		fclose(fp);
		return 0;
	}

	return 1;
}
