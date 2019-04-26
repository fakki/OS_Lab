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

fcb *getfcbposition(fcb *fcbcursor, int *dirno, int *diroff, char *filename)
{
	*dirno =  fcbcursor->first, *diroff = 1;
	int plen = *dirno, i;
	for(i = 1; i <= plen; i++, fcbcursor++, (*diroff)++)
	{
		if(strcmp(fcbcursor->filename, filename) == 0)
			break;
		else if(i == plen)
		{
			printf("no such file.\n");
			return -1;
		}

		if(i%16 == 0)
		{
			*diroff = 0;
			*dirno = nextblkno(*dirno);
			fcbcursor = (fcb *)(myvhead + (*dirno-1)*BLOCKSIZE);
		}
	}

	return fcbcursor;
}

fcb *getfcb(int fd)
{
	return (fcb *)(myvhead + BLOCKSIZE*(openfilelist[fd].ffcb.first-1));
}

char * makedirpath(char *dirname, char *filename)
{
	strcat(dirname, "/");
	strcat(dirname, filename);
}

char *gettime()
{
    time_t timep;
    time(&timep);
    return ctime(&timep);
}

int nextblkno(int blkno)
{
	fat *fat1 = (fat *)(myvhead + BLOCKSIZE) + (blkno-1);
	return fat1->id;
}

//blkno is the block which need a free block to add behind
int addfreeblock(int blkno)
{
	fat *fat1 = (fat *)(myvhead + BLOCKSIZE), *fat2 = (fat *)(myvhead + 3*BLOCKSIZE);

	for(int i = 1; i <= SIZE/BLOCKSIZE; i++, fat1++, fat2++)
	{
		if(fat1->id == FREE)
		{
			fat1->id = END;
			fat2->id = END;
			if(blkno != 0)
			{
				fat1 = (fat *)(myvhead + BLOCKSIZE) + (blkno-1);
				fat2 = (fat *)(myvhead + 3*BLOCKSIZE) + (blkno-1);
				fat1->id = i;
				fat2->id = i;
			}
			return i;
		}
	}

	return -1;
}

void my_format()
{
	printf("\nwaiting to initialized fs...\n");
	int i;
	myvhead = (unsigned char *)malloc(SIZE);
	startp = myvhead + 5*BLOCKSIZE;
	//initialize block0 here...
	//...
	char *magicnum = "10101010";
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
	fcbroot->length = 1;   //?
	fcbroot->free = 1;

	//. and .. directory
	fcb *onep = (fcb *)malloc(sizeof(fcb)), *twop = (fcb *)malloc(sizeof(fcb));
	memcpy(onep, fcbroot, sizeof(fcb));
	memcpy(twop, fcbroot, sizeof(fcb));
	strcpy(onep->filename, ".");
	strcpy(twop->filename, "..");
	memcpy(startp+sizeof(fcb), onep, sizeof(fcb));
	memcpy(startp+2*sizeof(fcb), twop, sizeof(fcb));
	fcbroot->length += 2;
	printf("%d\n", fcbroot->length);
	memcpy(startp, fcbroot, sizeof(fcb));

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
	strcpy(openfilelist[0].dir, "/root");

	free(fcbroot);
	fclose(file);
}

int do_read(int fd, int len, char *text)
{
    //char *texttmp = text;
	int read_byte = 0;
	useropen file = openfilelist[fd];
	int blocknum = file.ffcb.first;
	unsigned char *off, *buf = (unsigned char *)malloc(1024), *start;
	assert(buf);

	fat *fat1 = (fat *)(myvhead + BLOCKSIZE);
	fat *blockpos = fat1 + (blocknum-1);

	int blkno = file.count/1024;
	for(int i = 1; i <= blkno; i++)
	{
		blocknum = blockpos->id;
		blockpos = fat1 + (blocknum-1);
		if(blockpos->id == END)
		{
			printf("In do_read: count reach out of range.\n");
			return -1;
		}
	}

	while(len > 0)
	{
		start = myvhead + (blocknum-1)*BLOCKSIZE;
		blockpos = fat1 + (blocknum-1);
		off = start + file.count%1024;

		memcpy(buf, start, 1024);

		if(len <= start+1024-off)
		{
			read_byte += len;
			memcpy(text, off, len);
			file.count += len;
			text += len;
			len = 0;
		}
		else
		{
            int temp = start+1024-off;
			read_byte += temp;
			len -= temp;
			memcpy(text, off, temp);
			file.count += temp;
			text += temp;
		}
		if(len > 0)
		{
			assert(blockpos->id != END);
			blocknum = blockpos->id;
		}
	}

	free(buf);

	return read_byte;
}

