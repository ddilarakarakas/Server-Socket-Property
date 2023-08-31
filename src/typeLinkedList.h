/**
 * Stores estateDateLinkedList with using cities as an index.
*/
#ifndef TYPE_LINKED_LIST_H
#define TYPE_LINKED_LIST_H

#include "commonStructs.h"

struct typeEstateLL* createTypeEstateLL();
void addTypeEstate(struct typeEstateLL *ce,char *type,struct dateEstateLL *dateEstates);
void deleteTypeEstateLL(struct typeEstateLL **ce);
int typeEstateCount(struct typeEstateLL *e,char* type,struct date d1,struct date d2);
int typeEstateCountWithCity(struct typeEstateLL *e,char* type,struct date d1,struct date d2,char *city);


#endif