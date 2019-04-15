#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define BLOCKSIZE 1024
#define SIZE 1024000
#define END 65535
#define FREE 0
#define ROOTBLOCKNUM 2
#define MAXOPENFILE 10

typedef struct FCB{
    char filename[8];       //file name
    char exname[3];         //file extension
    unsigned char attribute;    //0 : directory. 1: data file.
    char time[30]; //create time
    unsigned short first;    //start block number
    unsigned long length;   //file size(Byte)
    char free;  //to label the directory empty or not. 0: empty, 1: not empty;
}fcb;

typedef struct FAT{
    unsigned short id;
}fat;

typedef struct USEROPEN{
    fcb ffcb;
    //dynamic info of the file is below
    int count;
    char free;
    int dirno;
    int diroff;
    char dir[80];
    char fcbstate;  //1: modified. 0: not.
    char topenfile; //

}useropen;

typedef struct BLOCK0{
    char info[200]; //store some discription info, block size, block num and so on
    unsigned short root; //root dir start index of blocks
    unsigned char *startblock; //virtual disk start position
}block0;

unsigned char *myvhead; //virtual disk start address
useropen openfilelist[MAXOPENFILE]; //open file list array
int curdir; //current directory disc character fd
char currentdir[80]; //current directory name
unsigned char *startp; //v d data area start position

FILE *file; // = fopen("/home/hondor/Desktop/filesys1.txt", "w");

char *gettime()
{
    time_t timep;
    time(&timep);
    return ctime(&timep);
}

void my_format()
{
	printf("\nwaiting to initialized fs...\n");
	int i;
	myvhead = (unsigned char *)malloc(SIZE);
	startp = myvhead + 5*BLOCKSIZE;
	//initialize block0 here...
	//...
	unsigned char *magicnum = "10101010";
	memcpy(myvhead, magicnum, 8);

	unsigned char *fat1 = (unsigned char *)(myvhead + BLOCKSIZE), *fat2 = (unsigned char *)(myvhead + 3*BLOCKSIZE);
	fat *fat_init = (fat *)malloc(1000*sizeof(fat));
	fat *cursor = fat_init;
	for(i = 0; i < 5; i++, cursor++)
		cursor->id = END; //?
	for(;i < 1000; i++, cursor++)
		cursor->id = FREE;

	cursor = fat_init + 5;
	cursor->id = END;
	fcb *fcbroot = (fcb *)malloc(sizeof(fcb));
	strcpy(fcbroot->filename, "root");
	strcpy(fcbroot->exname, "dir");
	strcpy(fcbroot->time, gettime());
	fcbroot->first = 6;
	fcbroot->attribute = 0;
	fcbroot->length = 0;   //?
	fcbroot->free = 1;
	memcpy(startp, fcbroot, sizeof(fcb));

	//. and .. directory
	fcb *onep = (fcb *)malloc(sizeof(fcb)), *twop = (fcb *)malloc(sizeof(fcb));
	memcpy(onep, fcbroot, sizeof(fcb));
	memcpy(twop, fcbroot, sizeof(fcb));
	strcpy(onep->filename, ".");
	strcpy(twop->filename, "..");
	memcpy(startp+sizeof(fcb), onep, sizeof(fcb));
	memcpy(startp+2*sizeof(fcb), twop, sizeof(fcb));
	fcbroot->length += 2;

	memcpy(fat1, fat_init, 1000*sizeof(fat));
	memcpy(fat2, fat_init, 1000*sizeof(fat));

	free(onep);
	free(twop);
	free(fcbroot);
	free(fat_init);
	printf("initialized successfully!\n");
}

void startsys()
{
	int i;
	file = fopen("/home/hondor/Desktop/filesys1.txt", "r");
	char magicnum[8];
	fread(magicnum, 8, 1, file);
	fseek(file, 0, SEEK_SET);
	if(strcmp(magicnum, "10101010") != 0)
		my_format();
	else 
	{	
		myvhead = (unsigned char *)malloc(SIZE);
		fread(myvhead, SIZE, 1, file);
	}
	startp = myvhead + 5*BLOCKSIZE;
	//initialize openfilelist
	for(i = 0; i < MAXOPENFILE; i++)
		openfilelist[i].topenfile = 0;
	//open root dir
	fcb *fcbroot = (fcb *)malloc(sizeof(fcb));
	memcpy(fcbroot, startp, sizeof(fcb));
	curdir = 0;
	openfilelist[0].ffcb = *fcbroot;
	openfilelist[0].count = 0; //?
	openfilelist[0].dirno = openfilelist[0].diroff = -1; //?
	openfilelist[0].topenfile = 1;
	openfilelist[0].fcbstate = 0;
	strcpy(openfilelist[0].dir, "/");

	free(fcbroot);
	fclose(file);
}

void my_exitsys()
{
	file = fopen("/home/hondor/Desktop/filesys1.txt", "w");
	fwrite(myvhead, SIZE, 1, file);
	fclose(file);
}

int main()
{
    startsys();

    my_exitsys();
    
    return 0;
}