#include "inode.h"

/******************************************************************************/
static int active = 0; /* is the virtual disk open (active) */

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
map<int, int> file_descriptor_mode_map;       //Stores mode in which file descriptor is used
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
        for (int j = 0; j < 12; j++)
        {
            inode_arr[i].pointer[j] = -1;
        }
    }

    int len;
    /* storing superblock into starting of the file */
    fseek(diskptr, 0, SEEK_SET);
    len = sizeof(struct super_block);
    char sb_buff[len];
    memset(sb_buff, 0, len);
    memcpy(sb_buff, &sb, sizeof(sb));
    fwrite(sb_buff, sizeof(char), sizeof(sb), diskptr);

    /* storing file_inode_mapping after super block */
    fseek(diskptr, (sb.no_of_blocks_used_by_superblock) * BLOCK_SIZE, SEEK_SET);
    len = sizeof(file_inode_mapping_arr);
    char dir_buff[len];
    memset(dir_buff, 0, len);
    memcpy(dir_buff, file_inode_mapping_arr, len);
    fwrite(dir_buff, sizeof(char), len, diskptr);

    /* storing inodes after file_inode mapping */
    fseek(diskptr, (sb.starting_index_of_inodes) * BLOCK_SIZE, SEEK_SET);
    len = sizeof(inode_arr);
    char inode_buff[len];
    memset(inode_buff, 0, len);
    memcpy(inode_buff, inode_arr, len);
    fwrite(inode_buff, sizeof(char), len, diskptr);

    fclose(diskptr);
    cout << "\nVirtual Disk Created!!!" << endl;

    return 1;
}

void printall()
{
    cout << "File-inode-Mapping" << endl;
    for (auto it : file_inode_mapping_arr)
    {
        cout << it.file_name << endl;
        cout << it.inode_num << endl;
        cout << "---------" << endl;
    }
    cout << "Freeinode vector" << endl;
    for (auto it : free_inode_vector)
    {
        cout << it << endl;
        cout << "---------" << endl;
    }
    cout << "File-data-vector" << endl;
    for (auto it : free_data_block_vector)
    {
        cout << it << endl;
        cout << "---------" << endl;
    }
}

int mount_disk(char *name)
{
    diskptr = fopen(disk_name, "rb+");
    if (diskptr == NULL)
    {
        cout << "Disk does not exist :(" << endl;
        return 0;
    }
    int len;
    /* retrieve super block from virtual disk and store into global struct super_block sb */
    char sb_buff[sizeof(sb)];
    memset(sb_buff, 0, sizeof(sb));
    fread(sb_buff, sizeof(char), sizeof(sb), diskptr);
    memcpy(&sb, sb_buff, sizeof(sb));

    /* retrieve file_inode mapping array from virtual disk and store into global
  struct dir_entry file_inode_mapping_arr[NO_OF_INODES] */
    fseek(diskptr, (sb.no_of_blocks_used_by_superblock) * BLOCK_SIZE, SEEK_SET);
    len = sizeof(file_inode_mapping_arr);
    char dir_buff[len];
    memset(dir_buff, 0, len);
    fread(dir_buff, sizeof(char), len, diskptr);
    memcpy(file_inode_mapping_arr, dir_buff, len);

    /* retrieve Inode block from virtual disk and store into global struct inode 
  inode_arr[NO_OF_INODES] */
    fseek(diskptr, (sb.starting_index_of_inodes) * BLOCK_SIZE, SEEK_SET);
    len = sizeof(inode_arr);
    char inode_buff[len];
    memset(inode_buff, 0, len);
    fread(inode_buff, sizeof(char), len, diskptr);
    memcpy(inode_arr, inode_buff, len);

    /* storing all filenames into map */
    for (int i = NO_OF_INODES - 1; i >= 0; --i)
        if (sb.inode_freelist[i] == 1)
            dir_map[string(file_inode_mapping_arr[i].file_name)] = file_inode_mapping_arr[i].inode_num;
        else
            free_inode_vector.push_back(i);

    /* maintain free data block vector */
    for (int i = DISK_BLOCKS - 1; i >= sb.starting_index_of_data_blocks; --i)
        if (sb.datablock_freelist[i] == 0)
            free_data_block_vector.push_back(i);

    /* maintain free filedescriptor vector */
    for (int i = NO_OF_FILE_DESCRIPTORS - 1; i >= 0; i--)
    {
        free_filedescriptor_vector.push_back(i);
    }

    cout << "Disk is mounted!!!" << endl;
    active = 1;
    return 1;
}

