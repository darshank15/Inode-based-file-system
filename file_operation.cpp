#include "inode.h"

int erase_inode_content(int cur_inode)
{
    //flag to verify if deletion completed or not
    bool delete_completed = false;

    //iterate over direct pointers
    for (int i = 0; i < 10; i++)
    {
        if (inode_arr[cur_inode].pointer[i] == -1)
        {
            delete_completed = true;
            break;
        }
        else
        {
            free_data_block_vector.push_back(inode_arr[cur_inode].pointer[i]);
            inode_arr[cur_inode].pointer[i] = -1;
        }
    }

    int indirectptr;
    if (!delete_completed)
    {
        //reading content of block pointed by indirect pointer
        indirectptr = inode_arr[cur_inode].pointer[10];
        char blockbuffer[BLOCK_SIZE];
        int indirect_ptr_array[1024];
        block_read(indirectptr, blockbuffer);
        memcpy(indirect_ptr_array, blockbuffer, sizeof(indirect_ptr_array));

        for (int i = 0; i < 1024; i++)
        {
            if (indirect_ptr_array[i] == -1)
            {
                delete_completed = true;
                break;
            }
            else
            {
                free_data_block_vector.push_back(indirect_ptr_array[i]);
            }
        }

        inode_arr[cur_inode].pointer[10] = -1;
        free_data_block_vector.push_back(indirectptr);
    }

    int doubleindirectptr;
    if (!delete_completed)
    {
        doubleindirectptr = inode_arr[cur_inode].pointer[11];
        char blockbuffer[BLOCK_SIZE];
        int doubleindirect_ptr_array[1024];
        block_read(doubleindirectptr, blockbuffer);
        memcpy(doubleindirect_ptr_array, blockbuffer, sizeof(doubleindirect_ptr_array));

        for (int i = 0; i < 1024; i++)
        {
            if (doubleindirect_ptr_array[i] == -1)
            {
                delete_completed = true;
                break;
            }
            else
            {

                int singleindirectptr = doubleindirect_ptr_array[i];
                char blockbuffer1[BLOCK_SIZE];
                int indirect_ptr_array[1024];
                block_read(singleindirectptr, blockbuffer1);
                memcpy(indirect_ptr_array, blockbuffer1, sizeof(indirect_ptr_array));

                for (int i = 0; i < 1024; i++)
                {
                    if (indirect_ptr_array[i] == -1)
                    {
                        delete_completed = true;
                        break;
                    }
                    else
                    {
                        free_data_block_vector.push_back(indirect_ptr_array[i]);
                    }
                }

                free_data_block_vector.push_back(singleindirectptr);
            }
        }

        free_data_block_vector.push_back(doubleindirectptr);
    }

    //Resetting inode structure with default values.
    for (int i = 0; i <= 11; ++i)
    {
        inode_arr[cur_inode].pointer[i] = -1;
    }
    inode_arr[cur_inode].filesize = 0;
    return 0;
}

int create_file(char *name)
{
    string filename = string(name);
    //check if file already exist in disk
    if (dir_map.find(filename) != dir_map.end())
    {
        cout << "Create File Error : File already present !!!" << endl;
        return -1;
    }

    //check if inode are available
    if (free_inode_vector.size() == 0)
    {
        cout << "Create File Error : No more Inodes available" << endl;
        return -1;
    }

    //check if datablock are available
    if (free_data_block_vector.size() == 0)
    {
        cout << "Create File Error : No more DataBlock available" << endl;
        return -1;
    }

    //get next free inode and datablock number
    int next_avl_inode = free_inode_vector.back();
    free_inode_vector.pop_back();
    int next_avl_datablock = free_data_block_vector.back();
    free_data_block_vector.pop_back();

    //assigned one data block to this inode
    inode_arr[next_avl_inode].pointer[0] = next_avl_datablock;
    inode_arr[next_avl_inode].filesize = 0;

    dir_map[filename] = next_avl_inode;

    file_inode_mapping_arr[next_avl_inode].inode_num = next_avl_inode;
    strcpy(file_inode_mapping_arr[next_avl_inode].file_name, name);

    cout << "File Successfully Created :) " << endl;
    return 1;
}

