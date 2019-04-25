#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>

#define TEXT_SIZE 1024

sem_t sem_sender, sem_receiver;


pthread_t sender_t, receiver_t;

struct msg_block
{
	char msg[TEXT_SIZE];
};

struct msg_block *msg_send;

void *sender(void *args)
{            
	printf("------------------sender: \"please input words to receiver.\"--------------------\n");
	char input[TEXT_SIZE];

	scanf("%s", input);
	strcpy(msg_send->msg, input);
	sem_post(&sem_sender);

	printf("sender is waiting....\n");
	sem_wait(&sem_receiver);
	printf("------------------sender received: \"%s\"--------------------\n", msg_send->msg);

	printf("------------------sender: \"Exit, goodBye!\"--------------------\n");

	pthread_cancel(sender_t);

}

void *receiver(void *args)
{
	printf("receiver is waiting....\n");
	sem_wait(&sem_sender);
	printf("------------------receiver received: \"%s\"--------------------\n", msg_send->msg);

	strcpy(msg_send->msg, "over");
	sem_post(&sem_receiver);

	printf("------------------receiver: \"Exit, goodBye!\"--------------------\n");

	pthread_cancel(receiver_t);
}

int main()
{
	sem_init(&sem_sender, 0, 0);
	sem_init(&sem_receiver, 0, 0);

	int shmid = shmget(ftok("/home", 123671), sizeof(struct msg_block), IPC_CREAT);
	
	if(shmid == -1) perror("shmget");

	void *shm = shmat(shmid, 0, 0);

	if(shm == (void*)-1) perror("shmat");

	msg_send = (struct msg_block *)shm;

	pthread_create(&sender_t, NULL, sender, NULL);
	pthread_create(&receiver_t, NULL, receiver, NULL);

	pthread_join(sender_t, NULL);
	pthread_join(receiver_t, NULL);

	return 0;
}