#include <csapp.h>

int main(){
	int fd1,fd2;
	char c;
	fd1 = Open("foo.txt",O_RDONLY,0);
	fd2 = Open("foo.txt",O_RDONLY,0);
	Read(fd1,&c,1);
	Read(fd2,&c,1);
	printf("%c\n",c);
	exit(0);
}