#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <bits/stdc++.h>
#include "inode.h"
using namespace std;

/******************************************************************************/
static int active = 0;  /* is the virtual disk open (active) */
static int handle;

char disk_name[50],filename[30];
struct super_block sb;
struct file_to_inode_mapping file_inode_mapping_arr[NO_OF_INODES];
struct inode inode_arr[NO_OF_INODES];

map <string,int> dir_map; //file name as key maps to inode (value)
vector<int> free_inode_vector; // denote free inodes
vector<int> free_data_block_vector; // denote free data blocks
vector<int> free_filedescriptor_vector; // denote free filedescriptor.
int openfile_count=0 ; //keeps track of number of files opened.
map <int,pair<int,int> > file_descriptor_map;	//Stores files Descriptor as key and corresponding Inode number(First) and file pointer.


/******************************************************************************/
int create_disk(char *name)
{ 
  int f, cnt;
  char buf[BLOCK_SIZE];

  if (!name) {
    fprintf(stderr, "make_disk: invalid file name\n");
    return -1;
  }

  if ((f = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
    perror("make_disk: cannot open file");
    return -1;
  }

  memset(buf, 0, BLOCK_SIZE);
  for (cnt = 0; cnt < DISK_BLOCKS; ++cnt)
    write(f, buf, BLOCK_SIZE);

  close(f);

  return 0;
}

int mount_disk(char *name)
{
  int f;

  if (!name) {
    fprintf(stderr, "open_disk: invalid file name\n");
    return -1;
  }  
  
  if (active) {
    fprintf(stderr, "open_disk: disk is already open\n");
    return -1;
  }
  
  if ((f = open(name, O_RDWR, 0644)) < 0) {
    perror("open_disk: cannot open file");
    return -1;
  }

  handle = f;
  active = 1;

  return 0;
}

int unmount_disk()
{
  if (!active) {
    fprintf(stderr, "close_disk: no open disk\n");
    return -1;
  }
  
  close(handle);

  active = handle = 0;

  return 0;
}

int block_write(int block, char *buf)
{
  if (!active) {
    fprintf(stderr, "block_write: disk not active\n");
    return -1;
  }

  if ((block < 0) || (block >= DISK_BLOCKS)) {
    fprintf(stderr, "block_write: block index out of bounds\n");
    return -1;
  }

  if (lseek(handle, block * BLOCK_SIZE, SEEK_SET) < 0) {
    perror("block_write: failed to lseek");
    return -1;
  }

  if (write(handle, buf, BLOCK_SIZE) < 0) {
    perror("block_write: failed to write");
    return -1;
  }

  return 0;
}

int block_read(int block, char *buf)
{
  if (!active) {
    fprintf(stderr, "block_read: disk not active\n");
    return -1;
  }

  if ((block < 0) || (block >= DISK_BLOCKS)) {
    fprintf(stderr, "block_read: block index out of bounds\n");
    return -1;
  }

  if (lseek(handle, block * BLOCK_SIZE, SEEK_SET) < 0) {
    perror("block_read: failed to lseek");
    return -1;
  }

  if (read(handle, buf, BLOCK_SIZE) < 0) {
    perror("block_read: failed to read");
    return -1;
  }

  return 0;
}
int main()
{
	string s;
	cout<<"Enter disk name";
	cin>>s;
	char *ab = new char[s.length()+1];
	strcpy(ab,s.c_str());
	create_disk(ab);
	mount_disk(ab);
	int x;
	while(1)
	{
		char* xx="aqw";

		cin>>x;
		if(x==1)
			block_write(0,xx);
	}
	return 0;
}