#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "commonStructs.h"
#include "requestsQueue.h"

#define SIZE 2048
#define LINE_SIZE 2048
#define ERROR -1
#define FAIL -1
#define SKIP 2
#define SUCCESS 0
#define SOCKET_QUEUE_SIZE 512
#define TRUE 1
#define PATH_SIZE 512
#define ENDOFFFILE 0

char line[LINE_SIZE];

pthread_mutex_t thread_m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thCondVar = PTHREAD_COND_INITIALIZER;
int arrived=0;
char ip[255];
int port;

struct threadStruct{
    pthread_t** t;
    int count;
    int capacity;
}threadStrs;

struct threadStruct *threads;
int requestCapacity=255;
int requestSize=0;
struct reqWithParam **requests;

void resizeRequests();
int readFileAndCreateRequests(char *file);
struct reqWithParam* createReq(char *city,char *type,struct date d1,struct date d2);
void clearRequests();
void sigint_handler(int signal_number);
void parse_arg(int argc, char* argv[],int *port,char *ip,char *reqFile);
void *requestHandlerThread(void *arg);
void printError(char *message);
void printTerminal(char *message);

int main(int argc, char* argv[]){
    char requestFile[255];
    int i,error;
    requestFile[0]='\0';
    ip[0]='\0';
    requests=(struct reqWithParam**)malloc(sizeof(struct reqWithParam*)*requestCapacity);

    struct sigaction sigint_action;
    /*Set SIGINT handler for CTRL+C.*/
    memset(&sigint_action,0,sizeof(sigint_action));
    sigint_action.sa_handler = &sigint_handler;
    if(sigaction(SIGINT,&sigint_action,NULL)==ERROR){
        sprintf(line,"\nError while sigaction operation.\n");                
        printError(line);
        exit(EXIT_FAILURE);
    }

    parse_arg(argc,argv,&port,ip,requestFile);

    readFileAndCreateRequests(requestFile);
    
    sprintf(line,"I have loaded %d requests and I’m creating %d threads.\n",requestSize,requestSize);
    printTerminal(line);

    threads=(struct threadStruct *)malloc(sizeof(struct threadStruct)*1);
    threads->t = (pthread_t**)malloc(sizeof(pthread_t*)*requestSize);
    threads->capacity=256;
    threads->count=requestSize;

    for(i=0;i<requestSize;i++){
        threads->t[i]=(pthread_t*)malloc(sizeof(pthread_t)*1);
        requests[i]->threadId=i;
        error=pthread_create(threads->t[i],NULL,requestHandlerThread,requests[i]);
        if(error!=0){
            sprintf(line,"Thread couldn't create!.\n");
            printError(line);
            exit(EXIT_FAILURE);
        }
    }

    for(i=0;i<requestSize;i++){
        pthread_join(*(threads->t[i]),NULL);
        free(threads->t[i]);
    }
    free(threads->t);
    free(threads);

    line[0]='\0';
    sprintf(line,"All threads have terminated, goodbye.\n");
    printTerminal(line);
    clearRequests();
    return 0;

}

void resizeRequests(){
    int i;
    struct reqWithParam **temp=(struct reqWithParam **)malloc(sizeof(struct reqWithParam *)*(requestSize*2));

    for(i=0;i<requestSize;i++){
        temp[i]=requests[i];
    }
    free(requests);
    requests=temp;
    requestSize=requestSize*2;
}

