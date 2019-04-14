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

/*void *mem = (void *)malloc(SIZE);
    my_format(mem);
    block0 *block = (block0 *)malloc(sizeof(block0));
    memcpy(block, mem, sizeof(block0));
    printf("%s %d %d\n", block->info, block->root, block->startblock);
    fcb *fcb1 = (fcb *)malloc(sizeof(fcb));
    memcpy(fcb1, (void *)(mem + 5*BLOCKSIZE), sizeof(fcb));
    printf("%s %s %s %d %d\n", fcb1->filename, fcb1->exname, fcb1->time, fcb1->length, fcb1->free);
    free(block);*/

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
    char dir[80];
    int count;
    char fcbstate;  //1: modified. 0: not.
    char topenfile; //

}useropen;

typedef struct BLOCK0{
    char info[200]; //store some discription info, block size, block num and so on
    unsigned short root; //root dir start index of blocks
    unsigned char *startblock; //virtual disk start position
}block0;

unsigned char *myvhead; //virtual disk start address
useropen *openfilelist[MAXOPENFILE]; //open file list array
int curdir; //current directory disc character fd
char currentdir[80]; //current directory name
unsigned char *startp; //v d data area start position

int exist(fcb *cursor, char *str, int fod)
{
    int onetime = 1;
    int blocknum = cursor->first;
    int flag = -1;
    fat *fat1 = (fat *)(myvhead+BLOCKSIZE+(blocknum-1)*sizeof(fat));
    do{
        if(onetime < 1)
        {
            blocknum = fat1->id;
            fat1 = (fat *)(myvhead+BLOCKSIZE+(blocknum-1)*sizeof(fat));
            onetime--;
        }
        fcb *fcb1 = (fcb *)(startp+(blocknum-1)*BLOCKSIZE);
        //root
        if(blocknum == 1)
            fcb1 = fcb1+1;
        while(fcb1)
        {
            if(fcb1->attribute == fod && strcmp(fcb1->filename, str) == 0)
            {
                flag = 1;
                return fcb1->first;
            }
            fcb1 = fcb1+1;
        }
    }
    while(fat1->id != END);

    return flag;
}

int addopenfl(fcb ffcb, char dir[80], int topenfile)
{
    int res;

    useropen *uo = (useropen *)malloc(sizeof(useropen));
    uo->ffcb = ffcb;
    uo->count = -1;
    strcpy(uo->dir, dir);
    uo->fcbstate = 0;
    uo->topenfile = topenfile;

    for(res = 0; res < MAXOPENFILE; res++)
    {
        if(openfilelist[res] == NULL)
        {
            memcpy(openfilelist[res], uo, sizeof(useropen));
            break;
        }
    }

    if(res >= MAXOPENFILE)
    {
        printf("Can't open more files!\n");
        exit(0);
    }

    return res;
}

int findfblock()
{
    int first, i;
    fat *fat1 = (fat *)malloc(sizeof(fat)), *fat2 = (fat *)malloc(sizeof(fat));
    fat1 = (fat *)(myvhead + BLOCKSIZE);
    fat2 = (fat *)(myvhead + 3*BLOCKSIZE);
    for(i = 0; i < 1000; i++, fat1++, fat2++)
    {
        if(fat1->id == 0)
        {
            first = fat1->id;
            break;
        }
    }

    if(i >= 1000)
    {
        printf("No more memory!\n");
        exit(0);
    }

    fat1->id = END;
    fat2->id = END;
    free(fat1);
    free(fat2);

    return first;
}

char *gettime()
{
    time_t timep;
    time(&timep);
    return ctime(&timep);
}

void my_format(void *mem)
{
    //init BLOCK0 1 block
    block0 *block0_start = (block0 *)mem;
    block0 *block0_data = (block0 *)malloc(sizeof(block0));
    block0_data->info[0] = 0;
    block0_data->root = 6;
    block0_data->startblock = (unsigned char *)(mem + 5*BLOCKSIZE);
    memcpy(block0_start, block0_data, sizeof(block0));
    free(block0_data);

    //init FAT1
    fat *fat1_start = (fat *)(mem + BLOCKSIZE);
    fat *fat1_data = (fat *)malloc(sizeof(fat));
    fat1_data->id = FREE;
    for(int i = 0; i < 1000; i++, fat1_start++)
        memcpy(fat1_start, fat1_data, sizeof(fat));
    free(fat1_data);

    //init FAT2
    fat *fat2_start = (fat *)(mem + 3*BLOCKSIZE);
    fat *fat2_data = (fat *)malloc(sizeof(fat));
    fat2_data->id = FREE;
    for(int i = 0; i < 1000; i++, fat2_start++)
        memcpy(fat2_start, fat2_data, sizeof(fat));
    free(fat2_data);

    //init root directory
    fcb *root_dir = (fcb *)(mem + 5*BLOCKSIZE);
    fcb *root_dir_data = (fcb *)malloc(sizeof(fcb));
    root_dir_data->attribute = 0; //directory file
    strcpy(root_dir_data->filename, "root");
    strcpy(root_dir_data->exname, "");
    strcpy(root_dir_data->time, gettime());
    root_dir_data->length = 0; //directory file
    root_dir_data->free = 0;
    //find disk block
    root_dir_data->first = findfblock();
    memcpy(root_dir, root_dir_data, sizeof(fcb));
    free(root_dir_data);
}

