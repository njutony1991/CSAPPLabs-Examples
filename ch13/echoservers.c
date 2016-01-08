#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "msocket.h"
#include "rio.h"

#define MAXLINE 1024

typedef struct
{
	int maxfd;
	fd_set read_set;
	fd_set ready_set;
	int nready;
	int maxi;
	int clientfd[FD_SETSIZE];
	rio_t clientrio[FD_SETSIZE];	
}pool;

int byte_cnt = 0;

void init_pool(int listenfd,pool *p);
void add_client(int connfd,pool *p);
void check_clients(pool *p);
int main(int argc,char **argv){
	int port,connfd,listenfd,clientlen = sizeof(struct sockaddr_in);
	struct sockaddr_in clientaddr;
	static pool pool;
	if(argc!=2){
		fprintf(stderr, "usage: %s <port>\n",argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);
	listenfd = open_listenfd(port);
	init_pool(listenfd,&pool);
	while(1){
		pool.ready_set = pool.read_set;
		pool.nready = select(pool.maxfd+1,&pool.ready_set,NULL,NULL,NULL);

		if(FD_ISSET(listenfd,&pool.ready_set)){
			connfd = accept(listenfd,(SA*)&clientaddr,&clientlen);
			add_client(connfd,&pool);
		}
		check_clients(&pool);
	}
	return 0;
}

void init_pool(int listenfd,pool *p){
	int i;
	p->maxi = -1;
	for(i=0;i<FD_SETSIZE;i++)
		p->clientfd[i] = -1;

	p->maxfd = listenfd;
	FD_ZERO(&p->read_set);
	FD_SET(listenfd,&p->read_set);
}

void add_client(int connfd,pool *p){
	int i;
	p->nready++;
	for(i=0;i<FD_SETSIZE;i++)
		if(p->clientfd[i]<0){
			p->clientfd[i] = connfd;
			rio_readinitb(&p->clientrio[i],connfd);
			FD_SET(connfd,&p->read_set);

			if(connfd > p->maxfd)
				p->maxfd = connfd;
			if(i > p->maxi)
				p->maxi = i;
			break;
		}
		if(i==FD_SETSIZE)
			fprintf(stderr, "add_client error: Too many clients\n");
}

void check_clients(pool *p){
	int i=0,connfd,n;
	rio_t rio;
	char buf[MAXLINE];

	for(i=0;(i<=p->maxi) && (p->nready>0);i++){
		 connfd = p->clientfd[i];
		 rio = p->clientrio[i];

		 if(connfd>0 && (FD_ISSET(connfd,&p->ready_set))){
		 	p->nready--;
		 	if((n=rio_readlineb(&rio,buf,MAXLINE))!=0){
		 		byte_cnt += n;
		 		printf("Server received %d (%d total) bytes on fd %d\n",n,byte_cnt,connfd);
		 		rio_writen(connfd,buf,n);
		 	}
		 	else{
		 		close(connfd);
		 		FD_CLR(connfd,&p->read_set);
		 		p->clientfd[i] = -1;
		 	}
		 }
	}
}