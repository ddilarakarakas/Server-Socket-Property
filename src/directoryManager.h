
/**
 * Stores estateLinkedList with using date as an index.
*/
#ifndef DIRECTORY_MANAGER_H
#define DIRECTORY_MANAGER_H



void addDirectory(char ***dirList,int *maxDirCapacity,int* currentCount,char *dir);
char** getDirectoryList(char* path,int *dirListSize);
char ** filterDatasetWithGivenRange(int upperLimit,int lowerLimit,int *currentDirCount,char ***dirList);
void sortStringListAlphabetically(int sizeOfArray,char **stringList);
void ifNeedToResize(char **s1,char** s2);
void freeDirectoryList(char ***dirList,int size);
#endif