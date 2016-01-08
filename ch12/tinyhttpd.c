#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rio.h"
#include "msocket.h"

#define MAXLINE 1024
#define MAXBUF 8192

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri,char *filename,char *cgiargs);
void serve_static(int fd, char *filename,int filesize);
void get_filetype(char *filename,char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd,char *cause,char *errnum,char *shortmsg,char *longmsg);

int main(int argc,char **argv)
{
	int listenfd,connfd,port,clientlen;
	struct sockaddr_in clientaddr;

	if(argc==2){
		fprintf(stderr,"usage: %s <port>\n",argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);

	listenfd = open_listenfd(port);
	while(1){
		clientlen = sizeof(clientaddr);
		connfd = accept(listenfd,(SA *)&clientaddr,clientlen);
		doit(connfd);
		close(connfd);
	}
	return 0;
}

void clienterror(int fd,char *cause,char *errnum,char *shortmsg,char *longmsg){
	char buf[MAXLINE],body[MAXLINE];
	sprintf(body,"<html><title>Tiny Error</title>");
	sprintf(body,"%s<body bgcolor=""ffffff"">\r\n",body);
	sprintf(body,"%s%s: %s\r\n",body,errnum,shortmsg);
	sprintf(body,"%s<p>%s: %s\r\n",body,longmsg,cause);
	sprintf(body,"%s<hr><em>The Tiny Web Server</em>\r\n",body);

	sprintf(buf,"HTTP/1.0 %s %s\r\n",errnum,shortmsg);
	rio_writen(fd,buf,strlen(buf));
	sprintf(buf,"Content-type:text/html\r\n");
	rio_writen(fd,buf,strlen(buf));
	sprintf(buf,"Content-length: %d\r\n",strlen(body));
	rio_writen(fd,buf,strlen(buf));
	rio_writen(fd,body,strlen(body));		
}

void doit(int connfd){
	rio_t rio;
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
	char filename[MAXLINE],cgiargs[MAXLINE];

	rio_readinitb(&rio, connfd);
	rio_readlineb(&rio,buf,strlen(buf));
	sscanf(buf,"%s %s %s",method,uri,version);
	if(strcasecmp(method,"GET")){
		clienterror(fd,method,"501","Not Implemented","Tiny does not implement this method");
		return;
	}
	read_requesthdrs(&rio);

	is_static = parse_uri(uri,filename,cgiargs);
	if(stat(filename,&sbuf)<0){
		clienterror(fd,filename,"404","Not found","Tiny couldn't ");
		return;
	}

	if(is_static){
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
			clienterror(fd,filename,"403","Forbidden","Tiny couldn't read the file");
			return;
		}
		serve_static(fd,filename,sbuf.st_size);
	}else{
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
			clienterror(fd,filename,"403","Forbidden","Tiny couldn't run the CGI program");
			return;
		}
		serve_dynamic(fd,filename,cgiargs);
	}
}

void read_requesthdrs(rio_t *rp){
	char buf[MAXLINE];
	rio_readlineb(rp,buf,MAXLINE);
	while(strcmp(buf,"\r\n")){
		rio_readlineb(rp,buf,MAXLINE);
	}
	return;
}

int parse_uri(char *uri,char *filename,char *cgiargs){
	char *ptr;
	if(!strstr(uri,"cgi-bin")){
		strcpy(cgiargs,"");
		strcpy(filename,".");
		strcat(filename,uri);
		if(uri[strlen(uri)-1] =='/')
			strcat(filename,"home.html");
		return 1;
	}
	else{
		ptr = index(uri,'?');
		if(ptr){
			strcpy(cgiargs,ptr+1);
			*ptr = '\0';
		}else
			strcpy(cgiargs,"");
		strcpy(filename,".");
		strcat(filename,uri);
		return 0;
	}
}

void serve_static(int fd,char *filename,int filesize){
	
}

void serve_dynamic(int fd,char *filename,char *cgiargs){

}