int do_write(int fd, char *text, int len, char wstyle)
{
	unsigned char *buf = (unsigned char*)malloc(1024), *start, *off;
	assert(buf);

	int blocknum = openfilelist[fd].ffcb.first;

	fat *fat1 = (fat *)(myvhead + BLOCKSIZE);
	fat *blockpos = fat1 + (blocknum-1);

	int blkno = openfilelist[fd].count/1024;
	for(int i = 1; i <= blkno; i++)
	{
		if(blockpos->id == END)
		{
			if((blocknum = addfreeblock(blockpos-fat1+1)) == -1)
			{
				printf("In do_write: no more block to allocate.\n");
				return -1;
			}
		}
	}

	int write_byte = 0;

	while(len > 0)
	{
		int tmplen = 0;
		start = myvhead + (blocknum-1)*BLOCKSIZE;
		blockpos = fat1 + (blocknum-1);
		off = buf + openfilelist[fd].count%1024;

		if(wstyle == 2 || start != off)
			memcpy(buf, start, 1024);
		else memset(buf, 0, 1024);

		if(len <= start+1024-off)
		{
			tmplen = len;
			write_byte += len;
			memcpy(off, text, len);
			memcpy(start, buf, BLOCKSIZE);
			text += len;
			len=0;
		}
		else
		{
            int tmplen = start+1024-off;
			write_byte += tmplen;
			len -= tmplen;
			memcpy(off, text, tmplen);
			memcpy(start, buf, BLOCKSIZE);
			text += tmplen;
		}

		openfilelist[fd].count += tmplen;
		if(len > 0 && blockpos->id == END)
		{
			if((blocknum = addfreeblock(blockpos-fat1+1)) == -1)
			{
				printf("In do_write: no more block to allocate.\n");
				return -1;
			}
		}
	}

	free(buf);
	return write_byte;
}


