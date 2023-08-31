#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "dateEstateLinkedList.h"
#include "estateLinkedList.h"

struct dateEstate;
struct dateEstateLL;
struct date;
struct estate;
struct estateLL;


/*Create ll for dateEstate*/
struct dateEstateLL* createDateEstateLL(){
    struct dateEstateLL* e = (struct dateEstateLL*)malloc(sizeof(struct dateEstateLL));
    e->front =  NULL;
    e->rear =  NULL; 
    e->size=0;
    return e; 
}

int compareDates(struct date d1,struct date d2){
    if(d1.year!=d2.year){
        if(d1.year>d2.year) return BIGGER;
        else return SMALLER;
    }

    if(d1.month!=d2.month){
        if(d1.month>d2.month) return BIGGER;
        else return SMALLER;
    }

    if(d1.day!=d2.day){
        if(d1.day>d2.day) return BIGGER;
        else return SMALLER;
    }

    return EQUAL;
}

/**
 * Add element to back of the given point
*/
void addDateEstateBack(struct dateEstate *point,struct dateEstate *element){
    element->next = point;
    element->back = point->back;
    if(point->back!=NULL){
        point->back->next = element;
    }
    point->back = element;
}

/**
 * Add element to front of the given point
*/
void addDateEstateFront(struct dateEstate *point,struct dateEstate *element){
    element->next = point->next;
    element->back = point;
    if(point->next!=NULL){
        point->next->back = element;
    }
    point->next = element;
}

/**
 * Adds real estate element to proper position with respect to date.
 * 
*/
void addElementToProperPosition(struct dateEstateLL *de,struct date d,struct dateEstate *element){
    struct dateEstate *iter=de->front;

    while(iter->next!=NULL){
        if(compareDates(iter->d,d)!=SMALLER){
            addDateEstateBack(iter,element);
            break;
        }
        iter=iter->next;
    }

    if(iter->next==NULL){
        if(compareDates(iter->d,d)!=SMALLER){
            addDateEstateBack(iter,element);
        }else{
            addDateEstateFront(iter,element);
            de->rear=element;
        }
    }

    if(de->front->back!=NULL){
        de->front = de->front->back;
    }
}

/*adds date estateLL to dateEstateLL*/
void addDateEstate(struct dateEstateLL *de,struct date d,struct estateLL *estates){

    /*Create new node.*/
    struct dateEstate *temp = (struct dateEstate*)malloc(sizeof(struct dateEstate));
    temp->d=d;
    temp->estates=estates;
    temp->next=NULL;
    temp->back=NULL;

    /*If ll is empty.*/
    if (de->rear==NULL){
        de->front = de->rear = temp;
        de->size=1;
        return;
    }

    addElementToProperPosition(de,d,temp);
    
    de->size++;
}

void deleteDateEstateLL(struct dateEstateLL **de){
    struct dateEstateLL *iter=(*de);
    struct dateEstate *temp=NULL;

    while(iter->front!=NULL){
        temp = iter->front->next;
        deleteEstateLL(&(iter->front->estates));
        free(iter->front);
        iter->front=temp;
    }

    free((*de));
}


int dateEstateCount(struct dateEstateLL *e,struct date d1,struct date d2){
    struct dateEstate *iter=e->front;
    int count=0;
    if(iter==NULL) return 0;
    while(iter->next!=NULL){
        if(compareDates(iter->d,d1)!=SMALLER && compareDates(iter->d,d2)!=BIGGER){
            count+=estateCount(iter->estates);
        }
        iter=iter->next;
    }
    if(compareDates(iter->d,d1)!=SMALLER && compareDates(iter->d,d2)!=BIGGER){
        count+=estateCount(iter->estates);
    }

    return count;
}

int dateEstateCountWithCity(struct dateEstateLL *e,struct date d1,struct date d2,char *city){
    struct dateEstate *iter=e->front;
    int count=0;
    if(iter==NULL) return 0;

    while(iter->next!=NULL){
        if(compareDates(iter->d,d1)!=SMALLER && compareDates(iter->d,d2)!=BIGGER){
            count+=estateCountWithCity(iter->estates,city);
        }
        iter=iter->next;
    }
    if(compareDates(iter->d,d1)!=SMALLER && compareDates(iter->d,d2)!=BIGGER){
        count+=estateCountWithCity(iter->estates,city);
    }
    return count;
}

