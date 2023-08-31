#ifndef DATA_OPERATIONS_H
#define DATA_OPERATIONS_H

#include "commonStructs.h"

void addEstateToDatedLL(struct dateEstateLL *de,struct date d,int transactionId,
                            char* type,char *streetName, int surface,int price);

void addEstateToTypeLL(struct typeEstateLL *ce,char *city,struct date d,int transactionId,
                            char* type,char *streetName, int surface,int price);

int transactionCount(struct typeEstateLL *e,char *type,struct date d1,struct date d2);
int transactionCountWithCity(struct typeEstateLL *e,char *type,struct date d1,struct date d2,char *city);

#endif