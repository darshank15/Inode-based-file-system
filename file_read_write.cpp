#include "inode.h"

string user_input()
{
    cout << "Enter file content : " << endl;
    cout.flush();
    string s;
    do
    {
        string tmp_s;
        getline(cin, tmp_s);
        s += (tmp_s + "\n");
    } while (!cin.eof());
    s.pop_back();
    s.pop_back();
    cin.clear();
    return s;
}

int _write_into_file(int fd, char *buff, int len, int *bytes_written)
{

    int cur_pos = file_descriptor_map[fd].second;
    int filled_data_block = cur_pos / BLOCK_SIZE;
    int file_inode = file_descriptor_map[fd].first;

    /* if last Data block is partially filled */
    if (cur_pos % BLOCK_SIZE != 0)
    {
        /* Write file into direct pointed block */
        if (filled_data_block < 10)
        {
            int data_block_to_write = inode_arr[file_inode].pointer[filled_data_block];
            block_write(data_block_to_write, buff, len, cur_pos % BLOCK_SIZE);
            inode_arr[file_inode].filesize += len;
            file_descriptor_map[fd].second += len; //updating the cur pos in the file
            *bytes_written += len;
        }
        /* Write file into single Indirect block */
        else if (filled_data_block < 1034)
        {
            int block = inode_arr[file_descriptor_map[fd].first].pointer[10];
            int block_pointers[1024]; //Contains the array of data block pointers.
            char read_buf[BLOCK_SIZE];
            block_read(block, read_buf); //reading the block into read_buf
            memcpy(block_pointers, read_buf, sizeof(read_buf));

            int data_block_to_write = block_pointers[filled_data_block - 10];
            block_write(data_block_to_write, buff, len, cur_pos % BLOCK_SIZE);
            inode_arr[file_inode].filesize += len;
            file_descriptor_map[fd].second += len; //updating the cur pos in the file
            *bytes_written += len;
        }
        /* Write file into double Indirect block */
        else
        {
            int block = inode_arr[file_descriptor_map[fd].first].pointer[11];
            int block_pointers[1024]; //Contains the array of data block pointers.
            char read_buf[BLOCK_SIZE];
            block_read(block, read_buf); //reading the block into read_buf
            memcpy(block_pointers, read_buf, sizeof(read_buf));

            int block2 = block_pointers[(filled_data_block - 1034) / 1024];
            int block_pointers2[1024];    //Contains the array of data block pointers.
            block_read(block2, read_buf); //reading the block2 into read_buf
            memcpy(block_pointers2, read_buf, sizeof(read_buf));

            int data_block_to_write = block_pointers[(filled_data_block - 1034) / 1024];
            block_write(data_block_to_write, buff, len, cur_pos % BLOCK_SIZE);
            inode_arr[file_inode].filesize += len;
            file_descriptor_map[fd].second += len; //updating the cur pos in the file
            *bytes_written += len;
        }
    }
    /* if last data block is fully filled */
    else
    {
        /* if filesize = 0 means file is empty then start writting into file from first direct block. */
        if (inode_arr[file_descriptor_map[fd].first].filesize == 0)
        {
            block_write(inode_arr[file_inode].pointer[0], buff, len, 0);
            inode_arr[file_inode].filesize += len;
            file_descriptor_map[fd].second += len;
            *bytes_written += len;
        }
        /* if filesize != 0 then */
        else
        {
            if (cur_pos == 0)
            {
                /* flush all data blocks and start writting into file from first block */
                if (file_descriptor_map[fd].second == 0)
                {
                    erase_inode_content(file_inode);
                    //check if datablock are available
                    if (free_data_block_vector.size() == 0)
                    {
                        cout << string(RED) << "Write File Error : No more DataBlock available" << string(DEFAULT) << endl;
                        return -1;
                    }
                    int next_avl_datablock = free_data_block_vector.back();
                    free_data_block_vector.pop_back();
                    inode_arr[file_inode].pointer[0] = next_avl_datablock;
                }
                block_write(inode_arr[file_inode].pointer[0], buff, len, 0);
                inode_arr[file_inode].filesize += len;
                file_descriptor_map[fd].second += len;
                *bytes_written += len;
            }
            else
            {
                if (filled_data_block < 10)
                {
                    //check if datablock are available
                    if (free_data_block_vector.size() == 0)
                    {
                        cout << string(RED) << "Write File Error : No more DataBlock available" << string(DEFAULT) << endl;
                        return -1;
                    }
                    int data_block_to_write = free_data_block_vector.back();
                    free_data_block_vector.pop_back();
                    inode_arr[file_inode].pointer[filled_data_block] = data_block_to_write;

                    block_write(data_block_to_write, buff, len, cur_pos % BLOCK_SIZE);
                    inode_arr[file_inode].filesize += len;
                    file_descriptor_map[fd].second += len; //updating the cur pos in the file
                    *bytes_written += len;
                }
                else if (filled_data_block < 1034)
                {
                    if (filled_data_block == 10) //i.e all direct DB pointer 0-9 is full then need to allocate a DB to pointer[10] & store 1024 DB in this DB
                    {
                        int block_pointers[1024];
                        for (int i = 0; i < 1024; ++i)
                            block_pointers[i] = -1;

                        /* to store block_pointers[1024] into db_for_single_indirect */
                        //check if datablock are available
                        if (free_data_block_vector.size() == 0)
                        {
                            cout << string(RED) << "Write File Error : No more DataBlock available" << string(DEFAULT) << endl;
                            return -1;
                        }
                        int data_block_single_indirect = free_data_block_vector.back();
                        free_data_block_vector.pop_back();

                        inode_arr[file_inode].pointer[10] = data_block_single_indirect;
                        char temp_buf[BLOCK_SIZE];
                        memcpy(temp_buf, block_pointers, BLOCK_SIZE);

                        block_write(data_block_single_indirect, temp_buf, BLOCK_SIZE, 0);
                    }

                    int block = inode_arr[file_inode].pointer[10];
                    int block_pointers[1024]; //Contains the array of data block pointers.
                    char read_buf[BLOCK_SIZE];
                    block_read(block, read_buf);                        //reading the single indirect block into read_buf
                    memcpy(block_pointers, read_buf, sizeof(read_buf)); //moving the data into blockPointer

                    //check if datablock are available
                    if (free_data_block_vector.size() == 0)
                    {
                        cout << string(RED) << "Write File Error : No more DataBlock available" << string(DEFAULT) << endl;
                        return -1;
                    }
                    int data_block_to_write = free_data_block_vector.back();
                    free_data_block_vector.pop_back();

                    //store back the blockPointer to disk
                    block_pointers[filled_data_block - 10] = data_block_to_write;
                    char temp_buf[BLOCK_SIZE];
                    memcpy(temp_buf, block_pointers, BLOCK_SIZE);
                    block_write(block, temp_buf, BLOCK_SIZE, 0);

                    //write data into DB
                    block_write(data_block_to_write, buff, len, 0);
                    inode_arr[file_inode].filesize += len;
                    file_descriptor_map[fd].second += len; //updating the cur pos in the file
                    *bytes_written += len;
                }
                else
                {

                    if (filled_data_block == 1034)
                    {
                        int block_pointers[1024];
                        for (int i = 0; i < 1024; ++i)
                            block_pointers[i] = -1;
                        //check if datablock are available
                        if (free_data_block_vector.size() == 0)
                        {
                            cout << string(RED) << "Write File Error : No more DataBlock available" << string(DEFAULT) << endl;
                            return -1;
                        }
                        int data_block_double_indirect = free_data_block_vector.back(); //to store block_pointers[1024] into db_for_double_indirect
                        free_data_block_vector.pop_back();

                        inode_arr[file_inode].pointer[11] = data_block_double_indirect;
                        char temp_buf[BLOCK_SIZE];
                        memcpy(temp_buf, block_pointers, BLOCK_SIZE);

                        block_write(data_block_double_indirect, temp_buf, BLOCK_SIZE, 0);
                    }
                    if ((filled_data_block - 1034) % 1024 == 0) //i.e if filled_data_block is multiple of 1024 means need new DB to be assigned
                    {
                        int block = inode_arr[file_inode].pointer[11];
                        int block_pointers[1024]; //Contains the array of data block pointers.
                        char read_buf[BLOCK_SIZE];
                        block_read(block, read_buf); //reading the block into read_buf
                        memcpy(block_pointers, read_buf, sizeof(read_buf));

                        int block_pointers2[1024];
                        for (int i = 0; i < 1024; ++i)
                            block_pointers2[i] = -1;
                        //check if datablock are available
                        if (free_data_block_vector.size() == 0)
                        {
                            cout << string(RED) << "Write File Error : No more DataBlock available" << string(DEFAULT) << endl;
                            return -1;
                        }
                        int data_block_double_indirect2 = free_data_block_vector.back(); //to store block_pointers[1024] into db_for_double_indirect
                        free_data_block_vector.pop_back();

                        block_pointers[(filled_data_block - 1034) / 1024] = data_block_double_indirect2;
                        char temp_buf[BLOCK_SIZE];
                        memcpy(temp_buf, block_pointers2, BLOCK_SIZE);
                        block_write(data_block_double_indirect2, temp_buf, BLOCK_SIZE, 0);

                        memcpy(temp_buf, block_pointers, BLOCK_SIZE);
                        block_write(block, temp_buf, BLOCK_SIZE, 0);
                    }

                    int block = inode_arr[file_inode].pointer[11];
                    int block_pointers[1024]; //Contains the array of data block pointers.
                    char read_buf[BLOCK_SIZE];
                    block_read(block, read_buf); //reading the block into read_buf
                    memcpy(block_pointers, read_buf, BLOCK_SIZE);

                    int block2 = block_pointers[(filled_data_block - 1034) / 1024];
                    int block_pointers2[1024]; //Contains the array of data block pointers.
                    char read_buf2[BLOCK_SIZE];
                    block_read(block2, read_buf2); //reading the block2 into read_buf2
                    memcpy(block_pointers2, read_buf2, BLOCK_SIZE);

                    //check if datablock are available
                    if (free_data_block_vector.size() == 0)
                    {
                        cout << string(RED) << "Write File Error : No more DataBlock available" << string(DEFAULT) << endl;
                        return -1;
                    }
                    int data_block_to_write = free_data_block_vector.back(); //to store block_pointers[1024] into db_for_double_indirect
                    free_data_block_vector.pop_back();
                    block_pointers2[(filled_data_block - 1034) % 1024] = data_block_to_write;
                    block_write(data_block_to_write, buff, len, 0); //writing data into db_to_write DB

                    //now restore block_pointers2 back to the block2
                    char temp_buf[BLOCK_SIZE];
                    memcpy(temp_buf, block_pointers2, BLOCK_SIZE);
                    block_write(block2, temp_buf, BLOCK_SIZE, 0);

                    //updating the filesize
                    inode_arr[file_inode].filesize += len;
                    file_descriptor_map[fd].second += len; //updating the cur pos in the file
                    *bytes_written += len;
                }
            }
        }
    }
    return 0;
}

