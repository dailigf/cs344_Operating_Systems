/***********************************************************************************************************************
 * Author: Francis C. Dailig										               *
 * Title: Queue.h                                                                                                      *
 * Description: Header file to implement a queue data structure to be used with Program 4.                             *
 * This was taken from my data structure class, Worksheet 18: Linked List Queue, Pointer to Tail, 22 APR 2018          *
 * ********************************************************************************************************************/
#ifndef QUEUE_H
#define QUEUE_H

/* Define the Structs that will be used for our queue implementation*/
struct node{	//Node
	int socketFD;
	struct node *next;
};

struct queue{	//Queus data structure
	struct node *sentinel;
	struct node *front;
	struct node *last;
};

/*Declare Prototypes for functions used bye queue data structure*/
void queueInit(struct queue *q);	//function to initialize the queue
void enQueue(struct queue *q, int value);	//Function to add a socket to the queue
int getQueueFront(struct queue *q);	//Function to check the first value of the queue
int deQueue(struct queue *q);	//Fucntion to get the fron tof the queue
int isQueueEmpty(struct queue *q); //Function to see if queue is empty
void deleteQueue(struct queue *q); //Function to delete queue

#endif