void my_mkdir(char *dirname)
{
	int i, newdir, temp;
	fat *fat1 = (fat *)(startp + BLOCKSIZE);
	int len = openfilelist[curdir].ffcb.length * sizeof(fcb);
	char *fcbs = (char *)malloc(len);
	fcb *fcbcursor;
	fcb *fcbnew = (fcb *)malloc(sizeof(fcb));
	assert(fcbs);

	assert(do_read(curdir, len, fcbs) == len);
	for(i = 1, fcbcursor = (fcb *)fcbs; i <= len/sizeof(fcb); i++, fcbcursor++)
	{
        //printf("%s\n", fcbcursor->filename);
        //printf("%s\n", dirname);
        //printf("%d %d\n", strcmp(dirname, fcbcursor->filename), fcbcursor->attribute == 0);
		if(fcbcursor->attribute == 0 && strcmp(dirname, fcbcursor->filename) == 0)
		{
			printf("directory %s has already exist!\n", dirname);
			return;
		}
	}

	for(i = 0; i < MAXOPENFILE; i++)
	{
		if(openfilelist[i].topenfile == 0)
		{
			openfilelist[i].topenfile = 1;
			openfilelist[i].fcbstate = 0;
			openfilelist[i].count = 0;
			strcpy(openfilelist[i].dir, openfilelist[curdir].dir);
			strcat(openfilelist[i].dir, "/");
			strcat(openfilelist[i].dir, dirname);
			newdir = i;
			break;
		}
		else if(i == MAXOPENFILE-1)
		{
			printf("In function my_mkdir(): can't open more file.\n");
			free(fcbs);
			return;
		}
	}

	if((temp = addfreeblock(0)) == -1)
	{
		printf("In my_mkdir(): no more block to allocate.\n");
		openfilelist[newdir].topenfile = 0;
		return;
	}
	else fcbnew->first = temp;

	int blkno = openfilelist[curdir].ffcb.first, diroff;
	fcbcursor = (fcb *)(myvhead + (blkno-1)*BLOCKSIZE);
	for(i = 1, diroff = 1; i <= len/sizeof(fcb); fcbcursor++, i++, diroff++)
	{
		if(fcbcursor->free == 0)
			break;

		if(i%16 == 0)
		{
			diroff = 0;
			blkno = nextblkno(blkno);
			if(blkno == END)
				blkno = addfreeblock(blkno);
			fcbcursor = (fcb *)(myvhead + (blkno-1)*BLOCKSIZE);
		}

	}

	openfilelist[curdir].ffcb.length++;
    openfilelist[curdir].fcbstate = 1;

    openfilelist[newdir].dirno = blkno;
    openfilelist[newdir].diroff = diroff;

	strcpy(fcbnew->exname, "dir");
	strcpy(fcbnew->filename, dirname);
	fcbnew->attribute = 0;
	strcpy(fcbnew->time, gettime());
	fcbnew->length = 1;
	fcbnew->free = 1;

	openfilelist[newdir].ffcb = *fcbnew;

	fcb *onep = (fcb *)malloc(sizeof(fcb)), *twop = (fcb *)malloc(sizeof(fcb));

	memcpy(onep, fcbnew, sizeof(fcb));
	memcpy(twop, &openfilelist[curdir].ffcb, sizeof(fcb));

	strcpy(onep->filename, ".");
	strcpy(twop->filename, "..");

	fcbnew->length += 2;

	openfilelist[newdir].ffcb = *fcbnew;
	memcpy(fcbcursor, fcbnew, sizeof(fcb));

	fcbcursor = (fcb *)(myvhead + (openfilelist[newdir].ffcb.first-1)*BLOCKSIZE);
	memcpy(fcbcursor++, fcbnew, sizeof(fcb));
	memcpy(fcbcursor++, onep, sizeof(fcb));
	memcpy(fcbcursor, twop, sizeof(fcb));

	free(onep); free(twop); free(fcbnew); free(fcbs);
	fcbcursor = (fcb *)startp;
	fcbcursor->length++;


	my_close(newdir);

}

void my_exitsys()
{
	file = fopen("/home/hondor/Desktop/filesys1.txt", "w");
	fwrite(myvhead, SIZE, 1, file);
	fclose(file);
	exit(0);
}

int my_open(char *filename)
{
	int i, plen = openfilelist[curdir].ffcb.length;

	if(strcmp(filename, ".") == 0) return curdir;
	if(strcmp(filename, "..") == 0) 
	{
		if(openfilelist[curdir].ffcb.first == 6) return 1;
    	else 
    	{
    		fcb *twop = getfcb(curdir) + 2;
    		my_close(curdir);
    		openfilelist[curdir].ffcb = *((fcb *)(myvhead + (twop->first-1)*BLOCKSIZE));
    		twop = getfcb(curdir) + 2;
    		getfcbposition((fcb *)(myvhead + (twop->first-1)*BLOCKSIZE), &openfilelist[curdir].dirno, &openfilelist[curdir].diroff, openfilelist[curdir].ffcb.filename);
    		openfilelist[curdir].count =0;
    		openfilelist[curdir].fcbstate = 0;
    		i = strlen(openfilelist[curdir].dir);
    		while(openfilelist[curdir].dir[i] != '/') i--;
    		openfilelist[curdir].dir[i] = 0;
    		return curdir;
    	}
	}

	for(i = 0;i < MAXOPENFILE; i++)
	{
		if(openfilelist[i].topenfile == 1 && strcmp(filename, openfilelist[i].ffcb.filename) == 0)
		{
			return i;
		}
	}

	int dirno, diroff;
	fcb *fcbcursor = getfcbposition(&(openfilelist[curdir].ffcb), &dirno, &diroff, filename);

	int fd = -1;
	for(i =0; i < MAXOPENFILE; i++)
	{
		if(openfilelist[i].topenfile == 0)
		{
			fd = i;
			break;
		}
		else if(i == MAXOPENFILE-1)
		{
			printf("can't open more files.\n");
			return -1;
		}
	}

	openfilelist[fd].count = 0;
	openfilelist[fd].fcbstate = 0;
	openfilelist[fd].dirno = dirno;
	openfilelist[fd].diroff = diroff;
	openfilelist[fd].ffcb = *fcbcursor;
	strcpy(openfilelist[fd].dir, makedirpath(openfilelist[curdir].dir, filename));

	return fd;
}