int write_into_file(int fd, int mode)
{

    //check if file exist or not
    if (file_descriptor_map.find(fd) == file_descriptor_map.end())
    {
        cout << string(RED) << "Write File Error : File descriptor " << fd << " doesn't exist !!!" << string(DEFAULT) << endl;
        return -1;
    }

    //getting inode of file
    int cur_inode = file_descriptor_map[fd].first;
    if (mode == 1)
    {
        /* Write  */
        if (file_descriptor_mode_map[fd] != 1)
        {
            cout << string(RED) << "Write File Error : File descriptor " << fd << " is not opened in write mode!!!" << string(DEFAULT) << endl;
            return -1;
        }

        // Make All read pointers at start of file.[ set second elem of pair to 0 ]
        for (int i = 0; i < NO_OF_FILE_DESCRIPTORS; i++)
        {
            if (file_descriptor_map.find(i) != file_descriptor_map.end() &&
                file_descriptor_map[i].first == cur_inode &&
                file_descriptor_mode_map[i] == 0)
            {
                file_descriptor_map[i].second = 0;
            }
        }
    }
    else
    {
        /* Append */
        if (file_descriptor_mode_map[fd] != 2)
        {
            cout << string(RED) << "Append File Error : File descriptor " << fd << " is not opened in append mode!!!" << string(DEFAULT) << endl;
            return -1;
        }
        file_descriptor_map[fd].second = inode_arr[cur_inode].filesize;
    }

    string x = user_input();
    x = x.substr(1);
    int bytes_written = 0;

    //checking how many in last DB have space remaing to store data
    unsigned int remaining_size_in_last_written_data_block = BLOCK_SIZE - ((file_descriptor_map[fd].second) % BLOCK_SIZE);
    int len = -1;
    //if remaining empty size in last data block is more than buffer size then write directly in last data block
    if (remaining_size_in_last_written_data_block >= x.size())
    {
        len = x.size();
        char buff[len + 1];
        memset(buff, 0, len);
        memcpy(buff, x.c_str(), len);
        buff[len] = '\0';
        _write_into_file(fd, buff, len, &bytes_written);
    }
    else
    {
        //1st write remaining size in last written data block
        len = remaining_size_in_last_written_data_block;
        char buff[len + 1];
        memset(buff, 0, len);
        memcpy(buff, x.c_str(), len);
        buff[len] = '\0';
        _write_into_file(fd, buff, len, &bytes_written);
        x = x.substr(len);

        //write block by block till last block
        int remaining_block_count = x.size() / BLOCK_SIZE;
        while (remaining_block_count--)
        {
            len = BLOCK_SIZE;
            char buff[len + 1];
            memset(buff, 0, len);
            memcpy(buff, x.c_str(), len);
            buff[len] = '\0';
            x = x.substr(len);
            if (_write_into_file(fd, buff, len, &bytes_written) == -1)
            {
                cout << string(RED) << "No Enough space. Only " << bytes_written << " bytes written." << string(DEFAULT) << endl;
                return -1;
            }
        }

        //write last partial block
        int remaining_size = x.size() % BLOCK_SIZE;
        len = remaining_size;
        char buff1[len + 1];
        memset(buff1, 0, len);
        memcpy(buff1, x.c_str(), len);
        buff1[len] = '\0';
        if (_write_into_file(fd, buff1, len, &bytes_written) == -1)
        {
            cout << string(RED) << "No Enough space. Only " << bytes_written << " bytes written." << string(DEFAULT) << endl;
            return -1;
        }
    }
    cout << string(GREEN) << bytes_written << " bytes written. \nFile Written Successfully." << string(DEFAULT) << endl;
    return 0;
}

