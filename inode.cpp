#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <bits/stdc++.h>
#include "inode.h"
using namespace std;

/******************************************************************************/
static int active = 0; /* is the virtual disk open (active) */
static int handle;

char disk_name[50], filename[30];
struct super_block sb;
struct file_to_inode_mapping file_inode_mapping_arr[NO_OF_INODES];
struct inode inode_arr[NO_OF_INODES];

map<string, int> dir_map;                     //file name as key maps to inode (value)
vector<int> free_inode_vector;                // denote free inodes
vector<int> free_data_block_vector;           // denote free data blocks
vector<int> free_filedescriptor_vector;       // denote free filedescriptor.
int openfile_count = 0;                       //keeps track of number of files opened.
map<int, pair<int, int>> file_descriptor_map; //Stores files Descriptor as key and corresponding Inode number(First) and file pointer.

FILE *diskptr;
/******************************************************************************/

int create_disk(char *disk_name)
{
  char buffer[BLOCK_SIZE];

  diskptr = fopen(disk_name, "wb");
  memset(buffer, 0, BLOCK_SIZE); // intialize null buffer

  /* Using buffer to initialize whole disk as NULL  */
  for (int i = 0; i < DISK_BLOCKS; ++i)
    fwrite(buffer, 1, BLOCK_SIZE, diskptr);

  struct super_block sb; //initializing sb

  /* Marking all the blocks other than data blocks as used(super_block,mapping,inodes) */
  for (int i = 0; i < sb.starting_index_of_data_blocks; ++i)
    sb.datablock_freelist[i] = true; // true means reserved

  /* Marking all data bloks as free */
  for (int i = sb.starting_index_of_data_blocks; i < DISK_BLOCKS; ++i)
    sb.datablock_freelist[i] = false; // false means free

  /* marking all inodes as free */
  for (int i = 0; i < NO_OF_INODES; ++i)
    sb.inode_freelist[i] = false;

  for (int i = 0; i < NO_OF_INODES; ++i)
  {
    /* setting all pointers of inode as free */
    for (int j = 0; j < 13; j++)
    {
      inode_arr[i].pointer[j] = -1;
    }
  }

  /* storing superblock into starting of the file */
  fseek(diskptr, 0, SEEK_SET);
  char sb_buff[sizeof(struct super_block)];
  memset(sb_buff, 0, sizeof(struct super_block));
  memcpy(sb_buff, &sb, sizeof(sb));
  fwrite(sb_buff, sizeof(char), sizeof(sb), diskptr);

  /* storing file_inode_mapping after super block */
  fseek(diskptr, (sb.no_of_blocks_used_by_superblock) * BLOCK_SIZE, SEEK_SET);
  int len = sizeof(file_inode_mapping_arr);
  char dir_buff[len];
  memset(dir_buff, 0, len);
  memcpy(dir_buff, file_inode_mapping_arr, len);
  fwrite(dir_buff, sizeof(char), len, diskptr);

  /* storing inodes after file_inode mapping */
  fseek(diskptr, (sb.starting_index_of_inodes) * BLOCK_SIZE, SEEK_SET);
  int len = sizeof(inode_arr);
  char inode_buff[len];
  memset(inode_buff, 0, len);
  memcpy(inode_buff, inode_arr, len);
  fwrite(inode_buff, sizeof(char), len, diskptr);

  fclose(diskptr);
  cout << "\nVirtual Disk Created!!!" << endl;

  return 1;
}

int mount_disk(char *name)
{
  diskptr = fopen(disk_name,"rb+");

	/* retrieve super block from virtual disk and store into global struct super_block sb */
	char sb_buff[sizeof(sb)];
	memset(sb_buff,0,sizeof(sb));
	fread(sb_buff,sizeof(char),sizeof(sb),diskptr);
	memcpy(&sb,sb_buff,sizeof(sb));

	/* retrieve file_inode mapping array from virtual disk and store into global
  struct dir_entry file_inode_mapping_arr[NO_OF_INODES] */
	fseek( diskptr, (sb.no_of_blocks_used_by_superblock)*BLOCK_SIZE, SEEK_SET );
  int len = sizeof(file_inode_mapping_arr);
	char dir_buff[len];
	memset(dir_buff,0,len);
	fread(dir_buff,sizeof(char),len,diskptr);
	memcpy(file_inode_mapping_arr,dir_buff,len);

	/* retrieve Inode block from virtual disk and store into global struct inode 
  inode_arr[NO_OF_INODES] */
	fseek( diskptr, (sb.starting_index_of_inodes)*BLOCK_SIZE, SEEK_SET );
  int len = sizeof(inode_arr);
	char inode_buff[len];
	memset(inode_buff,0,len);
	fread(inode_buff,sizeof(char),len,diskptr);
	memcpy(inode_arr,inode_buff,len);

	/* storing all filenames into map */
	for (int i = NO_OF_INODES - 1; i >= 0; --i)
		if(sb.inode_freelist[i]==1)
			dir_map[string(file_inode_mapping_arr[i].file_name)]=file_inode_mapping_arr[i].inode_num;
		else
			free_inode_vector.push_back(i);

	/* maintain free data block vector */
	for(int i = DISK_BLOCKS - 1; i>=sb.starting_index_of_data_blocks; --i)
		if(sb.datablock_freelist[i] == 0)
			free_data_block_vector.push_back(i);

	/* maintain free filedescriptor vector */
	for(int i=NO_OF_FILE_DESCRIPTORS-1;i>=0;i--){
		free_filedescriptor_vector.push_back(i);
	}

  cout<<"Disk is mounted!!!"<<endl;

	return 1;
}

int unmount_disk()
{
  if (!active)
  {
    fprintf(stderr, "close_disk: no open disk\n");
    return -1;
  }

  close(handle);

  active = handle = 0;

  return 0;
}

int block_write(int block, char *buf)
{
  if (!active)
  {
    fprintf(stderr, "block_write: disk not active\n");
    return -1;
  }

  if ((block < 0) || (block >= DISK_BLOCKS))
  {
    fprintf(stderr, "block_write: block index out of bounds\n");
    return -1;
  }

  if (lseek(handle, block * BLOCK_SIZE, SEEK_SET) < 0)
  {
    perror("block_write: failed to lseek");
    return -1;
  }

  if (write(handle, buf, BLOCK_SIZE) < 0)
  {
    perror("block_write: failed to write");
    return -1;
  }

  return 0;
}

int block_read(int block, char *buf)
{
  if (!active)
  {
    fprintf(stderr, "block_read: disk not active\n");
    return -1;
  }

  if ((block < 0) || (block >= DISK_BLOCKS))
  {
    fprintf(stderr, "block_read: block index out of bounds\n");
    return -1;
  }

  if (lseek(handle, block * BLOCK_SIZE, SEEK_SET) < 0)
  {
    perror("block_read: failed to lseek");
    return -1;
  }

  if (read(handle, buf, BLOCK_SIZE) < 0)
  {
    perror("block_read: failed to read");
    return -1;
  }

  return 0;
}
int main()
{
  string s;
  cout << "Enter disk name";
  cin >> s;
  char *ab = new char[s.length() + 1];
  strcpy(ab, s.c_str());
  create_disk(ab);
  mount_disk(ab);
  int x;
  while (1)
  {
    char *xx = "aqw";

    cin >> x;
    if (x == 1)
      block_write(0, xx);
  }
  return 0;
}