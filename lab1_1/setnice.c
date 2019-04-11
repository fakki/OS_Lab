#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{
	pid_t pid;
	int nicevalue;
	int flag;
	int p = 0, n = 0;
	int * prio = &p;
	int * nice = &n;
	
	
	printf("please enter pid: ");
	scanf("%d", &pid);
	
	printf("please enter flag: \n");
	scanf("%d", &flag);

	printf("please enter value of nice: \n");
	scanf("%d", &nicevalue);

	syscall(333, pid, flag, nicevalue, prio, nice);
	
	printf("%d(pid)'s nice and prio now are %d, %d\n", pid, n, p);
	return 0;
	
}