int readFileAndCreateRequests(char *file){
    int fd,error;
    char buffer,count;
    char type[512],city[512];
    struct date d1,d2;
    struct reqWithParam *temp;
    type[0]='\0';
    city[0]='\0';

    fd=open(file,O_RDONLY);
	if(fd==ERROR){
        sprintf(line,"file couldn't open.\n");                
        printError(line);
        return FAIL;
    }

    error=read(fd,&buffer,1);
    if(error==ERROR){
        sprintf (line,"Failed to read file.\n");
        printError(line);
        return FAIL;
    }

    if(error==0){
        sprintf(line,"File is empty. Please provide file with data!!\n");    
        printError(line);
        return FAIL;
    }

    count = 0;
    int dateCount=0;
    d1.day=-1;
    d1.month=-1;
    d1.year=-1;
    d2.day=-1;
    d2.month=-1;
    d2.year=-1;
    while(TRUE){
        error=read(fd,&buffer,1);
        if(error==ERROR){
            sprintf(line, "Failed to read file.\n");
            printError(line);
            return FAIL;
        }
        if(error==ENDOFFFILE){
            break;
        }
        if(buffer=='\n'){
            if(type[0]!='\0'){
                if(line[0]!='\0'){
                    if(count==2 || count==3){
                        if(d1.year==-1){
                            d1.year=atoi(line);
                        }else{
                            d2.year=atoi(line);
                        }
                        line[0]='\0'; 
                        dateCount=0;
                    }else{
                        strcpy(city,line);
                        line[0]='\0';
                    }
                }
                /*create and add request*/
                temp=createReq(city,type,d1,d2);
                if(requestSize>=requestCapacity){
                    resizeRequests();
                }
                requests[requestSize++]=temp;
                /*reset variables*/
                type[0]='\0';
                city[0]='\0';
                count=0;
                dateCount=0;
                d1.day=-1;
                d1.month=-1;
                d1.year=-1;
                d2.day=-1;
                d2.month=-1;
                d2.year=-1;
            }
        }else if(buffer==' '){
            if(count==1){
                strcpy(type,line);
                line[0]='\0';
            }
            else if(count==2 || count==3){
                if(d1.year==-1){
                    d1.year=atoi(line);
                }else{
                    d2.year=atoi(line);
                }
                line[0]='\0'; 
                dateCount=0;
            }else if(count==4){
                strcpy(city,line);
                line[0]='\0';
            }else{
                line[0]='\0';
            }
            count++;
        }else if(buffer=='-'){
            if(dateCount==0){
                if(d1.day==-1){
                    d1.day=atoi(line);
                }else{
                    d2.day=atoi(line);
                }
            }else if(dateCount==1){
                if(d1.month==-1){
                    d1.month=atoi(line);
                }else{
                    d2.month=atoi(line);
                }
            }
            dateCount++;
            line[0]='\0'; 
        }
        else{
            strncat(line,&buffer,1);           
        }
    }
}

struct reqWithParam* createReq(char *city,char *type,struct date d1,struct date d2){
    struct reqWithParam *temp = (struct reqWithParam*)malloc(sizeof(struct reqWithParam)*1);
    temp->city[0]='\0';
    temp->type[0]='\0';

    strcpy(temp->city,city);
    strcpy(temp->type,type);
    temp->d1.day=d1.day;
    temp->d1.month=d1.month;
    temp->d1.year=d1.year;
    temp->d2.day=d2.day;
    temp->d2.month=d2.month;
    temp->d2.year=d2.year;

    return temp;
}

void clearRequests(){
    int i=0;
    for(int i=0;i<requestSize;i++){
        free(requests[i]);
    }
    free(requests);
}


/*SIGINT Handler.*/
void sigint_handler(int signal_number){


}


/*Parses command line arg with using getopt.*/
void parse_arg(int argc, char* argv[],int *port,char *ip,char *reqFile){
	int opt;
    int flag=0;
    int error=0;
    char *ch;
    char line[LINE_SIZE];
	while((opt = getopt(argc, argv, "r:q:s:"))!=ERROR){
		switch(opt){
			case 'r':
				strcpy(reqFile,optarg);
				break;
			case 'q':
                (*port)=atoi(optarg);
				break;
            case 's':
                strcpy(ip,optarg);
				break;
			default:
                sprintf(line,"\nPlease provide proper command line arguement!\n");
                printError(line);
                sprintf(line,"./server -p PORT -t numberOfThreads");
                printError(line);
                free(requests);
				exit(EXIT_FAILURE);
		}
	}
	if(argc!=7){
        sprintf(line,"\nPlease provide proper command line arguement!\n");
        printError(line);
        sprintf(line,"./client -r requestFile -q PORT -s IP");                
        printError(line);
        free(requests);
		exit(EXIT_FAILURE);
    }
}

