/**
 * Stores estateLinkedList with using date as an index.
*/
#ifndef DATE_ESTATE_LINKED_LIST_H
#define DATE_ESTATE_LINKED_LIST_H

#include "commonStructs.h"

/*Create ll for dateEstate*/
struct dateEstateLL* createDateEstateLL();

void addDateEstate(struct dateEstateLL *de,struct date d,struct estateLL *e);

struct dateEstateLL* createDateEstateLL();

int compareDates(struct date d1,struct date d2);

void addDateEstateBack(struct dateEstate *point,struct dateEstate *element);

void addDateEstateFront(struct dateEstate *point,struct dateEstate *element);

void addElementToProperPosition(struct dateEstateLL *de,struct date d,struct dateEstate *element);

void addDateEstate(struct dateEstateLL *de,struct date d,struct estateLL *estates);

void deleteDateEstateLL(struct dateEstateLL **de);

int dateEstateCount(struct dateEstateLL *e,struct date d1,struct date d2);

int dateEstateCountWithCity(struct dateEstateLL *e,struct date d1,struct date d2,char *city);


#endif
