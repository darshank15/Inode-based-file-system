#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <bits/stdc++.h>
using namespace std;

#ifndef _DISK_H_
#define _DISK_H_

/******************************************************************************/

// total disk size = 512MB
// total no of blocks = 512M/4096 = 131072

#define DISK_BLOCKS 131072        /* number of blocks on the disk      */
#define BLOCK_SIZE 4096           /* block size on "disk"              */
#define NO_OF_INODES 78644        /* In 60% possible number of inodes. */
#define NO_OF_FILE_DESCRIPTORS 32 /* this is predefined                */

// #define DISK_BLOCKS 100           /* number of blocks on the disk     */
// #define BLOCK_SIZE 32             /* block size on "disk"             */
// #define NO_OF_INODES 10           /* In 60% possible number of inodes.*/
// #define NO_OF_FILE_DESCRIPTORS 16 /* this is predefined */
/******************************************************************************/

struct inode // total size = 52Byte
{
    int filesize;
    int pointer[12]; /* 10 direct pointers, 1 single indirect, 1 double indirect pointer */
};

struct file_to_inode_mapping // total size = 36Byte
{
    char file_name[30]; // fileName
    int inode_num;      // inode number
};

struct super_block /* super block structure */
{
    /*  Total size of struct superblock is 6*4(int) + 78644 +131072(bool array) =  209740 Byte
    No Of Blocks = 209740/4096 = 52 blocks */
    int no_of_blocks_used_by_superblock = ceil(((float)sizeof(super_block)) / BLOCK_SIZE);

    /* Total size required for mapping between filename and inode is 36*78644/4096 = 692  */
    int no_of_blocks_used_by_file_inode_mapping = ceil(((float)sizeof(struct file_to_inode_mapping) * NO_OF_INODES) / BLOCK_SIZE);

    /* It denotes the position from where inode starts. 744 */
    int starting_index_of_inodes = no_of_blocks_used_by_superblock + no_of_blocks_used_by_file_inode_mapping;

    /*  Total size required to store all inodes = 78644*52 = 999   */
    int no_of_blocks_used_to_store_inodes = ceil(((float)(NO_OF_INODES * sizeof(struct inode))) / BLOCK_SIZE); // 8192

    /* 52 + 692 + 999 = 1743 (reserved blocks) */
    int starting_index_of_data_blocks = no_of_blocks_used_by_superblock + no_of_blocks_used_by_file_inode_mapping + no_of_blocks_used_to_store_inodes;

    /* 131072 - 1743 = 129329 (free blocks to store data) */
    int total_no_of_available_blocks = DISK_BLOCKS - starting_index_of_data_blocks;

    bool inode_freelist[NO_OF_INODES];    // to check which inode no is free to assign to file
    bool datablock_freelist[DISK_BLOCKS]; //to check which data block is free to allocate to file
};


/******************************************************************************/

/******************************************************************************/
// Data Structure to be used.

extern char disk_name[50], filename[30];
extern struct super_block sb;
extern struct file_to_inode_mapping file_inode_mapping_arr[NO_OF_INODES];
extern struct inode inode_arr[NO_OF_INODES];

extern FILE *diskptr;
extern int openfile_count;                           // keeps track of number of files opened.
extern map<string, int> file_to_inode_map;           // filename->inode file name as key maps to inode (value)
extern map<int, string> inode_to_file_map;           // indoe-> filename inode to file mapping

extern vector<int> free_inode_vector;                // denote free inodes
extern vector<int> free_data_block_vector;           // denote free data blocks

extern vector<int> free_filedescriptor_vector;       // denote free filedescriptor.
extern map<int, pair<int, int>> file_descriptor_map; // fd->(inode,file_seek_ptr) Stores files Descriptor as key and corresponding Inode number(First) and file pointer.
extern map<int, int> file_descriptor_mode_map;       // fd->mode [ 0:read, 1:write, 2: append ]


/******************************************************************************/

/******************************************************************************/
// Methods to be implemented.

int create_disk(char *name);            /* create an empty, virtual disk file          */
int mount_disk(char *name);             /* open a virtual disk (file)                  */
int unmount_disk();                     /* close a previously opened disk (file)       */
int open_file(char *name);              /* open file to get its file descriptor        */
int close_file(int fd);                 /* close the file                              */
int block_read(int block, char *buf);   /* read block */
int block_write(int block, char *buf, 
        int size, int start_position);  /* Write block */
int create_file(char *name);            /* to create file */
int delete_file(char *name);            /* to delete file */
int write_into_file(int fd, int mode);  /* to write content into file */
int read_file(int fd);                  /* to read content from file */
int erase_inode_content(int cur_inode); /* to erase inode content */
/******************************************************************************/

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define BOLD "\033[1m"
#define DEFAULT "\033[0m"

#endif