void *requestHandlerThread(void *arg){
    int error=0;
    int serverfd;
    struct reqWithParam *myArg=((struct reqWithParam*)arg);
    struct sockaddr_in addr;
    int result;
    char buffer='\0';
    char threadLine[2048];
    threadLine[0]='\0';

    sprintf(threadLine,"Client-Thread-%d: thread %d has been created\n",myArg->threadId,myArg->threadId);
    printTerminal(threadLine);

    error=pthread_mutex_lock(&thread_m);
    if(error!=0){
        sprintf(threadLine,"Failed while locking mutex.\n");
        printError(threadLine);
        exit(EXIT_FAILURE);
    }
    ++arrived;
    if(arrived < threads->count){
        error=pthread_cond_wait(&thCondVar,&thread_m);
        if(error!=0){
            sprintf (threadLine, "Failed while waiting conditional variable.\n");
            printError(threadLine);
            exit(EXIT_FAILURE);
        }
    }else{
        error=pthread_cond_broadcast(&thCondVar);
        if(error!=0){
            sprintf(threadLine, "Failed while broadcast conditional variable.\n");
            printError(threadLine);
            exit(EXIT_FAILURE);
        }
    }

    error=pthread_mutex_unlock(&thread_m);
    if(error!=0){
        sprintf(threadLine,"Failed while unlocking mutex.\n");
        printError(threadLine);
        exit(EXIT_FAILURE);
    }

    sprintf(threadLine,"Client-Thread-%d: I am requesting \"%s %d-%d-%d %d-%d-%d %s\"\n",myArg->threadId,
        myArg->type,myArg->d1.day,myArg->d1.month,myArg->d1.year,
        myArg->d2.day,myArg->d2.month,myArg->d2.year,myArg->city);
    printTerminal(threadLine);

   /*Open socket.*/
    if((serverfd=socket(PF_INET, SOCK_STREAM,0))==ERROR){
        sprintf(line,"error while socket operation!\n");
        printError(line);
        exit(EXIT_FAILURE);
    }

    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
        
    if (inet_pton(AF_INET,ip,&(addr.sin_addr))!=1) {
        sprintf(threadLine,"Invalid IP address\n");
        printError(threadLine);
        exit(EXIT_FAILURE);
    }

    if(error=connect(serverfd,(struct sockaddr*)(&addr),sizeof(addr))==ERROR){
        close(serverfd);
        sprintf(threadLine,"connect error:%s,%d!\n",strerror(errno),errno);  
        printError(threadLine);
        return NULL;
    }

    threadLine[0]='\0';
    if(myArg->city[0]!='\0'){
        sprintf(threadLine,"c %s %d-%d-%d %d-%d-%d %s *",myArg->type,myArg->d1.day,myArg->d1.month,myArg->d1.year,
            myArg->d2.day,myArg->d2.month,myArg->d2.year,myArg->city);
    }else{
        sprintf(threadLine,"c %s %d-%d-%d %d-%d-%d *",myArg->type,myArg->d1.day,myArg->d1.month,myArg->d1.year,
            myArg->d2.day,myArg->d2.month,myArg->d2.year);
    }

    error=write(serverfd,threadLine,strlen(threadLine));
    if(error==ERROR){
        sprintf(threadLine,"Error while write operation.!");
        printError(threadLine);
    }

    /*wait for response*/
    threadLine[0]='\0';
    while((error=read(serverfd,&buffer,1))!=0){
        if(error==ERROR){
            sprintf(threadLine,"Error while read operation.\n");
            printError(threadLine);
            exit(EXIT_FAILURE);
        }
        if(buffer=='*')break;
        strncat(threadLine,&buffer,1);
    }
    result=atoi(threadLine);

    sprintf(threadLine,"Client-Thread-%d: Servers respond to \"%s %d-%d-%d %d-%d-%d %s\" is result:%d \n",
        myArg->threadId,myArg->type,myArg->d1.day,myArg->d1.month,myArg->d1.year,
        myArg->d2.day,myArg->d2.month,myArg->d2.year,myArg->city,result);
    printTerminal(threadLine);

    if(close(serverfd)<0){
        sprintf(threadLine,"socket close error\n");
        printError(threadLine);
        exit(EXIT_FAILURE);
    }

    line[0]='\0';
    sprintf(threadLine,"Client-Thread-%d: Terminating\n",myArg->threadId);
    printTerminal(threadLine);

}

void printError(char *message){
    char errorText[512];
    errorText[0]='\0';
    sprintf(errorText,"%s %s",message,strerror(errno));
    write(STDOUT_FILENO, errorText,strlen(errorText));
}

void printTerminal(char *message){
    write(STDOUT_FILENO, message,strlen(message));
}















