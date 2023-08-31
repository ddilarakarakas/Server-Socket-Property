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
#include "directoryManager.h"
#include "commonStructs.h"
#include "socketManager.h"
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
char line[LINE_SIZE];

struct servantStruct **servants;
int servantCacheSize=256;
int currentServantCount=0;
struct reqQueue *requestQueue;
int requestCount=0;
int nThreads;
int busyThreads;
int sigintFlag=0;
int handledRequests=0;
int serverfd;

pthread_mutex_t req_m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t reqCondVar = PTHREAD_COND_INITIALIZER;

pthread_mutex_t reqCount_m = PTHREAD_MUTEX_INITIALIZER;

pthread_t **threads;

struct reqWithParam;

struct servantStruct{
    int port;
    char *ip;
    int lowerRange;
    int upperRange;
    char **cityNames;
    int cityNamesSize;
    int cityNamesCapacity;
    int servantId;
}servant;

void increaseServantCacheSize(int *servantCacheSize,struct servantStruct ***servants);
struct servantStruct* readServant(int *conn_fd);
void addCityToServant(char* line, struct servantStruct *s);
int getRequestResultFromServant(char *type,struct date d1,struct date d2,char *city,struct servantStruct **servants,
                            int currentServantCount);
int sendServantToRequest(struct servantStruct *servant,char *type,struct date d1,struct date d2,char *city);
void printError(char *message);
void clearAllMemory();

/*SIGINT Handler.*/
void sigint_handler(int signal_number);
void parse_arg(int argc, char* argv[],int *port,int *nThreads);
void handleClientRequest(int *conn_fd);
void *requestHandlerThread(void *arg);
void printTerminal(char *message);

