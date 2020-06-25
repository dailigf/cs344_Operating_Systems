#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>


//Program2 Outline taken from the lecture notes
//Defining Enumerated Type to Represent to Rooms
enum ROOMS{ Terra, Luna, Mars, Ultramar, Baal, Fenris, Caliban, Medusa, Nocturne, Olympia };
enum ROOM_TYPE { START_ROOM, MID_ROOM, END_ROOM };

//Create a Constant char array that will be used when trying to get the string of a ROOM
const char* Room_Strings[] = {"Terra", "Luna", "Mars", "Ultramar", "Baal", "Fenris", "Caliban", "Medusa", "Nocturne", "Olympia"};
const char* Type_String[] = { "START_ROOM", "MID_ROOM", "END_ROOM" };

/**********************************************************************************************
 * struct Room                                                                                *
 * Define a room struct.                                                                      *
 * *******************************************************************************************/
struct Room{
    char name[10];		//Name buffer
    int numConnections;		//Value will track number of connections
    int connectionArr[6];	//Value will hold int representation of the rooms
    char type[16];		//Start, Mid, or End
    bool isFull;		//track to see if room has six connections
};

/**********************************************************************************************
 * getRoomString(int ROOMS)                                                                   *
 * This Function will return the Room String.                                                 *
 *********************************************************************************************/
const char* getRoomString(int room){
        return Room_Strings[room];

}
/**********************************************************************************************
 * getTypeString(int type)                                                                    *
 * This Function will return the Type String.                                                 *
 *********************************************************************************************/
const char* getTypeString(int type){
    return Type_String[type];
}
/**********************************************************************************************
 * isNameTaken(struct Room array[], int rmName)                                               *
 * Function will check to see if rmName exits in the array.                                   *
 * Returns 1 if it exist, 0 otherwise.                                                        *
 *********************************************************************************************/
int isNameTaken(struct Room array[], int rmName){
    char name[10];		//buffer to hold name being examined
    strcpy(name, getRoomString(rmName));
    int i = 0;
    for(i = 0; i < 7; i++){	//Iterate through room array to see if name is taken
        if(strcmp(array[i].name, name) == 0){
            return 1; //Name taken
        }
    }
    return 0; //Name not taken
}
/**********************************************************************************************
 * assignNameAndType(struct Room array[])                                                     *
 * This Function will assign a type and random name to each struct in the array.              *
 *********************************************************************************************/
int asignNameAndType(struct Room array[]){
    int rmName;    //value to hold random number for selecting room and room type;
    int tmp1, tmp2;//tmp varialbes needed to hold random numbers

    char name[10];	//Initailzie name buffer
    memset(name, '\0', 10);

    int i;
    //Iterate through the array of room to initialize values
    for(i = 0; i < 7; i++){
        array[i].isFull = false;            //Initialize isFull to false
        array[i].numConnections = 0;         //Initialize numConnections = 0
        int x;
        for(x = 0; x < 6; x++){             //Initialize connectionArr in each room to -5
            array[i].connectionArr[x] = -5;
        }
        if(i == 0){	//If looking at first element, assign any random room
            rmName = rand() % 10;		//Select a random name from the Enum Array of room names
            strcpy(name, getRoomString(rmName));
            strcpy(array[i].name, name);
            strcpy(array[i].type, getTypeString(1));
        }else{
            //assign a random number to rmName
            do{		//Keep selecting a room name until we find one that is not taken
                rmName = rand() % 10;
            }while(isNameTaken(array, rmName) == 1);
            strcpy(name, getRoomString(rmName));
            strcpy(array[i].name, name);
            strcpy(array[i].type, getTypeString(1));
        }

    }

    //Select a Random room in the array to have the Start_Room;
    tmp1 = rand() % 7;
    strcpy(array[tmp1].type, getTypeString(0));

    //Select a Random room in the array to have the End_Room;
    //Room can bethe same as tmp1
    do{
        tmp2 = rand() % 7;
    }while(tmp2 == tmp1);
    strcpy(array[tmp2].type, getTypeString(2));
    return 0;
}
/**********************************************************************************************
 * isGraphFull()                                                                              *
 * This function iterate through the room arrray to see if each room is full.  If not it will *
 * return false, else true.                                                                   *
 *********************************************************************************************/
bool isGraphFull(struct Room array[]){
    int i;
    int numRoomsFull = 0;
    //Iterate through the struct Room array and check each 'isFull' variable
    for(i = 0; i < 7; i++){
        if(array[i].isFull == true){ 
            numRoomsFull++; 
        }
    }

    //If the number of rooms that are full is < 7, graph is not full
    if(numRoomsFull < 7){
        return false;
    }else{ return true; }
}

/**********************************************************************************************
 * isGraphSparse(struct Room array[])                                                         *
 * Function will check to see if there is a minimum of three connections in each room.        *
 *********************************************************************************************/
bool isGraphSparse(struct Room array[]){
    //Iterate through each struct Room in the array, check their numConnections
    //If numConnections for any room is < 3, Graph does not meet the minimum requirement
    //That each room has at least 3 connectioncs, therefore is sparse.
    int i;
    for(i = 0; i < 7; i++){
        if(array[i].numConnections < 3){ return true; }
    }
    return false;
}
/**********************************************************************************************
 * areRoomConnected(struct Room array[], int roomA, int roomB)                                *
 * Function will check to see if there is an existing connection between roomA and roomB.     *
 *********************************************************************************************/
