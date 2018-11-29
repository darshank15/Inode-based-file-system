#include "inode.h"

/******************************************************************************/
static int active = 0; /* is the virtual disk open (active) */

char disk_name[50], filename[30];
struct super_block sb;
struct file_to_inode_mapping file_inode_mapping_arr[NO_OF_INODES];
struct inode inode_arr[NO_OF_INODES];

map<string, int> file_to_inode_map;           //file name as key maps to inode (value)
vector<int> free_inode_vector;                //denote free inodes
vector<int> free_data_block_vector;           //denote free data blocks
vector<int> free_filedescriptor_vector;       //denote free filedescriptor.
int openfile_count = 0;                       //keeps track of number of files opened.
map<int, pair<int, int>> file_descriptor_map; //Stores files Descriptor as key and corresponding Inode number(First) and file pointer.
map<int, int> file_descriptor_mode_map;       //Stores mode in which file descriptor is used
map<int, string> inode_to_file_map;           //inode-> filename inode to file mapping
FILE *diskptr;
/******************************************************************************/

int create_disk(char *disk_name)
{
    char buffer[BLOCK_SIZE];

    if(access( disk_name, F_OK ) != -1)
    {
        cout << string(RED) << "Virtual Disk already exists !!!" << string(DEFAULT) << endl;
        return -1;
    }

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
    cout << string(GREEN) << "Virtual Disk Created!!!" << string(DEFAULT) << endl;

    return 1;
}

void printall()
{
    cout << string(BOLD) << "File-inode-Mapping" << string(DEFAULT) << endl;
    for (auto it : file_inode_mapping_arr)
    {
        cout << it.file_name << " : " << it.inode_num << endl;
    }
    cout << "Freeinode vector" << endl;
    int i = 0;
    for (int it = 0; it < NO_OF_INODES; it++)
    {
        if (sb.inode_freelist[it] == false)
            i++;
    }
    cout << i << endl;
    i = 0;
    cout << "File-data-vector" << endl;
    for (int it = 0; it < DISK_BLOCKS; it++)
    {
        if (sb.datablock_freelist[it] == false)
            i++;
    }
    cout << i << endl;
}

