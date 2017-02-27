#include <stdint.h>
#include <stdlib.h>
#include "queue.h"
#include <stdio.h>
#include <unistd.h>
//we are using a linked list to construct our queue
//each are connected with a Node

//Source: https://gist.github.com/mycodeschool/7510222
struct node {
	void* ndata;
	struct node* next; 
};

struct queue {
	struct node* start;
	struct node* end;
	int queue_size;
};


queue_t queue_create(void)
{
	queue_t myqueue = malloc(sizeof(struct queue));
	myqueue->start = NULL;
	myqueue->end = NULL;
	myqueue->queue_size = 0;

	return myqueue;
}

int queue_length(queue_t queue);

int queue_destroy(queue_t queue)
{
	if(queue == NULL)
		return -1;
	free(queue);

	if(queue_length(queue)==0)
		return 0;
	else
		return -1;
}

int queue_enqueue(queue_t queue, void *data)
{
	if(queue == NULL || data == NULL)
		return -1;

	struct node* newEle = (struct node*)malloc(sizeof(struct node));
	newEle->ndata = data;
	newEle->next = NULL;

	if (queue->start == NULL)
		queue->start = newEle;
	else
		queue->end->next = newEle;

	queue->end = newEle;
	queue->queue_size++;
	return 0;
}

int queue_dequeue(queue_t queue, void **data)
{
    if (queue->start == NULL) { //the queue is empty
    	printf("Error: Empty queue!\n");
    	return -1;
    }

	struct node* old = queue->start;
	*data = old->ndata; //ndata holds address of thread
	queue->start = old->next;
	queue->queue_size--;
	free(old);

	return 0;
}

int queue_delete(queue_t queue, void *data)
{
	struct node* rip = queue->start;
	int returnVal = -1;

	if(rip == NULL){ // Empty queue
		printf("Error: Empty queue!\n");
		return -1;
	}

	else{

		while(rip->next != NULL) {
			if (rip->next->ndata == data) {
				rip->next = rip->next->next;
				free(rip->next->ndata);
				free(rip->next);
				returnVal = 0;
				break;
			}
		}
		queue->queue_size = 0;
		return returnVal;
	}	

}

int queue_iterate(queue_t queue, queue_func_t func)
{
	struct node* temp = queue->start;
	while (temp != NULL) {
		//Source: http://msl.cs.uiuc.edu/~lavalle/cs326a/projects/bgoc/code/queue.C [http://msl.cs.uiuc.edu/~lavalle/cs326a/projects/bgoc/code/queue.c_green] 
		(*func)(temp->ndata);
		temp = temp->next;
	}

	return 0;
}

int queue_length(queue_t queue)
{
	return queue->queue_size;
}
