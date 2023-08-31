#include "socketManager.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "commonStructs.h"
#include "requestsQueue.h"

#define SIZE 2048
#define LINE_SIZE 4096
#define ERROR -1
#define SUCCESS 0
#define SOCKET_QUEUE_SIZE 512
#define TRUE 1

char line[LINE_SIZE];

/*Returns portw*/
int findAvailablePortForServantSocketAndListen(int *servantFd,struct sockaddr_in *addr,int portIndex){
    int connectPort;
    int acceptAddrSize=0;
    int conn_fd;
    int error,portTry;
    
    /*SOCKET*/
    conn_fd=socket(AF_INET, SOCK_STREAM, 0);
    if(conn_fd==ERROR){
        sprintf(line,"Error on socket.\n");
        print_error(line);
        exit(EXIT_FAILURE);
    }

    connectPort=33500+portIndex;
    resetStructAndIncreasePort(addr,&connectPort);
    /*Bind created socket.*/
    portTry=bind(conn_fd, (struct sockaddr*) addr, sizeof((*addr)));
    while(portTry == EADDRINUSE){
        resetStructAndIncreasePort(addr,&connectPort);
        portTry=bind(conn_fd, (struct sockaddr*) addr, sizeof((*addr)));
    }
    if(portTry==ERROR){
        sprintf(line,"Error on bind:%s.\n",strerror(errno));
        print_error(line);
        exit(EXIT_FAILURE);
    }

    if(listen(conn_fd,SOCKET_QUEUE_SIZE) < 0){
        sprintf(line,"Error on listen.\n");
        print_error(line);
        exit(EXIT_FAILURE);
    }
    (*servantFd)=dup(conn_fd);

    return connectPort;
}

void resetStructAndIncreasePort(struct sockaddr_in *addr,int *port){
    (*port)++;
    memset(addr, 0, sizeof (*addr)); /*Reset structure.*/
    (*addr).sin_family = AF_INET;
    (*addr).sin_addr.s_addr = htonl(INADDR_ANY);
    (*addr).sin_port = htons((*port));/*Set server port. Using htons provide proper 
                                    byte order.*/
}

int sendServantPortToServer(int serverPort,int servantPort,char *ip,int lowerRange,int upperRange,
                            char **manDirList,int manDirListSize){
    int serverfd=0;
    struct sockaddr_in addr;
    int error=0,i;
    char buffer;

    /*Open socket.*/
    if((serverfd=socket(PF_INET, SOCK_STREAM,0))==ERROR){
        sprintf(line,"error while socket operation!\n");
        print_error(line);
        exit(EXIT_FAILURE);
    }

    addr.sin_family=AF_INET;
    addr.sin_port=htons(serverPort);
        
    if (inet_pton(AF_INET,ip,&(addr.sin_addr))!=1) {
        sprintf(line,"Invalid IP address\n");
        print_error(line);
        exit(EXIT_FAILURE);
    }

    if(error=connect(serverfd,(struct sockaddr*)(&addr),sizeof(addr))==ERROR){
        close(serverfd);
        sprintf(line,"connect error:%s,%d!\n",strerror(errno),errno);
        print_error(line);
        return ERROR;
    }

    sprintf(line,"s %d %d %d %s ",servantPort,lowerRange,upperRange,ip);

    for(i=0;i<manDirListSize;i++){
        strcat(line,manDirList[i]);
        strcat(line," ");
    }
    error=write(serverfd,line,strlen(line));
    if(error==ERROR){
        sprintf(line,"Error while write operation.!");
        print_error(line);
    }

    if(close(serverfd)<0){
        sprintf(line,"socket close error\n");
        print_error(line);
        exit(EXIT_FAILURE);
    }

    return SUCCESS;
}

/*Read and handle client*/
struct reqWithParam* readRequest(int *conn_fd){
    char buffer='\0';
    int error=0;
    int count=0;
    int dateCount=0;
    int len=0;
    struct reqWithParam* r=(struct reqWithParam*)malloc(sizeof(struct reqWithParam)*1);
    r->type[0]='\0';
    r->city[0]='\0';
    r->d1.day=-1;
    r->d1.month=-1;
    r->d1.year=-1;
    r->d2.day=-1;
    r->d2.month=-1;
    r->d2.year=-1;
    char threadLine[SIZE];
    threadLine[0]='\0';

    while((error=read((*conn_fd),&buffer,1))!=0){
        if(error==ERROR){
            sprintf(threadLine,"Failed while locking mutex.\n");
            print_error(threadLine);
            exit(EXIT_FAILURE);
        }
        if(buffer==' '){
            if(r->type[0]=='\0'){
                strcpy(r->type,threadLine);
                threadLine[0]='\0';
            }else{
                if(dateCount==2){
                    if(r->d1.year==-1){
                        r->d1.year=atoi(threadLine);
                        dateCount=0;
                        threadLine[0]='\0';
                    }else{
                        r->d2.year=atoi(threadLine);
                        dateCount++;
                        threadLine[0]='\0';
                    }
                }else{
                    strcpy(r->city,threadLine);
                    threadLine[0]='\0';
                    break;
                }
            }
        }else if(buffer=='-'){
            if(dateCount==0){
                if(r->d1.day==-1){
                    r->d1.day=atoi(threadLine);
                    dateCount++;
                    threadLine[0]='\0';
                }else{
                    r->d2.day=atoi(threadLine);
                    dateCount++;
                    threadLine[0]='\0';
                }
            }else if(dateCount==1){
                if(r->d1.month==-1){
                    r->d1.month=atoi(threadLine);
                    dateCount++;
                    threadLine[0]='\0';
                }else{
                    r->d2.month=atoi(threadLine);
                    dateCount++;
                    threadLine[0]='\0';
                } 
            }
        }else if(buffer=='*'){
            break;
        }
        else{
            strncat(threadLine,&buffer,1);
        }
    }
    
    return r;
}


void print_error(char *message){
    char errorText[512];
    errorText[0]='\0';
    sprintf(errorText,"%s %s",message,strerror(errno));
    write(STDOUT_FILENO, errorText,strlen(errorText));
}