int delete_file(char *name)
{
    string filename = string(name);

    //check if file exist or not
    if (dir_map.find(filename) == dir_map.end())
    {
        cout << "Delete File Error : File doesn't exist !!!" << endl;
        return -1;
    }

    //getting inode of file
    int cur_inode = dir_map[filename];

    for (int i = 0; i < NO_OF_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptor_map.find(i) != file_descriptor_map.end() && file_descriptor_map[i].first == cur_inode)
        {
            cout << "Delete File Error : File is opened, Can not delete an opened file !!!" << endl;
            return -1;
        }
    }

    erase_inode_content(cur_inode);

    free_inode_vector.push_back(cur_inode);
    char emptyname[30] = "";
    strcpy(file_inode_mapping_arr[cur_inode].file_name, emptyname);
    file_inode_mapping_arr[cur_inode].inode_num = -1;

    dir_map.erase(filename);

    cout << "File Deleted successfully :) " << endl;

    return 0;
}

int open_file(char *name, int file_mode)
{
    string filename = string(name);
    if (dir_map.find(filename) == dir_map.end())
    {
        cout << "Open File Error : File not found !!!" << endl;
        return -1;
    }

    if (free_filedescriptor_vector.size() == 0)
    {
        cout << "Open File Error : File descriptor not available !!!" << endl;
        return -1;
    }

    int cur_inode = dir_map[filename];
    int fd = free_filedescriptor_vector.back();
    free_filedescriptor_vector.pop_back();

    file_descriptor_map[fd].first = cur_inode;
    file_descriptor_map[fd].second = 0;
    file_descriptor_mode_map[fd] = file_mode;
    openfile_count++;

    cout << "File " << filename << " opened with file descriptor  : " << fd << endl;

    return fd;
}

int close_file(int fd)
{
    if (file_descriptor_map.find(fd) == file_descriptor_map.end())
    {
        cout << "close File Error : file is not opened yet !!!" << endl;
        return -1;
    }

    file_descriptor_map.erase(fd);
    file_descriptor_mode_map.erase(fd);
    openfile_count--;
    free_filedescriptor_vector.push_back(fd);
    cout << "File closed successfully :) " << endl;
    return 1;
}

