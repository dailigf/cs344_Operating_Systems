/***********************************************************************************************************************
 * Author: Francis C. Dailig										               *
 * Title: Queue.c                                                                                                      *
 * Description: Header file to implement a queue data structure to be used with Program 4.                             *
 * This was taken from my data structure class, Worksheet 18: Linked List Queue, Pointer to Tail, 22 APR 2018          *
 * ********************************************************************************************************************/
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/**********************************************************************************************************************
 * void queueInit(struct queue *q)                                                                                    *
 * Function will initialize the queue                                                                                 *
 * ********************************************************************************************************************/
void queueInit(struct queue *q){
	struct node *lnk = (struct node *)malloc(sizeof(struct node)); //allocate space for a new node
	assert(lnk != 0); //Ensure link was created
	lnk->socketFD = -5;	//Invalid value to indicate that this is the sentinel
	lnk->next = NULL;
	q->sentinel = lnk;
	q->front = q->last = q->sentinel;
}

/**********************************************************************************************************************
 * void enQueue(struct queue *q, int value)                                                                           *
 * Function will create a new node and add it to the queue                                                            *
 * ********************************************************************************************************************/
void enQueue(struct queue *q, int value){
	struct node *lnk = (struct node *)malloc(sizeof(struct node)); //allocate space for a new node
	assert( lnk != 0);
	lnk->socketFD = value; //Set to value
	lnk->next = q->sentinel;	//Set lnk->next to wrap around to the sentinel
	if(q->sentinel->next == NULL){	//Queue is empty
		q->front = lnk;	//Set front to point to the new link
	}
	q->last->next = lnk;  //Set the previous last lnk->next to point to the newly created link
			      //If empty q->last = sentinel, this will set sentinel->next to point to the new lnk
	q->last = lnk; 	//Set last to be the current lnk

}
/**********************************************************************************************************************
 * void getQueueFront(struct queue *q)                                                                                *
 * This function will return the value in the front of the queueu.                                                    *
 * ********************************************************************************************************************/
int getQueueFront(struct queue *q){
	if(q->sentinel == NULL){	//Queue is empty return an invalid number
		return -5;
	}

	return q->front->socketFD;
}
/**********************************************************************************************************************
 * int  deQueue(struct queue *q)                                                                                      *
 * Function will remove the front of the queue                                                                        *
 * ********************************************************************************************************************/
int deQueue(struct queue *q){
	if(q->sentinel->next == NULL){	//Queue is empty return invalid number
		return -5;
	}
	int retVal = q->front->socketFD;
	struct node *lnk = q->front;	//This lnk will be removed from the queue

//	q->front = lnk->next;	//Set the current front of the queue

	//check to see if the values are NULL, if so then we removed the last link in the queue
	//Set front and last to point to the sentinel
	if(lnk->next == q->sentinel){
		q->sentinel->next = NULL;
		q->front = q->sentinel;
		q->last = q->sentinel;
	}else{
		q->front = lnk->next;	//Set the current front of the queue
	}

	free(lnk);
	return retVal;
};	//Fucntion to get the fron tof the queue


/**********************************************************************************************************************
 * void deleteQueue(struct queue *q)                                                                                  *
 * Function will delete queue                                                                                         *
 * ********************************************************************************************************************/
void deleteQueue(struct queue *q){
	while(q->sentinel->next != NULL){
		deQueue(q);
	}
	free(q->sentinel);
	free(q);
}
