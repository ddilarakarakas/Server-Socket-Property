#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "dataOperations.h"
#include "dateEstateLinkedList.h"
#include "typeLinkedList.h"
#include "estateLinkedList.h"

#define EQUAL 0

struct dateEstate;
struct dateEstateLL;
struct typeEstate;
struct typeEstateLL;
struct date;
struct estate;
struct estateLL;

void addEstateToDatedLL(struct dateEstateLL *de,struct date d,int transactionId,
                            char* city,char *streetName, int surface,int price){

    struct dateEstate *iter=de->front;

    /*adds if any estate list for spesific date exist*/
    while(iter!=NULL){
        if(compareDates(iter->d,d)==EQUAL){
            addEstate(iter->estates,transactionId,city,streetName,surface,price);
            return;
        }
        iter=iter->next;
    }

    /*create estate list for spesific date and adds it to estateDateList*/
    struct estateLL *estates=createEstateLL();
    addEstate(estates,transactionId,city,streetName,surface,price);

    addDateEstate(de,d,estates);
}


void addEstateToTypeLL(struct typeEstateLL *ce,char *city,struct date d,int transactionId,
                            char* type,char *streetName, int surface,int price){

    struct typeEstate *iter=ce->front;

    /*adds if any estate list for spesific type exist*/
    while(iter!=NULL){
        if(strcmp(type,iter->type)==EQUAL){
            addEstateToDatedLL(iter->dateEstates,d,transactionId,city,streetName,surface,price);
            return;
        }
        iter=iter->next;
    }

    /*create estate list for spesific date and adds it to estateDateList*/
    struct dateEstateLL *dateEstates=createDateEstateLL();
    addEstateToDatedLL(dateEstates,d,transactionId,city,streetName,surface,price);

    addTypeEstate(ce,type,dateEstates);
}

int transactionCount(struct typeEstateLL *e,char *type,struct date d1,struct date d2){
    return typeEstateCount(e,type,d1,d2);
}

int transactionCountWithCity(struct typeEstateLL *e,char *type,struct date d1,struct date d2,char *city){
    return typeEstateCountWithCity(e,type,d1,d2,city);
}


