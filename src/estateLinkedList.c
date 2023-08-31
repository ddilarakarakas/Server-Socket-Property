#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "estateLinkedList.h"

struct estate;
struct estateLL;

struct estateLL* createEstateLL(){
    struct estateLL* e = (struct estateLL*)malloc(sizeof(struct estateLL));
    e->front =  NULL;
    e->rear =  NULL; 
    e->size=0;
    return e; 
}

void addEstate(struct estateLL *e,int transactionId,char* city,char* streetName, int surface,int price){
    int len=0;
    /*Create new node.*/
    struct estate *temp = (struct estate*)malloc(sizeof(struct estate));
    temp->transactionId=transactionId;

    len = strlen(city);
    temp->city=(char*)malloc(len+1);
    strcpy(temp->city, city);

    len = strlen(streetName);
    temp->streetName=(char*)malloc(len+1);
    strcpy(temp->streetName, streetName);

    temp->surface = surface;
    temp->price=price;
    temp->next=NULL;

    if (e->rear==NULL){
        e->front = e->rear = temp;
        e->size=1;
        return;
    }

    e->rear->next=temp;
    e->rear=temp;
    e->size++;
}

/**
 * free all allocated memory
 **/
void deleteEstateLL(struct estateLL **e){
    struct estateLL *iter=(*e);
    struct estate *temp=NULL;

    while(iter->front!=NULL){
        temp = iter->front->next;
        free(iter->front->city);
        free(iter->front->streetName);
        free(iter->front);
        iter->front=temp;
    }

    free((*e));
}


int estateCount(struct estateLL *e){
    return e->size;
}

int estateCountWithCity(struct estateLL *e,char *city){
    struct estate *iter=e->front;
    int count=0;
    if(iter==NULL) return 0;

    while(iter->next!=NULL){

        if(strcmp(iter->city,city)==EQUAL){
            count++;
        }
        iter=iter->next;
    }
    if(strcmp(iter->city,city)==EQUAL){
        count++;
    }
    return count;
}

