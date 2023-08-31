#ifndef SOCKET_MANAGER_H
#define SOCKET_MANAGER_H

#include <sys/socket.h>
#include <arpa/inet.h>



int findAvailablePortForServantSocketAndListen(int *servantFd,struct sockaddr_in *addr,int portIndex);
void resetStructAndIncreasePort(struct sockaddr_in *addr,int *port);
int sendServantPortToServer(int serverPort,int servantPort,char *ip,int lowerRange,int upperRange,
                            char **manDirList,int manDirListSize);
struct reqWithParam* readRequest(int *conn_fd);
void print_error(char *message);
#endif