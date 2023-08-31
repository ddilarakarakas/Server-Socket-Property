#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "dateEstateLinkedList.h"
#include "estateLinkedList.h"
#include "typeLinkedList.h"

struct dateEstate;
struct dateEstateLL;
struct date;
struct estate;
struct estateLL;
struct typeEstate;
struct typeEstateLL;


/*Create ll for cityEstate*/
struct typeEstateLL* createTypeEstateLL(){
    struct typeEstateLL* e = (struct typeEstateLL*)malloc(sizeof(struct typeEstateLL));
    e->front =  NULL;
    e->rear =  NULL; 
    e->size=0;
    return e; 
}

/*adds date estateLL to dateEstateLL*/
void addTypeEstate(struct typeEstateLL *ce,char *city,struct dateEstateLL *dateEstates){
    int len=0;
    /*Create new node.*/
    struct typeEstate *temp = (struct typeEstate*)malloc(sizeof(struct typeEstate));

    len = strlen(city);
    temp->type=(char*)malloc(len+1);
    strcpy(temp->type, city);
    temp->dateEstates=dateEstates;
    temp->next=NULL;

    /*If ll is empty.*/
    if (ce->rear==NULL){
        ce->front = ce->rear = temp;
        ce->size=1;
        return;
    }

    ce->rear->next=temp;
    ce->rear=temp;

    ce->size++;
}

void deleteTypeEstateLL(struct typeEstateLL **ce){
    struct typeEstateLL *iter=(*ce);
    struct typeEstate *temp=NULL;

    while(iter->front!=NULL){
        temp = iter->front->next;
        deleteDateEstateLL(&(iter->front->dateEstates));
        free(iter->front->type);
        free(iter->front);
        iter->front=temp;
    }

    free((*ce));
}


int typeEstateCount(struct typeEstateLL *e,char* type,struct date d1,struct date d2){
    struct typeEstate *iter=e->front;
    int count=0;
    if(iter==NULL) return 0;
    while(iter->next!=NULL){
        if(strcmp(type,iter->type)==EQUAL){
            return dateEstateCount(iter->dateEstates,d1,d2);
        }
        iter=iter->next;
    }
    return 0;
}

int typeEstateCountWithCity(struct typeEstateLL *e,char* type,struct date d1,struct date d2,char *city){

    struct typeEstate *iter=e->front;
    if(iter==NULL) return 0;

    while(iter->next!=NULL){
        if(strcmp(type,iter->type)==EQUAL){
            return dateEstateCountWithCity(iter->dateEstates,d1,d2,city);
            
        }
        iter=iter->next;
    }
    return 0;
}