int main(int argc, char* argv[]){
    socklen_t cLen;
    struct sockaddr_in addr;
    struct sockaddr acceptAddr;
    int conn_fd;
    char buffer='\0';
    int error=0,count=0,i;
    int port;
    int connectFD;

    /*For blocking SIGINT, set variables*/
    sigset_t mask;    
    sigemptyset(&mask);
    sigaddset(&mask,SIGINT);
    sigset_t oldmask;
    sigemptyset(&oldmask);

    struct sigaction sigint_action;
    /*Set SIGINT handler for CTRL+C.*/
    memset(&sigint_action,0,sizeof(sigint_action));
    sigint_action.sa_handler = &sigint_handler;
    if(sigaction(SIGINT,&sigint_action,NULL)==ERROR){
        sprintf(line,"\nError while sigaction operation.\n");                
        printError(line);
        exit(EXIT_FAILURE);
    }

    /*Block SIGINT UNTIL thread lock is opened.*/
    error=sigprocmask(SIG_SETMASK,&mask,&oldmask);
    if(error==ERROR){
        sprintf(line,"Error while blocking SIGINT with sigprocmask.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }

    /*parse command line arguments*/
    parse_arg(argc,argv,&port,&nThreads);
    busyThreads=(-1)*nThreads;
    /*create thread pool*/
    threads=(pthread_t**)malloc(sizeof(pthread_t*)*nThreads);
    for(i=0;i<nThreads;i++){
        threads[i]=(pthread_t*)malloc(sizeof(pthread_t)*1);
        error=pthread_create(threads[i],NULL,requestHandlerThread,NULL);
        if(error!=0){
            sprintf(line,"Thread couldn't create!.\n");
            printError(line);
            exit(EXIT_FAILURE);
        }
    }

    /*create request queue*/
    requestQueue = createReqQueue();
    
    /*create servants*/
    servants = (struct servantStruct**)malloc(sizeof(struct servantStruct*)*servantCacheSize);
 
    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    /* OPEN SERVER PORT */
    serverfd=socket(AF_INET, SOCK_STREAM, 0);
    if(serverfd==ERROR){
        sprintf(line,"Error on socket.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }

    /*Bind created socket.*/
    if(bind(serverfd, (struct sockaddr*) &addr, sizeof (addr)) < SUCCESS){
        sprintf(line,"Error on bind:%s.\n",strerror(errno));
        printError(line);
        clearAllMemory();
        exit(EXIT_FAILURE);
    }

    if(listen(serverfd,SOCKET_QUEUE_SIZE) < 0){
        sprintf(line,"Error on listen.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }
    
    /*Critical section ends unblock SIGINT.*/
    error=sigprocmask(SIG_SETMASK,&oldmask,&mask);
    if(error==ERROR){
        sprintf(line,"Error while unblocking SIGINT with sigprocmask.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }

    cLen = sizeof (addr);
    struct req *r=NULL;

    while(TRUE){
        if((conn_fd = accept(serverfd,(struct sockaddr*) &addr,&cLen)) < SUCCESS) {
            close(serverfd);
            printError(line);
            exit(EXIT_FAILURE);
        }
        
        error=pthread_mutex_lock(&req_m);
        if(error!=0){
            sprintf(line,"Failed while locking mutex.\n");
            printError(line);
            exit(EXIT_FAILURE);
        }

        r=createReqArg(&conn_fd);
        reqEnqueue(requestQueue,r);
        requestCount++;
        error=pthread_cond_signal(&(reqCondVar));
        if(error!=0){
            sprintf (line,"Error while signalling conditional variable.\n");
            printError(line);
            exit(EXIT_FAILURE);
        }

        error=pthread_mutex_unlock(&req_m);
        if(error!=0){
            sprintf(line,"Failed while unlocking mutex.\n");
            printError(line);
            exit(EXIT_FAILURE);
        }
    }
}

/*Threads*/
void *requestHandlerThread(void *arg){
    int error;
    char thrLine[SIZE];
    int res=0;
    char buffer;

    while(TRUE){

        error=pthread_mutex_lock(&req_m);
        if(error!=0){
            sprintf(thrLine,"Failed while locking mutex.\n");
            printError(thrLine);
            exit(EXIT_FAILURE);
        }
        
        if(sigintFlag>0){
            pthread_mutex_unlock(&req_m);
            return NULL;
        }

        if(!(requestCount>0)){
            busyThreads--;

            error=pthread_cond_wait(&reqCondVar,&req_m);
            if(error!=0){
                sprintf (thrLine, "Failed while waiting conditional variable.\n");
                printError(thrLine);
                exit(EXIT_FAILURE);
            }

            if(sigintFlag>0){
                pthread_mutex_unlock(&req_m);
                return NULL;
            }
            busyThreads++;
        }

        /*If last thread is done and no request and skip if clause when thread is waiting cond_wait*/
        if(requestCount==0){
            error=pthread_mutex_unlock(&req_m);
            if(error!=0){
                sprintf(thrLine,"Failed while unlocking mutex.\n");
                printError(thrLine);
                exit(EXIT_FAILURE);
            }
            continue;
        }

        struct req *r=reqDequeue(requestQueue);
        requestCount--;

        error=pthread_mutex_unlock(&req_m);
        if(error!=0){
            sprintf(thrLine,"Failed while unlocking mutex.\n");
            printError(thrLine);
            exit(EXIT_FAILURE);
        }

        error=read(r->fd,&buffer,1);
        if(error==ERROR){
            sprintf(thrLine,"Failed while reading conn_fd.\n");
            printError(thrLine);
            exit(EXIT_FAILURE);
        }

        if(buffer=='s'){ /*servant information*/
            servants[currentServantCount] = readServant(&(r->fd));
            sprintf(line,"Servant x present at port %d handling cities %s-%s\n",
                servants[currentServantCount]->port,
                servants[currentServantCount]->cityNames[0],
                servants[currentServantCount]->cityNames[servants[currentServantCount]->cityNamesSize-1]);
            printTerminal(line);

            currentServantCount++;
            if(currentServantCount >= servantCacheSize){
                increaseServantCacheSize(&servantCacheSize,&servants);
            }
        }
        if(buffer=='c'){ /*client request*/
            handleClientRequest(&(r->fd));
        }
        close(r->fd);
        free(r);
    }
}

void handleClientRequest(int *conn_fd){
    int error;
    char buffer='\0';
    int count=0;
    int len=0;
    struct date d1;
    struct date d2;
    struct reqWithParam *r;
    char type[256];
    char city[256];
    int res;
    char thrLine[2048];

    r = readRequest(conn_fd);

    error=pthread_mutex_lock(&reqCount_m);
    if(error!=0){
        sprintf(line,"Failed while locking mutex.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }

    handledRequests++;
    
    error=pthread_mutex_unlock(&reqCount_m);
    if(error!=0){
        sprintf(line,"Failed while unlocking mutex.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }


    thrLine[0]='\0';
    sprintf(thrLine,"Request arrived \"transactionCount %s %d-%d-%d %d-%d-%d %s\" \n",
        r->type,r->d1.day,r->d1.month,r->d1.year,r->d2.day,r->d2.month,r->d2.year,r->city);
    printTerminal(thrLine);

    res=getRequestResultFromServant(r->type,r->d1,r->d2,r->city,servants,currentServantCount);

    thrLine[0]='\0';
    sprintf(thrLine,"Response received: %d, forwarded to client...\n",res);
    printTerminal(thrLine);

    thrLine[0]='\0';
    sprintf(thrLine,"%d*",res);

    error=write((*conn_fd),thrLine,strlen(thrLine));
    if(error==ERROR){
        sprintf(thrLine,"Error while write operation.!");
        printError(thrLine);
    }

    free(r);
}


/*Takes request informations from servants*/
int getRequestResultFromServant(char *type,struct date d1,struct date d2,char *city,struct servantStruct **servants,
                            int currentServantCount){
    int result=0,i,j;
    char threadLine[LINE_SIZE];

    if(city[0]=='\0'){
        for(i=0;i<currentServantCount;i++){
            sprintf(threadLine,"Contacting ALL servants\n");
            printTerminal(threadLine);
            result+=sendServantToRequest(servants[i],type,d1,d2,city);
        }
    }else{
        for(i=0;i<currentServantCount;i++){
            for(j=0;j<servants[i]->cityNamesSize;j++){
                if(strcmp(servants[i]->cityNames[j],city) == EQUAL){
                    sprintf(threadLine,"Contacting servant x\n");
                    printTerminal(threadLine);
                    result+=sendServantToRequest(servants[i],type,d1,d2,city);
                }
            }
        }
    }

    return result;
}

/*Sends request to servant*/
int sendServantToRequest(struct servantStruct *servant,char *type,struct date d1,struct date d2,char *city){
    char threadLine[SIZE];
    int servantfd=0;
    struct sockaddr_in addr;
    int error=0,i;
    char buffer;
    int result=0;

    /*Open socket.*/
    if((servantfd=socket(PF_INET, SOCK_STREAM,0))==ERROR){
        sprintf(threadLine,"error while socket operation!\n");
        printError(threadLine);
        exit(EXIT_FAILURE);
    }

    addr.sin_family=AF_INET;
    addr.sin_port=htons(servant->port);
        
    if(inet_pton(AF_INET,servant->ip,&(addr.sin_addr))!=1) {
        sprintf(threadLine,"Invalid IP address\n");
        printError(threadLine);
        exit(EXIT_FAILURE);
    }

    if((error=connect(servantfd,(struct sockaddr*)(&addr),sizeof(addr)))==ERROR){
        close(servantfd);
        sprintf(threadLine,"connect error:%s,%d\n",strerror(errno),errno);
        printError(threadLine);
        return ERROR;
    }

    if(city[0]=='\0'){
        sprintf(threadLine,"%s %d-%d-%d %d-%d-%d *",type,d1.day,d1.month,d1.year,d2.day,d2.month,d2.year);
    }else{
        sprintf(threadLine,"%s %d-%d-%d %d-%d-%d %s *",type,d1.day,d1.month,d1.year,d2.day,d2.month,d2.year,city);
    }

    error=write(servantfd,threadLine,strlen(threadLine));
    if(error==ERROR){
        sprintf(threadLine,"Error while write operation.!");
        printError(threadLine);
    }

    /*wait for response*/
    threadLine[0]='\0';
    while((error=read(servantfd,&buffer,1))!=0){
        if(error==ERROR){
            sprintf(threadLine,"Error while read operation.\n");
            printError(threadLine);
            exit(EXIT_FAILURE);
        }
        if(buffer=='*')break;
        strncat(threadLine,&buffer,1);
    }

    result=atoi(threadLine);
    if(close(servantfd)<0){
        sprintf(threadLine,"socket close error\n");
        printError(threadLine);
        exit(EXIT_FAILURE);
    }
    return result;
}

void increaseServantCacheSize(int *servantCacheSize,struct servantStruct ***servants){
    char thrLine[512];
    thrLine[0]='\0';
    int i,len;
    
    int error=pthread_mutex_lock(&req_m);
    if(error!=0){
        sprintf(thrLine,"Failed while locking mutex.\n");
        printError(thrLine);
        exit(EXIT_FAILURE);
    }

    len = (*servantCacheSize);
    (*servantCacheSize)=(*servantCacheSize)*2;
    struct servantStruct **temp = (struct servantStruct**)malloc(sizeof(struct servantStruct*)*(*servantCacheSize));

    for(i=0;i<len;i++){
        temp[i]=(*servants)[i];
    }
    free((*servants));
    (*servants)=temp;
    
    error=pthread_mutex_unlock(&req_m);
    if(error!=0){
        sprintf(thrLine,"Failed while locking mutex.\n");
        printError(thrLine);
        exit(EXIT_FAILURE);
    }
}

/*read servant informations in first init.*/
struct servantStruct* readServant(int *conn_fd){
    int error;
    char buffer='\0';
    int count=0;
    int len=0;

    struct servantStruct *s=(struct servantStruct*)malloc(sizeof(struct servantStruct)*1);
    s->cityNamesCapacity=256;
    s->cityNamesSize=0;
    s->cityNames=(char **)malloc(sizeof(char*)*(s->cityNamesCapacity));
    error=read((*conn_fd),&buffer,1);
    if(error==ERROR){
        sprintf(line,"Failed while read operation.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }
    line[0]='\0';

    while((error=read((*conn_fd),&buffer,1))!=0){
        if(error==ERROR){
            sprintf(line,"Failed while read operationn.\n");
            printError(line);
            exit(EXIT_FAILURE);
        }
        if(buffer==' '){
            if(count==0){
                s->port=atoi(line);
            }else if(count==1){
                s->lowerRange=atoi(line);
            }else if(count==2){
                s->upperRange=atoi(line);
            }else if(count==3){
                len=strlen(line);
                (s->ip)=(char *)malloc(sizeof(char)*(len+1));
                strcpy((s->ip),line);
            }else{
                addCityToServant(line,s);
            }
            line[0]='\0';
            count++;
        }else{
            strncat(line,&buffer,1);
        }
    }

    return s;
}

/*adds related cities to servantStruct in first init.
 In this way city filtering can be done in serverside.*/
void addCityToServant(char* line, struct servantStruct *s){
    int len=0,i;
    len=strlen(line);
    (s->cityNames)[s->cityNamesSize]=(char *)malloc(sizeof(char)*(len+1));
    strcpy((s->cityNames)[s->cityNamesSize],line);
    s->cityNamesSize++;

    char **temp;
    if(s->cityNamesSize>=s->cityNamesCapacity){
        temp=(char **)malloc(sizeof(char *)*(s->cityNamesCapacity*2));

        for(i=0;i<s->cityNamesSize;i++){
            temp[i]=s->cityNames[i];
        }
        s->cityNamesSize=s->cityNamesSize*2;
        free(s->cityNames);
        s->cityNames=temp;
    }
}


void printError(char *message){
    char errorText[512];
    errorText[0]='\0';
    sprintf(errorText,"%s %s",message,strerror(errno));
    write(STDOUT_FILENO, errorText,strlen(errorText));
}


void clearAllMemory(){
    int i,j,error;

    for(i=0;i<currentServantCount;i++){
        for(j=0;j<servants[i]->cityNamesSize;j++){
            free(servants[i]->cityNames[j]);
        }
        free(servants[i]->cityNames);
        free(servants[i]->ip);
        free(servants[i]);
    }
    free(servants);

    error=pthread_mutex_lock(&req_m);
    if(error!=0){
        sprintf(line,"Failed while locking mutex.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }
    sigintFlag++;
    pthread_cond_broadcast(&reqCondVar);

    error=pthread_mutex_unlock(&req_m);
    if(error!=0){
        sprintf(line,"Failed while locking mutex.\n");
        printError(line);
        exit(EXIT_FAILURE);
    }

    for(i=0;i<nThreads;i++){
        // pthread_cancel(*(threads[i]));
        pthread_join(*(threads[i]),NULL);

        free(threads[i]);
    }
    free(threads);

    struct reqNode *rFree;

    while(requestQueue->front!=NULL){
        rFree=requestQueue->front->next;

        free(requestQueue->front->reqValue);
        free(requestQueue->front);

        requestQueue->front=rFree;
    }
    free(requestQueue);

}
/*SIGINT Handler.*/
void sigint_handler(int signal_number){
    int error;
    char sigLine[256];
    sigLine[0]='\0';


    
    error=pthread_mutex_lock(&reqCount_m);
    if(error!=0){
        sprintf(sigLine,"Failed while locking mutex.\n");
        printError(sigLine);
        exit(EXIT_FAILURE);
    }

    sprintf(sigLine,"SIGINT has been received. I handled a total of %d client requests. Goodbye\n",handledRequests);
    printTerminal(sigLine);
    
    error=pthread_mutex_unlock(&reqCount_m);
    if(error!=0){
        sprintf(sigLine,"Failed while unlocking mutex.\n");
        printError(sigLine);
        exit(EXIT_FAILURE);
    }

    clearAllMemory();
    close(serverfd);
    exit(EXIT_SUCCESS);
}

/*Parses command line arg with using getopt.*/
void parse_arg(int argc, char* argv[],int *port,int *nTh){
	int opt;
    int flag=0;
    int error=0;
    char line[LINE_SIZE];
	while((opt = getopt(argc, argv, "t:p:"))!=ERROR){
		switch(opt){
			case 'p':
				(*port)=atoi(optarg);
				break;
			case 't':
                (*nTh)=atoi(optarg);
				break;
			default:
                sprintf(line,"\nPlease provide proper command line arguement!\n");
                printError(line);
                sprintf(line,"./server -p PORT -t numberOfThreads\n");
                printError(line);
				exit(EXIT_FAILURE);
		}
	}
	if(argc!=5){
        sprintf(line,"\nPlease provide proper command line arguement!\n");
        printError(line);
        sprintf(line,"./server -p PORT -t numberOfThreads\n");                
        printError(line);
		exit(EXIT_FAILURE);
    }
}


void printTerminal(char *message){
    write(STDOUT_FILENO, message,strlen(message));
}




