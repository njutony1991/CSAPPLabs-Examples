#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include "msocket.h"
#include "rio.h"

#define MAXLINE 1024

int main(int argc,char **argv){
	int clientfd , port;
	char *host, buf[MAXLINE];
	rio_t rio;
	if(argc != 3){
		fprintf(stderr,"usage: %s <host <port>\n",argv[0]);
		exit(0);
	}
	host = argv[1];
	port = atoi(argv[2]);
	if((clientfd=open_clientfd(host,port))<0){
		fprintf(stderr, "open_clientfd (%s,%d) error\n",host,port);
		exit(0);
	}
	rio_readinitb(&rio,clientfd);
	while(fgets(buf,MAXLINE,stdin)!=NULL){
		rio_writen(clientfd,buf,strlen(buf));
		rio_readlineb(&rio,buf,MAXLINE);
		if(fputs(buf,stdout)==EOF)
			fprintf(stderr,"fputs error\n");
	}
	if(close(clientfd)<0)
		fprintf(stderr, "close error\n");
	exit(0);
}