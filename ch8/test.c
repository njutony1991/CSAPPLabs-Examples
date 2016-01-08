#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(){
	char str[100] = "Hello\n";
	printf("%d\n",strlen(str));
	sleep(10);
	return 0;
}