#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "directoryManager.h"
#include "commonStructs.h"
#include "dataOperations.h"
#include "dateEstateLinkedList.h"
#include "typeLinkedList.h"
#include "estateLinkedList.h"
#include "socketManager.h"
#include "requestsQueue.h"

#define SIZE 2048
#define LINE_SIZE 256
#define ERROR -1
#define FAIL -1
#define SKIP 2
#define SUCCESS 0
#define SOCKET_QUEUE_SIZE 512
#define TRUE 1

pthread_mutex_t param_m = PTHREAD_MUTEX_INITIALIZER;

/*global variables for sigint*/
char line[LINE_SIZE];
struct typeEstateLL *dataset;
int manDirListSize=0;
char **mainDirList;
int handledRequestCount=0;
int servantFd=-1;

struct servantThreads{
    pthread_t** t;
    int count;
    int capacity;
}servantThrds;

struct servantThreadArg{
    int conn_fd;
}servantThreadArgs;

struct servantThreads *threads;

void parse_arg(int argc, char* argv[],char** directoryPath,int *lowerCity,int *upperCity,char** r,int *p);
void* handleServerRequests(void *arg);
void resizeServantThreads();
void handleIncomingRequests(int *fd, struct sockaddr_in *addr,struct typeEstateLL *dataset,int servantPort);
char* getPathOfDir(char* directoryPath,char* folderPath);
int isValidDate(char *str);
void getDate(char* date,struct date *d);
int readFileAndFillDataset(char *file,struct date d,char *city,struct typeEstateLL *dataset);
void readAllDataFromDir(char *filePath,char *city,struct typeEstateLL *dataset);
void sigint_handler(int signal_number);
void printTerminal(char *message);
void printError(char *message);