int unmount_disk()
{
    if (!active)
    {
        fprintf(stderr, "close_disk: no open disk\n");
        return -1;
    }

    /* storing super block into begining of virtual disk */
    for (int i = DISK_BLOCKS - 1; i >= sb.starting_index_of_data_blocks; --i)
    {
        sb.datablock_freelist[i] = true;
    }
    /* updating free data block in super block */
    for (unsigned int i = 0; i < free_data_block_vector.size(); i++)
    {
        sb.datablock_freelist[free_data_block_vector.back()] = false;
        free_data_block_vector.pop_back();
    }

    /* Initializing inode free list to true */
    for (int i = 0; i < NO_OF_INODES; ++i)
    {
        sb.inode_freelist[i] = true;
    }

    /* Making those inode nos which are free to false */
    for (unsigned int i = 0; i < free_inode_vector.size(); ++i)
    {
        sb.inode_freelist[free_inode_vector.back()] = false;
        free_inode_vector.pop_back();
    }
    int len;
    /* storing super block structure in starting of virtual disk */
    fseek(diskptr, 0, SEEK_SET);
    len = sizeof(struct super_block);
    char sb_buff[len];
    memset(sb_buff, 0, len);
    memcpy(sb_buff, &sb, sizeof(sb));
    fwrite(sb_buff, sizeof(char), sizeof(sb), diskptr);

    /* storing file_inode mapping after super block into virtual disk */
    fseek(diskptr, (sb.no_of_blocks_used_by_superblock) * BLOCK_SIZE, SEEK_SET);
    len = sizeof(file_inode_mapping_arr);
    char dir_buff[len];
    memset(dir_buff, 0, len);
    memcpy(dir_buff, file_inode_mapping_arr, len);
    fwrite(dir_buff, sizeof(char), len, diskptr);

    /* storing inodes after file_inode mapping into virtual disk */
    fseek(diskptr, (sb.starting_index_of_inodes) * BLOCK_SIZE, SEEK_SET);
    len = sizeof(inode_arr);
    char inode_buff[len];
    memset(inode_buff, 0, len);
    memcpy(inode_buff, inode_arr, len);
    fwrite(inode_buff, sizeof(char), len, diskptr);

    //close all open fd

    cout << "Disk Unmounted!!!" << endl;
    fclose(diskptr);

    active = 0;
    return 1;
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

    if (fseek(diskptr, block * BLOCK_SIZE, SEEK_SET) < 0)
    {
        perror("block_read: failed to lseek");
        return -1;
    }

    if (fread(buf, sizeof(char), BLOCK_SIZE, diskptr) < 0)
    {
        perror("block_read: failed to read");
        return -1;
    }

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

    if (fseek(diskptr, block * BLOCK_SIZE, SEEK_SET) < 0)
    {
        perror("block_write: failed to lseek");
        return -1;
    }

    if (fwrite(buf, sizeof(char), BLOCK_SIZE, diskptr) < 0)
    {
        perror("block_write: failed to write");
        return -1;
    }

    return 0;
}

int user_handle()
{
    int choice;
    int file_mode = -1;
    while (1)
    {
        cout << "1 to create file" << endl;
        cout << "2 to open file" << endl;
        cout << "3 to read file" << endl;
        cout << "4 to write file" << endl;
        cout << "5 to close file" << endl;
        cout << "6 to delete file" << endl;
        cout << "7 to unmount" << endl;
        cin >> choice;
        switch (choice)
        {
        case 1:
            cout << "Enter filename to create" << endl;
            cin >> filename;
            create_file(filename);
            break;
        case 2:
            cout << "Enter filename to open" << endl;
            cin >> filename;
            do
            {
                cout << "0: read mode\n1: write mode\n2: append mode\n";
                cin >> file_mode;
                if (file_mode < 0 || file_mode > 2)
                {
                    cout << "Please make valid choice" << endl;
                }
            } while (file_mode < 0 || file_mode > 2);
            open_file(filename, file_mode);
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            cout << "Enter filedescriptor to close" << endl;
            int fd;
            cin >> fd;
            close_file(fd);
            break;
        case 6:
            cout << "Enter filename to delete" << endl;
            cin >> filename;
            delete_file(filename);
            break;
        case 7:
            unmount_disk();
            return 0;
            break;
        }
    }
}
int main()
{
    int choice;
    while (1)
    {
        cout << "1 to create disk" << endl;
        cout << "2 to mount disk" << endl;
        cout << "9 to exit" << endl;
        cin >> choice;
        if (choice == 9)
            break;
        cout << "Enter diskname : " << endl;
        cin >> disk_name;
        switch (choice)
        {
        case 1:
            create_disk(disk_name);
            break;
        case 2:
            if (mount_disk(disk_name))
            {
                user_handle();
            }
            break;
        case 9:
            break;
        default:
            cout << "Please make valid choice" << endl;
            break;
        }
    }
    return 0;
}