#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "requestsQueue.h"
#include "commonStructs.h"
struct req;
struct reqQueue;
struct reqNode;
struct reqWithParam;

/*Create queue.*/
struct reqQueue* createReqQueue(){
    struct reqQueue* q = (struct reqQueue*)malloc(sizeof(struct reqQueue)); 
    q->front = q->rear = NULL;
    q->size=0; 
    return q; 
}

/*Adds element to rear.*/
void reqEnqueue(struct reqQueue *q,struct req *value){

    /*Create new node.*/
    struct reqNode *temp = (struct reqNode*)malloc(sizeof(struct reqNode));
    temp->reqValue=value;
    temp->next=NULL;
    /*If queue is empty.*/
    if(q->rear==NULL){
        q->front = q->rear = temp;
        q->size++;
        return;
    }
    q->rear->next=temp;
    q->rear=temp;
    q->size++;
}

/*Removes element in the front.*/
struct req* reqDequeue(struct reqQueue *q){
    struct req *return_value=0;

    /*If queue is empty return fail.*/
    if(q->front == NULL){
        return NULL;
    }    
    struct reqNode *temp = q->front;
    q->front = q->front->next;
    if(q->front == NULL){
        q->rear=NULL;
    }
    return_value=temp->reqValue;
    free(temp);
    q->size--;
    return return_value;
}

struct req* createReqArg(int *conn_fd){
    struct req *arg=NULL;

    arg=(struct req *)malloc(sizeof(struct req));

    arg->fd=dup((*conn_fd));

    //close((*conn_fd));
    
    return arg;
}
