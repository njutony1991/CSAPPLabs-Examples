#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "rio.h"

ssize_t rio_readn(int fd,void *usrbuf,size_t n){
	size_t nleft = n;
	ssize_t nread;
	char *buf = usrbuf;
	while(nleft>0){
		if((nread = read(fd,buf,nleft))<0){
			if(errno == EINTR)
				nread = 0;
			else
				return -1;
		}else if(nread==0)
			break;
		nleft -= nread;
		buf += nread;
	}
	return (n-nleft);		
}

ssize_t rio_writen(int fd,void *usrbuf,size_t n){
	size_t nleft = n;
	ssize_t nwritten;
	char *buf = usrbuf;
	while(nleft>0){
		if((nwritten = write(fd,buf,nleft))<=0){
			if(errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}
		nleft -= nwritten;
		buf += nwritten;
	}
	return n;
}

void rio_readinitb(rio_t *rp,int fd){
	rp->rio_fd = fd;
	rp->rio_cnt = 0;
	rp->rio_bufptr = rp->rio_buf;
}

static ssize_t rio_read(rio_t *rp,char *usrbuf,size_t n){
	int cnt;
	while(rp->rio_cnt <= 0){
		rp->rio_cnt = read(rp->rio_fd,rp->rio_buf,sizeof(rp->rio_buf));
		if(rp->rio_cnt<0){
			if(errno!=EINTR)
				return -1;
		}else if(rp->rio_cnt==0)
			return 0;
		else
			rp->rio_bufptr = rp->rio_buf;
	}
	cnt = n;
	if(rp->rio_cnt < n)
		cnt = rp->rio_cnt;
	memcpy(usrbuf,rp->rio_bufptr,cnt);
	rp->rio_bufptr += cnt;
	rp->rio_cnt -= cnt;
	return cnt;
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf,size_t maxlen){
	int n,rc;
	char c, *buf = usrbuf;
	for(n=1;n<maxlen;n++){
		if((rc=rio_read(rp,&c,1))==1){
			*buf++ = c;
			if(c=='\n')
				break;
		}else if(rc==0){
			if(n==1)
				return 0;
			else 
				break;
		}else 
			return -1;
	}
	*buf = 0;
	return n;
}


ssize_t rio_readnb(rio_t *rp,void *usrbuf,size_t n){
	size_t nleft = n;
	ssize_t nread;
	char *buf = usrbuf;
	while(nleft > 0){
		if((nread = rio_read(rp,buf,nleft))<0){
			if(errno==EINTR)
				nread = 0;
			else
				return -1;
		}else if(nread==0)
			break;
		nleft -= nread;
		buf += nread;
	}
	return (n-nleft);
}


// int main(){
//     rio_t rio1;
// 	rio_readinitb(&rio1,STDIN_FILENO);
// 	char buf[20];
// 	ssize_t s;
// 	while((s=rio_readnb(&rio1,buf,10))>0){
// 		printf("===>%s",buf);
// 	}		
// 	return 0;
// }