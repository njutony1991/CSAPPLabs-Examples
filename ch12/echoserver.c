#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "rio.h"
#include "msocket.h"

#define MAXLINE 1024

void echo(int connfd);

int main(int argc,char **argv){
	int listenfd,connfd,port,clientlen;
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char *haddrp;
	if(argc!=2){
		fprintf(stderr,"usage: %s <port>\n",argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);
	
	if((listenfd = open_listenfd(port))<0){
		fprintf(stderr, "open_listenfd (%d) error \n",port);
		exit(0);
	}

	while(1){
		clientlen = sizeof(clientaddr);
		connfd = accept(listenfd,(SA *)&clientaddr,&clientlen);
		if(connfd < 0){
			fprintf(stderr, "accept error \n");
			exit(0);
		}
		hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
						sizeof(clientaddr.sin_addr.s_addr),AF_INET);
		if(hp==NULL){
			fprintf(stderr, "gethostbyaddr error \n");
			exit(0);
		}
		haddrp = inet_ntoa(clientaddr.sin_addr);
		printf("Server connected to %s (%s) \n",hp->h_name,haddrp);
		echo(connfd);
		close(connfd);
	}	
	exit(0);
}


void echo(int connfd){
	size_t n;
	char buf[MAXLINE];
	rio_t rio;
	rio_readinitb(&rio,connfd);
	while((n=rio_readlineb(&rio,buf,MAXLINE))!=0){
		printf("server received %d bytes\n",n);
		rio_writen(connfd,buf,n);
	}
}