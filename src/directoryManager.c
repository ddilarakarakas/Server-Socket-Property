#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "directoryManager.h"

#define ARRAY_SIZE 256
#define TEMP_SIZE 512
#define EQUAL 0


/**
 * This file helps to bring directoryh list in dataset folder.
*/
void addDirectory(char ***dirList,int* maxDirCapacity,int* currentCount,char *dir){
    int i=0;
    int len=0;
    if((*maxDirCapacity)<=(*currentCount)){
        (*maxDirCapacity)=(*maxDirCapacity)*2;
        char **temp = (char **)malloc(sizeof(char *)*(*maxDirCapacity));

        for(i=0;i<(*currentCount);i++){
            temp[i]=(*dirList)[i];
        }
        free((*dirList));
        (*dirList)=temp;
    }  
    len = strlen(dir);
    (*dirList)[(*currentCount)]=(char*)malloc(sizeof(char)*(len+1));
    strcpy((*dirList)[(*currentCount)], dir);
    (*currentCount)++;
}



char** getDirectoryList(char* path,int *dirListSize){
    int maxDirCapacity=256;
    int currentDirCount=0,count=0;
    char **dirList = (char **)malloc(sizeof(char *)*maxDirCapacity);
    DIR *dir;
    struct dirent *directory;
    dir = opendir(path);
    if (dir){
        while ((directory = readdir(dir)) != NULL){
            if(!(strcmp(directory->d_name,".")==EQUAL || strcmp(directory->d_name,"..")==EQUAL)){
                addDirectory(&dirList,&maxDirCapacity,&currentDirCount,directory->d_name);
            }
        }
        closedir(dir);
    }
    (*dirListSize)=currentDirCount;
    return dirList;
}

char ** filterDatasetWithGivenRange(int upperLimit,int lowerLimit,int *currentDirCount,char ***dirList){
    char **mainDirList = (char **)malloc(sizeof(char *)*(upperLimit-lowerLimit+1));
    int index=0;
    sortStringListAlphabetically((*currentDirCount),(*dirList));


    /*Filter dataset with given range*/
    for(int i=0;i<(*currentDirCount);i++){
        if((i+1)>=lowerLimit && (i+1)<=upperLimit){
            mainDirList[index++]=(*dirList)[i];
        }else{
            free((*dirList)[i]);
        }
    }
    free((*dirList));
    (*currentDirCount)=index;
    return mainDirList;
}

void sortStringListAlphabetically(int sizeOfArray,char **stringList){

    char *temp;
    for(int i=0; i<sizeOfArray; i++){
        for(int j=0; j<sizeOfArray-1-i; j++){
            if(strcmp(stringList[j],stringList[j+1]) > EQUAL){
                temp = stringList[j];
                stringList[j]=stringList[j+1];
                stringList[j+1]=temp;
            }
        }
    }
}

void ifNeedToResize(char **s1,char** s2){
    char temp[512];

    if(strlen((*s1)) == strlen((*s2))){
        return;
    }else if(strlen((*s1)) < strlen((*s2))){
        strcpy(temp, (*s1));
        free((*s1));
        (*s1) = (char*)malloc(sizeof(malloc)*strlen((*s2))+1);
        strcpy((*s1),temp);
    }else if(strlen((*s1)) > strlen((*s2))){
        strcpy(temp, (*s2));
        free((*s2));
        (*s2) = (char*)malloc(sizeof(malloc)*strlen((*s2))+1);
        strcpy((*s2),temp);
    }
}

void freeDirectoryList(char ***dirList,int size){
    for(int i=0;i<size;i++){
        free((*dirList)[i]);
    }
    free((*dirList));
}