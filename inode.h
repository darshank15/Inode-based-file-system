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

#define DISK_BLOCKS 131072        /* number of blocks on the disk                */
#define BLOCK_SIZE 4096           /* block size on "disk"                        */
#define NO_OF_INODES 78644       /* In 60% possible number of inodes. */
#define NO_OF_FILE_DESCRIPTORS 32 /* this is predefined */

/******************************************************************************/
struct inode // total size = 256Byte (Estimated in worst case)
{
    //int inode_num;
    //int type;
    int filesize;
    int pointer[12]; /* 10 direct pointers, 1 single indirect, 1 double indirect pointer */
};
struct file_to_inode_mapping // total size = 34Byte
{
    char file_name[30]; // fileName
    int inode_num;      // inode number
};
struct super_block /* super block structure */
{
    /*  Total size of struct superblock is 5*4(int) + 2*131072(bool array) =  262164Byte
    No Of Blocks = 262164/4096 = 65 blocks */
    int no_of_blocks_used_by_superblock = ceil(((float)sizeof(super_block)) / BLOCK_SIZE); // 65

    /* Total size required for mapping between filename and inode is 36*131072/4096 = 1152  */
    int no_of_blocks_used_by_file_inode_mapping = (sizeof(struct file_to_inode_mapping) * NO_OF_INODES) / BLOCK_SIZE; // 32

    /* It denotes the position from where inode starts. 1217 */
    int starting_index_of_inodes = no_of_blocks_used_by_superblock + no_of_blocks_used_by_file_inode_mapping;

    /*  Total size required to store all inodes = 131072*256 = 8192   */
    int no_of_blocks_used_to_store_inodes = ceil(((float)(NO_OF_INODES * sizeof(struct inode))) / BLOCK_SIZE); // 8192

    /* 8192 + 1088 + 65 = 9345 (reserved blocks) */
    int starting_index_of_data_blocks = no_of_blocks_used_by_superblock + no_of_blocks_used_by_file_inode_mapping + no_of_blocks_used_to_store_inodes;

    /* 131072 - 9345 = 121727 (free blocks to store data) */
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

extern map<string, int> dir_map;                     //file name as key maps to inode (value)
extern vector<int> free_inode_vector;                // denote free inodes
extern vector<int> free_data_block_vector;           // denote free data blocks
extern vector<int> free_filedescriptor_vector;       // denote free filedescriptor.
extern int openfile_count;                       //keeps track of number of files opened.
extern map<int, pair<int, int>> file_descriptor_map; //Stores files Descriptor as key and corresponding Inode number(First) and file pointer.
extern FILE *diskptr;
/******************************************************************************/

/******************************************************************************/
// Methods to be implemented.

int create_disk(char *name); /* create an empty, virtual disk file          */
int mount_disk(char *name);  /* open a virtual disk (file)                  */
int unmount_disk();          /* close a previously opened disk (file)       */
int block_read(int block, char *buf); /*  */
int create_file(char *name); /* to create file */
int delete_file(char *name); /* to delete file */
/******************************************************************************/

#endif