int my_close(int fd)
{
	//if(curdir == fd)

	if(fd >= MAXOPENFILE)
	{
		printf("file description invalid.\n");
		return -1;
	}

	if(openfilelist[fd].fcbstate == 1)
	{
		openfilelist[fd].count = 0;
		assert(do_write(fd, (char *)(&openfilelist[fd].ffcb), sizeof(fcb), 2) == sizeof(fcb));
	}

	openfilelist[fd].topenfile = 0;
	return fd;
}

void my_cd(char *dirname)
{
    int newdir = my_open(dirname);
    if(newdir != curdir) my_close(curdir);
    curdir = newdir;
}


void my_ls()
{
	int i, len = openfilelist[curdir].ffcb.length;
	char *fcbs = (char *)malloc(len*sizeof(fcb));
    do_read(curdir, len*sizeof(fcb), fcbs);

	fcb *fcbcursor = (fcb *)fcbs;
	fcbs++;
	for(i = 1; i <= len; i++, fcbcursor++)
	{
		if(strcmp(fcbcursor->filename, ".") == 0 || strcmp(fcbcursor->filename, "..") == 0 || strcmp(fcbcursor->filename, openfilelist[curdir].ffcb.filename) == 0)
			continue;
		printf("%s\n", fcbcursor->filename);
	}
}

void my_rmdir(char *dirname)
{
	int i, plen = openfilelist[curdir].ffcb.length;
	char *fcbs = (char *)malloc(sizeof(fcb)*plen);
	assert(do_read(curdir, plen*sizeof(fcb), fcbs) == plen*sizeof(fcb));
	fcb *fcbcursor = (fcb *)fcbs;
	for(i = 1; i <= plen; i++, fcbcursor++)
	{
		if(fcbcursor->attribute == 0 && strcmp(fcbcursor->filename, dirname) == 0)
			break;
		else if(i == plen)
		{
			printf("no such directory.\n");
			return;
		}
	}

	if(fcbcursor->length > 3)
	{
		printf("this directory is not empty.\n");
		return;
	}

	for(i = 0; i < MAXOPENFILE; i++)
	{
		if(openfilelist[i].topenfile == 1 && strcpy(openfilelist[i].ffcb.filename, dirname) == 0)
		{
			my_close(i);
			break;
		}
	}

	fcbcursor->free = 0;
	assert(do_write(curdir, fcbs, plen*sizeof(fcb), 2) == plen*sizeof(fcb));

	openfilelist[curdir].ffcb.length++;
	openfilelist[curdir].fcbstate = 1;

}

/*fcb *searchdir(char *dirname)
{
	fcb *fcbcursor = getfcb(curdir) + 3;
	int i, len = fcbcursor->length;
	char *filename;
	strcpy(filename, dirname);
	i = 0;
	if(dirname++)
	while(1)
	{
		if(dirname[i] == '/')
		{
			filename[i] == 0;
			dirname++;
			break;
		}
		dirname++;
		i++;
	}

	for(i = 4; i <= length; i++, fcbcursor++)
	{
		if(fcbcursor->attribute == 0 && strcmp(fcbcursor->filename, filename))
			break;
		else if(i == length)
		{
			printf("no such directory.\n");
			return 0;
		}
	}
}*/

int main()
{
    startsys();

    char order[20], filename[20], dirname[20];

    printf("%s> ", openfilelist[curdir].dir);

    while(1)
    {
    	scanf("%s", order);
    	if(strcmp(order, "ls") == 0) my_ls();
    	else if(strcmp(order, "cd") == 0)
    	{
    		scanf("%s", dirname);
    		my_cd(dirname);
    	}
    	else if(strcmp(order, "mkdir") == 0)
    	{
    		scanf("%s", dirname);
    		my_mkdir(dirname);
    	}
    	else if(strcmp(order, "rmdir") == 0)
    	{
    		scanf("%s", dirname);
    		my_rmdir(dirname);
    	}
    	else if(strcmp(order, "exit") == 0)
    	{
    		my_exitsys();
    	}

    	printf("%s> ", openfilelist[curdir].dir);
    }
    //my_exitsys();

    return 0;
}
