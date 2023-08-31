#ifndef REQUESTS_QUEUE_H
#define REQUESTS_QUEUE_H
#include <pthread.h>
#include "commonStructs.h"

/*thread arguments*/
struct req{
    int fd;
    int threadId;
};

struct reqQueue{
    struct reqNode *front;
    struct reqNode *rear;
    int size;
};

struct reqNode{
    struct req *reqValue; /*Thread arguments.*/
    struct reqNode *next;
};

struct reqWithParam{
    int fd;
    struct date d1;
    struct date d2;
    char type[512];
    char city[512];
    int threadId;
};


/*Create queue.*/
struct reqQueue* createReqQueue();

/*Adds element to rear.*/
void reqEnqueue(struct reqQueue *q,struct req *value);

/*Removes element in the front.*/
struct req* reqDequeue(struct reqQueue *q);

struct req* createReqArg(int *conn_fd);

// struct req* createReqArg(int *conn_fd,struct date d1,struct date d2,char *type,char *city);

#endif