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

void *receiver(void *args)
{
	key_t key = ftok(".", 12);
	int msgid = msgget(key, IPC_CREAT);

	printf("msgid: %d\n", msgid);

	if(msgid < 0)
	{
		printf("msgget error!\n");
		return 0;
	}
	int count = 0;
	while(1)
	{
		struct msgbuf msg;
		msgrcv(msgid, &msg, TEXT_SIZE, 3, 0);
		if(strcmp(msg.mtext, "end1") == 0)
		{
			printf("receiver received \"%s\"\n", msg.mtext);
			msg.mtype = 1;
			strcpy(msg.mtext, "over1");
			msgsnd(msgid, &msg, TEXT_SIZE, 0);
			sleep(1);
			count++;
			printf("--------sender1 exits!---------\n");
		}
		else if(strcmp(msg.mtext, "end2") == 0)
		{
			printf("receiver received \"%s\"\n", msg.mtext);
			msg.mtype = 2;
			strcpy(msg.mtext, "over2");
			msgsnd(msgid, &msg, TEXT_SIZE, 0);
			sleep(1);
			count++;
			printf("--------sender2 exits!---------\n");
		}
		else printf("receiver received \"%s\"\n", msg.mtext);

		if(count >= 2)
		{
			printf("--------receiver exits!---------\n");
			msgctl(msgid, IPC_RMID, NULL);
			pthread_exit((void*)"receiver exit!\n");
		}
	}

}

int main()
{
	pthread_create(&p, NULL, receiver, NULL);

	pthread_join(p, NULL);
}