int main(int argc, char* argv[]){
    char *directoryPath;
    char *ip;
    int lowerCity=0,upperCity=5,serverPort=0,error=0;
    char *filePath;
    char ** dirList;
    struct sigaction sigint_action;
    struct sockaddr_in addr;
    int servantPort;
    int portIndex=0;
    /*For blocking SIGINT, set variables*/
    sigset_t mask;    
    sigemptyset(&mask);
    sigaddset(&mask,SIGINT);
    sigset_t oldmask;
    sigemptyset(&oldmask);
    int i;
    /*Set SIGINT handler for CTRL+C.*/
    memset(&sigint_action,0,sizeof(sigint_action));
	sigint_action.sa_handler = &sigint_handler;
	if(sigaction(SIGINT,&sigint_action,NULL)==ERROR){
        sprintf(line,"\nError while sigaction operation.\n");                
        printError(line);
        exit(EXIT_FAILURE);
    }
    
    threads=(struct servantThreads*)malloc(sizeof(struct servantThreads)*1);
    threads->capacity=256;
    threads->count=0;
    threads->t=(pthread_t **)malloc(sizeof(pthread_t*)*256);

    /*Block SIGINT UNTIL thread lock is opened.*/
    error=sigprocmask(SIG_SETMASK,&mask,&oldmask);
    if(error==ERROR){
        sprintf(line,"Error while blocking SIGINT with sigprocmask.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }

    parse_arg(argc,argv,&directoryPath,&lowerCity,&upperCity,&ip,&serverPort);

    portIndex=(upperCity+lowerCity)/2;
    dataset = createTypeEstateLL();
    dirList=getDirectoryList(directoryPath,&manDirListSize);
    mainDirList = filterDatasetWithGivenRange(upperCity,lowerCity,&manDirListSize,&dirList);

    line[0]='\0';
    sprintf(line,"loaded dataset, cities %s %s\n",mainDirList[0],mainDirList[manDirListSize-1]);
    printTerminal(line);

    for(i=0;i<manDirListSize;i++){
        filePath=getPathOfDir(directoryPath,mainDirList[i]);
        readAllDataFromDir(filePath,mainDirList[i],dataset);
        free(filePath);
    }

    servantPort = findAvailablePortForServantSocketAndListen(&servantFd,&addr,portIndex);

    /*Critical section ends unblock SIGINT.*/
    error=sigprocmask(SIG_SETMASK,&oldmask,&mask);
    if(error==ERROR){
        sprintf(line,"Error while unblocking SIGINT with sigprocmask.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }


    error=sendServantPortToServer(serverPort,servantPort,ip,lowerCity,upperCity,
        mainDirList,manDirListSize);
    if(error==ERROR){
        deleteTypeEstateLL(&dataset);
        freeDirectoryList(&mainDirList,manDirListSize);
        for(i=0;i<threads->count;i++){
            pthread_join(*(threads->t[i]),NULL);
            free(threads->t[i]);
        }
        free(threads->t);
        free(threads);
        if(close(servantFd)<0){
            sprintf(line,"Error while closing serverfd.\n");
            printError(line);
        }
        exit(EXIT_FAILURE);
    }

    handleIncomingRequests(&servantFd,&addr,dataset,servantPort);

    freeDirectoryList(&mainDirList,manDirListSize);
    deleteTypeEstateLL(&dataset);
    return 0;
}


/*Parses command line arg with using getopt.*/
void parse_arg(int argc, char* argv[],char** directoryPath,int *lowerCity,int *upperCity,char** r,int *p){
	int opt;
    int flag=0;
    int error=0;
    char *ch;
    char line[LINE_SIZE];
	while((opt = getopt(argc, argv, "d:c:r:p:"))!=ERROR){
		switch(opt){
			case 'd':
				(*directoryPath)=optarg;
				break;
			case 'c':
                ch = strtok(optarg, "-");
                (*lowerCity)=atoi(ch);
                ch = strtok(NULL, " -");
                (*upperCity)=atoi(ch);
				break;
			case 'r':
				(*r)=optarg;
                flag++;
				break;
			case 'p':
				(*p)=atoi(optarg);
				break;
			default:
                sprintf(line,"\nPlease provide proper command line arguement!\n");
                printError(line);
                sprintf(line,"./servant -d directoryPath -c 10-19 -r IP -p PORT\n");
                printError(line);
				exit(EXIT_FAILURE);
		}
	}
	if(argc!=9){
        sprintf(line,"\nPlease provide proper command line arguement!\n");
        printError(line);
        sprintf(line,"./servant -d directoryPath -c 10-19 -r IP -p PORT");                
        printError(line);
		exit(EXIT_FAILURE);
    }
}

void* handleServerRequests(void *arg){
    int error;
    int result;
    struct servantThreadArg *s=((struct servantThreadArg*)arg);

    error=pthread_mutex_lock(&param_m);

    if(error!=0){
        sprintf(line,"Failed while locking mutex.\n");
        printTerminal(line);
        exit(EXIT_FAILURE);
    }

    handledRequestCount++;x
    error=pthread_mutex_unlock(&param_m);
    if(error!=0){
        sprintf(line,"Failed while locking mutex.\n");
        printTerminal(line);
        exit(EXIT_FAILURE);
    }

    struct reqWithParam* r= readRequest(&(s->conn_fd));
    

    if(r->city[0]=='\0'){
        result=transactionCount(dataset,r->type,r->d1,r->d2);
    }else{
        result=transactionCountWithCity(dataset,r->type,r->d1,r->d2,r->city);
    }

    /*send back result*/
    line[0]='\0';
    sprintf(line,"%d*",result);
    error=write(s->conn_fd,line,strlen(line));
    if(error==ERROR){
        sprintf(line,"Error while write operation.!");
        printError(line);
    }
    free(r);
    close(s->conn_fd);
    free(s);
    
}

void resizeServantThreads(){
    int i;
    pthread_t **temp=(pthread_t **)malloc(sizeof(pthread_t *)*(threads->capacity*2));

    for(i=0;i<threads->count;i++){
        temp[i]=threads->t[i];
    }
    free(threads->t);
    threads->t=temp;
    threads->capacity=(threads->capacity*2);
}

void handleIncomingRequests(int *fd, struct sockaddr_in *addr,struct typeEstateLL *dataset,int servantPort){
    int conn_fd;
    socklen_t cLen;
    char buffer='\0';
    int error=0;
    int result=0;
    int count=0;
    int dateCount=0;
    int len=0;
    struct servantThreadArg *arg;
    line[0]='\0';
    sprintf(line,"listening port %d \n",servantPort);
    printTerminal(line);
    while(TRUE){

        cLen = sizeof (addr);
        if(((conn_fd) = accept((*fd),(struct sockaddr*) addr,&cLen)) < SUCCESS) {
            close((*fd));
            exit(EXIT_FAILURE);
        }

        arg = (struct servantThreadArg*)malloc(sizeof(struct servantThreadArg)*1);
        arg->conn_fd=dup(conn_fd);
        threads->t[threads->count]=(pthread_t*)malloc(sizeof(pthread_t)*1);
        error=pthread_create(threads->t[threads->count],NULL,handleServerRequests,arg);
        threads->count++;

        if(threads->count >= threads->capacity){
            resizeServantThreads();
        }
    }
}

char* getPathOfDir(char* directoryPath,char* folderPath){
    int len=strlen(directoryPath) + strlen(folderPath);
    char *filePath = (char *)malloc(sizeof(char)*(len+3));
    filePath[0]='\0';
    strcat(filePath,directoryPath);
    strcat(filePath,"/");
    strcat(filePath,folderPath);
    return filePath;
}

int isValidDate(char *str){
    int i=0, count=0;
    for(i = 0; str[i] != '\0'; i++){
        if(str[i] == '-'){
            count++;
        }else if(!(str[i] >= '0' && str[i] <= '9')){
            return 0;
        }
    }
    return (count==2 && str[strlen(str)-1] >= '0' && str[strlen(str)-1] <= '9');
}

void getDate(char* date,struct date *d){
    char* token = strtok(date, "-");
    d->day=atoi(token);
    token = strtok(NULL, "-");
    d->month=atoi(token);
    token = strtok(NULL, "-");
    d->year=atoi(token);
}

int readFileAndFillDataset(char *file,struct date d,char *city,struct typeEstateLL *dataset){
    int fd;
    char buffer[2];
    int readed_byte=0,count=0,len=0;
    char value[SIZE];

    int transactionId=-1;
    char* type=NULL;
    char* street=NULL;
    int surface=0;
    int price=0;

    fd=open(file,O_RDONLY);
	if(fd==ERROR){
        sprintf(line,"\nFile couldn't open.\n");                
        printError(line);
        exit(EXIT_FAILURE);
    }

    buffer[0]=' ';
    value[0]='\0';
    while((readed_byte=read(fd,&buffer,1))!=0){
        buffer[1]='\0';
        if(readed_byte==ERROR){
            sprintf (line, "Failed to read file.\n");
            printError(line);
            return FAIL;
        }

        if(buffer[0]!=' ' && buffer[0]!='\n'){
            strcat(value,buffer);
        }
        else if( buffer[0]=='\n'){
            price=atoi(value);
            addEstateToTypeLL(dataset,city,d,transactionId,type,street,surface,price);
            free(type);
            free(street);
            value[0]='\0';
            count=0;
        }
        else{
            if(count==0){
                transactionId=atoi(value);
            }else if(count==1){
                len=strlen(value);
                type=(char*)malloc(sizeof(char)*(len+1));
                strcpy(type,value);
            }else if(count==2){
                len=strlen(value);
                street=(char*)malloc(sizeof(char)*(len+1));
                strcpy(street,value);
            }else if(count==3){
                surface=atoi(value);
            }
            value[0]='\0';
            count++;
        }
    }


    if(close(fd)==ERROR){
        exit(EXIT_FAILURE);
    }
}

void readAllDataFromDir(char *filePath,char *city,struct typeEstateLL *dataset){
    char **fileList;
    int size,i=0;
    fileList=getDirectoryList(filePath,&size);
    struct date d;
    char *path;
    
    for(i=0;i<size;i++){
        if(!isValidDate(fileList[i])) continue;
        path = getPathOfDir(filePath,fileList[i]);
        getDate(fileList[i],&d);
        readFileAndFillDataset(path,d,city,dataset);
        free(path);
    }

    freeDirectoryList(&fileList,size);
}

/*SIGINT Handler.*/
void sigint_handler(int signal_number){
    line[0]='\0';
    int error;
    int i;
    error=pthread_mutex_lock(&param_m);
    if(error!=0){
        sprintf(line,"Failed while locking mutex.\n");
        printTerminal(line);
        exit(EXIT_FAILURE);
    }

    sprintf(line,"termination message received, handled %d requests in total\n",handledRequestCount);
    printTerminal(line);

    error=pthread_mutex_unlock(&param_m);
    if(error!=0){
        sprintf(line,"Failed while locking mutex.\n");
        printTerminal(line);
        exit(EXIT_FAILURE);
    }

    freeDirectoryList(&mainDirList,manDirListSize);
    deleteTypeEstateLL(&dataset);
    if(servantFd!=-1){
        close(servantFd);
    }

    for(i=0;i<threads->count;i++){
        pthread_join(*(threads->t[i]),NULL);
        free(threads->t[i]);
    }
    free(threads->t);
    free(threads);
    exit(EXIT_SUCCESS);
}

void printTerminal(char *message){
    write(STDOUT_FILENO, message,strlen(message));
}

void printError(char *message){
    char errorText[512];
    errorText[0]='\0';
    sprintf(errorText,"%s %s",message,strerror(errno));
    write(STDOUT_FILENO, errorText,strlen(errorText));
}
