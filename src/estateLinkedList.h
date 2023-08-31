#ifndef ESTATE_LINKED_LIST_H
#define ESTATE_LINKED_LIST_H

#include "commonStructs.h"

/*Create ll for estate*/
struct estateLL* createEstateLL();

void addEstate(struct estateLL *e,int transactionId,char* city,char* streetName, int surface,int price);

void deleteEstateLL(struct estateLL **e);

int estateCount(struct estateLL *e);

int estateCountWithCity(struct estateLL *e,char *city);

#endif