int read_file(int fd, char *buf, int kbytes)
{

    if (file_descriptor_map.find(fd) == file_descriptor_map.end())
    {
        cout << "Read File Error : file is not opened yet !!!" << endl;
        return -1;
    }

    if (file_descriptor_mode_map[fd] != 0)
    {
        cout << "Read File Error : file with descriptor " << fd << " is not opened in read mode !!!" << endl;
        return -1;
    }

    int bytes_read = 0;
    bool partial_read = false;
    int fs = file_descriptor_map[fd].second;
    cout << "**********************" << fs << "@@@@@@@@@@@@@@@@@@@@@@" << endl;
    int cur_inode = file_descriptor_map[fd].first;
    struct inode in = inode_arr[cur_inode];
    // int filesize = in.filesize;
    kbytes *= 1024;
    buf = new char[kbytes];
    memset(buf, 0, kbytes);
    char *initial_buf_pos = buf;
 
    int noOfBlocks = ceil(((float)in.filesize) / BLOCK_SIZE);
    int tot_block = noOfBlocks; // tot_block = numner of blocks to read and noOfBlocks = blocks left to read
    char read_buf[BLOCK_SIZE];

    // char dest_filename[20];
    // strcpy(dest_filename, file_inode_mapping_arr[cur_inode].file_name);
    // FILE *fp1 = fopen(dest_filename, "wb+");

    for (int i = 0; i < 10; i++)
    {
        if (noOfBlocks == 0)
        {
            break;
        }
        cout << "direct-------------------" << i << endl;
        int block_no = in.pointer[i];

        block_read(block_no, read_buf);

        if ((tot_block - noOfBlocks >= fs / BLOCK_SIZE) && (noOfBlocks > 1))
        {
            if (partial_read == false)
            {
                // fwrite(read_buf + (fs % BLOCK_SIZE), 1, (BLOCK_SIZE - fs % BLOCK_SIZE), fp1);
                // need concatenation
                memcpy(buf, read_buf + (fs % BLOCK_SIZE), (BLOCK_SIZE - fs % BLOCK_SIZE));
                buf = buf + (BLOCK_SIZE - fs % BLOCK_SIZE);
                partial_read = true;
                bytes_read += BLOCK_SIZE - fs % BLOCK_SIZE;
            }
            else
            {
                //fwrite(read_buf, 1, BLOCK_SIZE, fp1);
                memcpy(buf, read_buf, BLOCK_SIZE);
                buf = buf + BLOCK_SIZE;
                bytes_read += BLOCK_SIZE;
            }
            if(bytes_read >= kbytes-BLOCK_SIZE)
            {
                noOfBlocks=2;
            }
        }

        noOfBlocks--;
    }

    if (noOfBlocks) //Just to check any single indirect pointers are used or not
    {

        int block = inode_arr[cur_inode].pointer[10];
        int blockPointers[1024]; //Contains the array of data block pointers.
        block_read(block, read_buf);
        memcpy(blockPointers, read_buf, sizeof(read_buf));

        int i = 0;
        while (noOfBlocks && i < 1024)
        {
            cout << "single indirect-------------------" << i << endl;
            block_read(blockPointers[i++], read_buf);

            if (tot_block - noOfBlocks >= fs / BLOCK_SIZE && noOfBlocks > 1)
            {
                if (partial_read == false)
                {
                    // fwrite(read_buf + (fs % BLOCK_SIZE), 1, (BLOCK_SIZE - fs % BLOCK_SIZE), fp1);
                    //concatenation code
                    memcpy(buf, read_buf + (fs % BLOCK_SIZE), (BLOCK_SIZE - fs % BLOCK_SIZE));
                    buf = buf + (BLOCK_SIZE - fs % BLOCK_SIZE);
                    partial_read = true;
                    bytes_read += BLOCK_SIZE - fs % BLOCK_SIZE;
                }
                else
                {
                    // fwrite(read_buf, 1, BLOCK_SIZE, fp1);
                    memcpy(buf, read_buf, BLOCK_SIZE);
                    buf = buf + BLOCK_SIZE;
                    bytes_read += BLOCK_SIZE;
                }
            }

            noOfBlocks--;
        }
    }

    if (noOfBlocks) //Indirect pointers done check for Double Indirect
    {
        int block = inode_arr[cur_inode].pointer[11];
        int indirectPointers[1024]; //Contains array of indirect pointers
        block_read(block, read_buf);
        memcpy(indirectPointers, read_buf, sizeof(read_buf));
        int i = 0;
        while (noOfBlocks && i < 1024)
        {
            cout << "double indirect-------------------" << i << endl;
            block_read(indirectPointers[i++], read_buf);
            int blockPointers[1024];
            memcpy(blockPointers, read_buf, sizeof(read_buf));
            int j = 0;
            while (noOfBlocks && j < 1024)
            {
                block_read(blockPointers[j++], read_buf);

                if (tot_block - noOfBlocks >= fs / BLOCK_SIZE && noOfBlocks > 1)
                {
                    if (partial_read == false)
                    {
                        // fwrite(read_buf + (fs % BLOCK_SIZE), 1, (BLOCK_SIZE - fs % BLOCK_SIZE), fp1);
                        //concatenation code
                        memcpy(buf, read_buf + (fs % BLOCK_SIZE), (BLOCK_SIZE - fs % BLOCK_SIZE));
                        buf = buf + (BLOCK_SIZE - fs % BLOCK_SIZE);
                        partial_read = true;
                        bytes_read += BLOCK_SIZE - fs % BLOCK_SIZE;
                    }
                    else
                    {
                        // fwrite(read_buf, 1, BLOCK_SIZE, fp1);
                        memcpy(buf, read_buf, BLOCK_SIZE);
                        buf = buf + BLOCK_SIZE;
                        bytes_read += BLOCK_SIZE;
                    }
                }
                noOfBlocks--;
            }
        }
    }
    if (tot_block - fs / BLOCK_SIZE > 1)
    {
        // fwrite(read_buf, 1, (inode_arr[cur_inode].filesize) % BLOCK_SIZE, fp1);
        memcpy(buf, read_buf, (inode_arr[cur_inode].filesize) % BLOCK_SIZE);
        bytes_read += (inode_arr[cur_inode].filesize) % BLOCK_SIZE;
    }
    else if (tot_block - fs / BLOCK_SIZE == 1)
    {
        // fwrite(read_buf + (fs % BLOCK_SIZE), 1, (inode_arr[cur_inode].filesize) % BLOCK_SIZE - fs % BLOCK_SIZE, fp1);
        memcpy(buf, read_buf + (fs % BLOCK_SIZE), (inode_arr[cur_inode].filesize) % BLOCK_SIZE - fs % BLOCK_SIZE);
        bytes_read += (inode_arr[cur_inode].filesize) % BLOCK_SIZE - fs % BLOCK_SIZE;
    }
    cout.flush();
    cout << initial_buf_pos << endl;
    cout.flush();
    cout << "File read successfully with bytes: "<<bytes_read<< endl;
    file_descriptor_map[fd].second = file_descriptor_map[fd].second + bytes_read;

    // fclose(fp1);
    return 0;
}