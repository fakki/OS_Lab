#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

#define TEXT_SIZE 1024

struct msgbuf
{
	long mtype;
	char mtext[TEXT_SIZE];
};

pthread_t p;

void *sender(void *args)
{
	key_t key = ftok(".", 12);
	int msgid = msgget(key, IPC_CREAT);

	printf("msgid: %d\n", msgid);

	if(msgid < 0)
	{
		printf("msgget error!\n");
		return 0;
	}

	printf("--------sender2---------\n");

	while(1)
	{
		char input[TEXT_SIZE];
		printf("please input something to sender2: \n");
		//scanf("%s", input);
		gets(input);
		struct msgbuf msg, msg_receive;
		msg.mtype = 3;
		if(strcmp(input, "exit") == 0)
		{
			strcpy(msg.mtext, "end2");
			int s = msgsnd(msgid, &msg, TEXT_SIZE, 0);
			int r = msgrcv(msgid, &msg_receive, TEXT_SIZE, 2, 0);
			printf("sender2 received \"%s\"\n", msg_receive.mtext);
			printf("Goodbye!\n");
			pthread_exit((void*)"sender2 exit!\n");
		}
		else 
		{
			strcpy(msg.mtext, input);
			int s = msgsnd(msgid, &msg, TEXT_SIZE, 0);
		}
	}

}

int main()
{
	pthread_create(&p, NULL, sender, NULL);

	pthread_join(p, NULL);
}