void * startsys(FILE *file)
{
    void *mem = (void *)malloc(SIZE);
    fread(mem, SIZE, 1, file);
    return mem;
}

void my_exitsys(void *mem, FILE *file)
{
    fwrite(mem, SIZE, 1, file);
}

char **splitdir(char *dirname)
{
    char **res = (char **)malloc(10*sizeof(char *));
    for(int i = 0; i < 10; i++)
        res[i] = (char *)malloc(sizeof(char)*100);
    int curr = 0;
    char str[80];
    strcpy(str, dirname+1);
    char *token = strtok(str, "/");
    while(token != NULL)
    {
        strcpy(res[curr++], token);
        printf("%s\n", token);
        token = strtok(NULL, "/");
    }
    return res;

}

void my_cd(char *dirname)
{
    //directory "/root" as head
    void *cursor;
    char **dir = splitdir(dirname);
    bool flag = false;
    if(strcmp(dir[0], "root") == 0)
    {
        //from root
        int index = 1;
        cursor = (void *)startp + sizeof(fcb);
        while(dir[index])
        {
            int next_blocknum;
            if(next_blocknum = exist(cursor, dir[index], 0) == -1)
                break;
            else {
                index++;
                cursor = (void *)startp + (next_blocknum-1)*BLOCKSIZE;
            }
        }
        if(dir[index] == NULL)
            flag = true;
        else
        {
            printf("No such directory!\n");
            return;
        }
        if(flag) printf("%s>>", dirname);
    }
    else
    {
        //from curr directory
        int index = 1;
        int blocknum = openfilelist[curdir]->ffcb.first;

        cursor = (void *)startp + sizeof(fcb);
        while(dir[index])
        {
            int next_blocknum;
            if(next_blocknum = exist(cursor, dir[index], 0) == -1)
                break;
            else {
                index++;
                cursor = (void *)startp + (next_blocknum-1)*BLOCKSIZE;
            }
        }
        if(dir[index] == NULL)
            flag = true;
        else
        {
            printf("No such directory!\n");
            return;
        }
        if(flag) printf("%s>>", dirname);
    }
}

void my_mkdir(char *dirname)
{
    //find current directory's block num
    int blocknum = openfilelist[curdir]->ffcb.first;
    fat *fat1 = (fat *)(myvhead + BLOCKSIZE);
    fat1 += blocknum-1;
    //not renamed
    if(exist(&(openfilelist[curdir]->ffcb), dirname, 0) == -1)
    {
        _mkdir()
    }
}

void my_ls()
{
    int onetime = 1;
    int blocknum = openfilelist[curdir]->ffcb.first;
    fat *fat1 = (fat *)(myvhead+BLOCKSIZE+(blocknum-1)*sizeof(fat));
    do{
        if(!onetime)
            blocknum = fat1->id;
        fcb *fcb1 = (fcb *)(startp+(blocknum-1)*BLOCKSIZE);
        //root
        if(blocknum == 1)
            fcb1 = fcb1+1;
        while(fcb1)
        {
            if(fcb1->attribute == 0)
                printf("%s\n", fcb1->filename);
            else printf("%s.%s\n", fcb1->filename, fcb1->exname);
            fcb1++;
        }
    }
    while(fat1->id != END);
}

int main()
{
    FILE *file;
    file = fopen("/home/hondor/Desktop/filesys.txt", "r");
    void *mem = startsys(file);
    fclose(file);

    //root directory add to openfilelist
    for(int i = 0; i < MAXOPENFILE; i++)
        openfilelist[i] = NULL;

    myvhead = (unsigned char *)mem;
    startp = (unsigned char *)(mem + 5*BLOCKSIZE);
    strcpy(currentdir, "/root");
    fcb *root_fcb = (fcb *)malloc(sizeof(fcb));
    memcpy(root_fcb, startp, sizeof(fcb));
    curdir = addopenfl(*root_fcb, currentdir, 0);

    file = fopen("/home/hondor/Desktop/filesys.txt", "w");
    my_exitsys(mem, file);
    fclose(file);

    free(mem);
    return 0;
}