int read_file(int fd)
{

    if (file_descriptor_map.find(fd) == file_descriptor_map.end())
    {
        cout << string(RED) << "Read File Error : file is not opened yet !!!" << string(DEFAULT) << endl;
        return -1;
    }

    if (file_descriptor_mode_map[fd] != 0)
    {
        cout << string(RED) << "Read File Error : file with descriptor " << fd << " is not opened in read mode !!!" << string(DEFAULT) << endl;
        return -1;
    }

    int bytes_read = 0;
    bool partial_read = false;
    int fs = file_descriptor_map[fd].second;

    int cur_inode = file_descriptor_map[fd].first;
    struct inode in = inode_arr[cur_inode];
    int filesize = in.filesize;
    char *buf;
    buf = new char[filesize];
    char *initial_buf_pos = buf;

    int noOfBlocks = ceil(((float)inode_arr[cur_inode].filesize) / BLOCK_SIZE);
    int tot_block = noOfBlocks; // tot_block = numner of blocks to read and noOfBlocks = blocks left to read
    char read_buf[BLOCK_SIZE];

    for (int i = 0; i < 10; i++)
    {

        if (noOfBlocks == 0)
        {
            break;
        }
        int block_no = in.pointer[i];

        block_read(block_no, read_buf);

        if ((tot_block - noOfBlocks >= fs / BLOCK_SIZE) && (noOfBlocks > 1))
        {
            if (partial_read == false)
            {
                memcpy(buf, read_buf + (fs % BLOCK_SIZE), (BLOCK_SIZE - fs % BLOCK_SIZE));
                buf = buf + (BLOCK_SIZE - fs % BLOCK_SIZE);
                partial_read = true;
                bytes_read += BLOCK_SIZE - fs % BLOCK_SIZE;
            }
            else
            {

                memcpy(buf, read_buf, BLOCK_SIZE);
                buf = buf + BLOCK_SIZE;
                bytes_read += BLOCK_SIZE;
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
            block_read(blockPointers[i++], read_buf);

            if (tot_block - noOfBlocks >= fs / BLOCK_SIZE && noOfBlocks > 1)
            {
                if (partial_read == false)
                {

                    memcpy(buf, read_buf + (fs % BLOCK_SIZE), (BLOCK_SIZE - fs % BLOCK_SIZE));
                    buf = buf + (BLOCK_SIZE - fs % BLOCK_SIZE);
                    partial_read = true;
                    bytes_read += BLOCK_SIZE - fs % BLOCK_SIZE;
                }
                else
                {

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

                        memcpy(buf, read_buf + (fs % BLOCK_SIZE), (BLOCK_SIZE - fs % BLOCK_SIZE));
                        buf = buf + (BLOCK_SIZE - fs % BLOCK_SIZE);
                        partial_read = true;
                        bytes_read += BLOCK_SIZE - fs % BLOCK_SIZE;
                    }
                    else
                    {

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
        memcpy(buf, read_buf, (inode_arr[cur_inode].filesize) % BLOCK_SIZE);
        bytes_read += (inode_arr[cur_inode].filesize) % BLOCK_SIZE;
    }
    else if (tot_block - fs / BLOCK_SIZE == 1)
    {
        memcpy(buf, read_buf + (fs % BLOCK_SIZE), (inode_arr[cur_inode].filesize) % BLOCK_SIZE - fs % BLOCK_SIZE);
        bytes_read += (inode_arr[cur_inode].filesize) % BLOCK_SIZE - fs % BLOCK_SIZE;
    }

    initial_buf_pos[bytes_read] = '\0';
    cout.flush();
    cout << initial_buf_pos << endl;
    cout.flush();
    cout << string(GREEN) << "File read successfully " << string(DEFAULT) << endl;
    return 1;
}