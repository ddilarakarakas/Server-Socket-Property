#ifndef COMMON_STRUCTS_H
#define COMMON_STRUCTS_H

#define BIGGER 1
#define SMALLER -1
#define EQUAL 0


struct estate{
    int transactionId;
    char *city;
    char *streetName;
    int surface;
    int price;
    struct estate *next;
};

struct estateLL{
    struct estate *front;
    struct estate *rear;
    int size;
};

struct date{
    int day;
    int month;
    int year;
};

struct dateEstate{
    struct date d;
    struct estateLL *estates;   
    struct dateEstate *next;
    struct dateEstate *back;
};

struct dateEstateLL{
    struct dateEstate *front;
    struct dateEstate *rear;
    int size;
};

struct typeEstate{
    char* type;
    struct dateEstateLL *dateEstates;   
    struct typeEstate *next;
};

struct typeEstateLL{
    struct typeEstate *front;
    struct typeEstate *rear;
    int size;
};

#endif
