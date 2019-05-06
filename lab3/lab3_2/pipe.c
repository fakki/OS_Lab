#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define TEXT_SIZE 1024


int main()
{
	char buf[TEXT_SIZE];
	int fd[2], child, byte_write = 0, byte_read = 0;
	assert(pipe(fd) != -1);

	sem_t *write_ok = sem_open("write_ok", O_CREAT | O_RDWR, 0666, 0);
	sem_t *next_test = sem_open("next_test", O_CREAT | O_RDWR, 0666, 0);

	child = fork();
	assert(child != -1);
	if(child == 0)
	{
		close(fd[0]);

		int flag = fcntl(fd[1], F_GETFL);
		fcntl(fd[1], F_SETFL, flag|O_NONBLOCK);

		while(write(fd[1], "A", 1) != -1) 
		{
			byte_write++;
		}
		printf("test1: content of pipe is %dKB.\n", byte_write/1024);
		sem_post(write_ok);
		exit(0);
	}

	child = fork();
	assert(child != -1);
	if(child == 0)
	{
		sem_wait(next_test);
		close(fd[0]);
		strcpy(buf, "This is the second son.");
		write(fd[1], buf, strlen(buf));
		printf("p2: write %dB\n", (int)strlen(buf));
		sem_post(write_ok);
		exit(0);
	}

	child = fork();
	assert(child != -1);
	if(child == 0)
	{
		sem_wait(next_test);
		close(fd[0]);
		strcpy(buf, "This is the third son.");
		write(fd[1], buf, strlen(buf));
		printf("p3: write %dB\n", (int)strlen(buf));
		sem_post(write_ok);
		exit(0);
	}


	//clear pipe
	sem_wait(write_ok);
	close(fd[1]);
	byte_write = 64*1024;
	while(byte_write--) read(fd[0], buf, 1);
	sem_post(next_test);

	sem_wait(write_ok);
	close(fd[1]);
	byte_read = read(fd[0], buf, strlen("This is the second son."));
	printf("test2: try to read %dB and read %dB.\n", (int)strlen("This is the second son."), byte_read);
	sem_post(next_test);

	sem_wait(write_ok);
	close(fd[1]);
	byte_read = read(fd[0], buf, 200);
	printf("test3: try to read %dB and read %dB.\n", 200, byte_read);
	sem_post(next_test);


	sem_close(write_ok);
	sem_close(next_test);
	sem_unlink("write_ok");
	sem_unlink("next_test");

	return 0;
}