int mount_disk(char *name)
{
    diskptr = fopen(disk_name, "rb+");
    if (diskptr == NULL)
    {
        cout << string(RED) << "Disk does not exist :(" << string(DEFAULT) << endl;
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
        if (sb.inode_freelist[i] == true)
        {
            file_to_inode_map[string(file_inode_mapping_arr[i].file_name)] = file_inode_mapping_arr[i].inode_num;
            inode_to_file_map[file_inode_mapping_arr[i].inode_num] = string(file_inode_mapping_arr[i].file_name);
        }
        else
            free_inode_vector.push_back(i);

    /* maintain free data block vector */
    for (int i = DISK_BLOCKS - 1; i >= sb.starting_index_of_data_blocks; --i)
        if (sb.datablock_freelist[i] == false)
            free_data_block_vector.push_back(i);

    /* maintain free filedescriptor vector */
    for (int i = NO_OF_FILE_DESCRIPTORS - 1; i >= 0; i--)
    {
        free_filedescriptor_vector.push_back(i);
    }

    cout << string(GREEN) << "Disk is mounted!!!" << string(DEFAULT) << endl;
    active = 1;
    return 1;
}

int unmount_disk()
{
    if (!active)
    {
        cout << string(RED) << "close_disk: no open disk" << string(DEFAULT) << endl;
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
        sb.datablock_freelist[free_data_block_vector[i]] = false;
    }
    free_data_block_vector.clear();
    /* Initializing inode free list to true */
    for (int i = 0; i < NO_OF_INODES; ++i)
    {
        sb.inode_freelist[i] = true;
    }

    /* Making those inode nos which are free to false */
    for (unsigned int i = 0; i < free_inode_vector.size(); ++i)
    {
        sb.inode_freelist[free_inode_vector[i]] = false;
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

    //clear all in-memory data structures
    free_inode_vector.clear();
    free_data_block_vector.clear();
    free_filedescriptor_vector.clear();
    file_descriptor_mode_map.clear();
    file_descriptor_map.clear();
    file_to_inode_map.clear();
    inode_to_file_map.clear();

    cout << string(GREEN) << "Disk Unmounted!!!" << string(DEFAULT) << endl;
    fclose(diskptr);

    active = 0;
    return 0;
}

int block_read(int block, char *buf)
{
    cout << string(RED);
    if (!active)
    {
        fprintf(stderr, "block_read: disk not active\n");
        cout << string(DEFAULT);
        return -1;
    }

    if ((block < 0) || (block >= DISK_BLOCKS))
    {
        fprintf(stderr, "block_read: block index out of bounds\n");
        cout << string(DEFAULT);
        return -1;
    }

    if (fseek(diskptr, block * BLOCK_SIZE, SEEK_SET) < 0)
    {
        perror("block_read: failed to lseek");
        cout << string(DEFAULT);
        return -1;
    }

    if (fread(buf, sizeof(char), BLOCK_SIZE, diskptr) < 0)
    {
        perror("block_read: failed to read");
        cout << string(DEFAULT);
        return -1;
    }
    cout << string(DEFAULT);
    return 0;
}

int block_write(int block, char *buf, int size, int start_position)
{

    if ((block < 0) || (block >= DISK_BLOCKS))
    {
        fprintf(stderr, "block_write: block index out of bounds\n");
        return -1;
    }

    if (fseek(diskptr, (block * BLOCK_SIZE) + start_position, SEEK_SET) < 0)
    {
        perror("block_write: failed to lseek");
        return -1;
    }

    if (fwrite(buf, sizeof(char), size, diskptr) < 0)
    {
        perror("block_write: failed to write");
        return -1;
    }

    return 0;
}

void print_list_open_files()
{
    cout << string(GREEN) << "List of opened files " << string(DEFAULT) << endl;
    for (auto i : file_descriptor_map)
    {
        int fd = i.first;
        int inode = i.second.first;
        cout << inode_to_file_map[inode] << " is opened with descriptor [ " << fd << " ] in ";
        if (file_descriptor_mode_map[fd] == 0)
            cout << "READ mode" << endl;
        else if (file_descriptor_mode_map[fd] == 1)
            cout << "WRITE mode" << endl;
        else if (file_descriptor_mode_map[fd] == 2)
            cout << "APPEND mode" << endl;
    }
    return;
}

void print_list_files()
{
    cout << string(GREEN) << "List of All files" << string(DEFAULT) << endl;
    for (auto i : file_to_inode_map)
    {
        cout << i.first << " with inode : " << i.second << endl;
    }
    return;
}

int user_handle()
{
    int choice;
    int fd = -1;
    while (1)
    {
        cout << "=========================" << endl;
        cout << "1 : create file" << endl;
        cout << "2 : open file" << endl;
        cout << "3 : read file" << endl;
        cout << "4 : write file" << endl;
        cout << "5 : append file" << endl;
        cout << "6 : close file" << endl;
        cout << "7 : delete file" << endl;
        cout << "8 : list of files" << endl;
        cout << "9 : list of opened files" << endl;
        cout << "10: unmount" << endl;
        cout << "=========================" << endl;
        cin.clear();
        cin >> choice;
        switch (choice)
        {
        case 1:
            cout << "Enter filename to create : ";
            cin >> filename;
            create_file(filename);
            break;
        case 2:
            cout << "Enter filename to open : ";
            cin >> filename;
            open_file(filename);
            break;
        case 3:
            cout << "Enter filedescriptor to read : ";
            cin >> fd;
            // int k;
            // cout << "Enter size to read in kb" << endl;
            // cin >> k;
            read_file(fd);
            cin.clear();
            cout.flush();
            break;
        case 4:
            cout << "Enter filedescriptor to write : ";
            cin >> fd;
            write_into_file(fd, 1);
            cin.clear();
            cout.flush();
            break;
        case 5:
            cout << "Enter filedescriptor to append : ";
            cin >> fd;
            write_into_file(fd, 2);
            cin.clear();
            cout.flush();
            break;
        case 6:
            cout << "Enter filedescriptor to close : ";
            cin >> fd;
            close_file(fd);
            break;
        case 7:
            cout << "Enter filename to delete : ";
            cin >> filename;
            delete_file(filename);
            break;
        case 8:
            print_list_files();
            break;
        case 9:
            print_list_open_files();
            break;
        case 10:
            unmount_disk();
            return 0;
            break;
        default:
            cout << string(RED) << "Please make valid choice." << string(DEFAULT) << endl;
            cin.clear();
            break;
        }
    }
}

int main()
{
    int choice;
    while (1)
    {
        cout << "1 : create disk" << endl;
        cout << "2 : mount disk" << endl;
        cout << "9 : exit" << endl;
        cin.clear();
        cin >> choice;
        if (choice == 9)
        {
            cout << string(GREEN) << "Thank You!!!" << string(DEFAULT) << endl;
            break;
        }
        switch (choice)
        {
        case 1:
            cout << "Enter diskname : " << endl;
            cin >> disk_name;
            create_disk(disk_name);
            break;
        case 2:
            cout << "Enter diskname : " << endl;
            cin >> disk_name;
            if (mount_disk(disk_name))
            {
                user_handle();
            }
            break;
        default:
            cout << string(RED) << "Please make valid choice." << string(DEFAULT) << endl;
            break;
        }
    }
    return 0;
}