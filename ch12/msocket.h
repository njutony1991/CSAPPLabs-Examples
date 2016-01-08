#ifndef MSocket_H
#define MSocket_H

#include <sys/socket.h>

#define LISTENQ 1024
typedef struct sockaddr SA;

int open_clientfd(char *hostname,int port);
int open_listenfd(int port);
#endif