bool areRoomsConnected(struct Room array[], int roomA, int roomB){
    //Checking RoomA connectionArr to see if there is an element == roomB
    //If so, then roomA has a connection to roomB.
    int i;
    for(i = 0; i < array[roomA].numConnections; i++){
        if(array[roomA].connectionArr[i] == roomB){ return true; }
    }
    return false;
}
/**********************************************************************************************
 * addConnection(struct Room array[], int roomA, int roomB)                                   *
 * Function will add connection between roomA and roomB, increment their counters, check to   *
 * to see if they are full.                                                                   *
 *********************************************************************************************/
int addConnection(struct Room array[], int roomA, int roomB){
    int indexA = array[roomA].numConnections, indexB = array[roomB].numConnections;

    //Add roomB to roomA's connection Array
    //Also check to see if the number of connection is in roomA is == 6, if so,
    //room is full, set 'isFull' to true
    array[roomA].connectionArr[indexA] = roomB;
    array[roomA].numConnections++;
    if(array[roomA].numConnections == 6){ array[roomA].isFull = true; }

    //Add roomA to roomB's connection Array
    //Also check to see if the number of connection is in roomB is == 6, if so,
    //room is full, set 'isFull' to true
    array[roomB].connectionArr[indexB] = roomA;
    array[roomB].numConnections++;
    if(array[roomB].numConnections == 6){ array[roomB].isFull = true; }

    return 0;
}

/**********************************************************************************************
 * printArray(struct Room arra[])                                                             *
 * Function will print the room Array and their connections, used for diagnostics.            *
 *********************************************************************************************/
int printArray(struct Room array[]){
    int i;
    for(i = 0; i < 7; i++){
        printf("Room Name: %s\n", array[i].name);
        printf("Number of Connections: %d\tIsFull: %d\n", array[i].numConnections, array[i].isFull);
        int x;
        for(x = 0; x < array[i].numConnections; x++){
            printf("Connection %d: %s\n", x+1, array[array[i].connectionArr[x]].name);
        }
        printf("ROOM TYPE: %s\n", array[i].type);
    }
    return 0;
}
/**********************************************************************************************
 * writeToFiles(struct Room array[])                                                          *
 * Functino will create directory to write room Files to and will write the roon information  *
 * to the files.                                                                              *
 *********************************************************************************************/
int writeToFiles(struct Room array[]){
    FILE *fp;	//Variable for file pointer
    int tmp;    //tmp variable to hold status
    char dirPrefix[] = "dailigf.rooms."; //prefix of the directory
    pid_t PID = getpid(); 	//PID variable

    char directory[24];		//char buffer to hold global name of the directory
    memset(directory, '\0', 24); //initialize buffer to nulls

    char fileName[32];		//char buffer to hold fileName
    memset(fileName, '\0', 32); //Initialize buffer to nulls

    sprintf(directory, "%s%d", dirPrefix, PID); //concatenating the prefix with the PID

    tmp = mkdir(directory, 0777); //Creating the driecting to hold the room files
    if(tmp < 0){
        fprintf(stderr, "Could not create %s\n", directory);
        perror("Error in buildRooms()");
        exit(1);
    }

    int i;
    for(i = 0; i < 7; i++){
        sprintf(fileName, "%s/%s", directory, array[i].name);  //setting the filename
        fp = fopen(fileName, "a+"); //Opening the file with append permissions, will create it does not exist.
        if(fp == NULL){
            fprintf(stderr, "Could not open and create %s\n", fileName);
            perror("Error in writeToFiles()");
            exit(1);
        }

        fprintf(fp, "ROOM NAME: %s\n", array[i].name);  //Writing the room's name to file
        
	//For loop will iterate through each struct Room element in the room array
	//It will write the connection to file according to format specified in the assignment
        int x;
        for(x = 0; x < array[i].numConnections; x++){
            fprintf(fp, "CONNECTION %d:  %s\n", x+1, array[array[i].connectionArr[x]].name);
        }

        fprintf(fp, "ROOM TYPE: %s\n", array[i].type);	//Write the ROOM_Type to the file
    }
    return 0;
}
/**********************************************************************************************
 * buildRooms()                                                                               *
 * This Function will build the directory and rooms.                                          *
 * Will need an An array of structs passed to it
 *********************************************************************************************/
void buildRooms(){
    srand(time(0));             //seed random number generator
    struct Room roomArray[7];   
    int roomA, roomB;           //Variables needed when adding connection

    //Assign initial rooms and room types to the room array.
    asignNameAndType(roomArray);

    //Select to Rooms to connect until: 1) Minimum number of connections in each room is satisfied and 2) Graph is full.
    do{
        //Select a room, roomA, that is not Full.
        do{
            roomA = rand() % 7;
        }while(roomArray[roomA].isFull);


        //Select a room, roomB, that is: 1) not roomA, 2) not Full, 3) Does not have a connection to roomA
        do{
            roomB = rand() % 7;
        }while(roomB == roomA || roomArray[roomB].isFull || areRoomsConnected(roomArray, roomA, roomB));

        addConnection(roomArray, roomA, roomB);

    }while(!isGraphFull(roomArray) && isGraphSparse(roomArray));
    writeToFiles(roomArray);


}
/**********************************************************************************************
 * createGraph()                                                                              *
 * This Function will create all connections in graph.                                        *
 *********************************************************************************************/
void createGraph(){
}


int main(void){
        buildRooms();
        return 0;
}
