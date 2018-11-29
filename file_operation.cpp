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
    for (int i = 0; i < 12; ++i)
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
    if (file_to_inode_map.find(filename) != file_to_inode_map.end())
    {
        cout << string(RED) << "Create File Error : File already present !!!" << string(DEFAULT) << endl;
        return -1;
    }

    //check if inode are available
    if (free_inode_vector.size() == 0)
    {
        cout << string(RED) << "Create File Error : No more Inodes available" << string(DEFAULT) << endl;
        return -1;
    }

    //check if datablock are available
    if (free_data_block_vector.size() == 0)
    {
        cout << string(RED) << "Create File Error : No more DataBlock available" << string(DEFAULT) << endl;
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

    file_to_inode_map[filename] = next_avl_inode;
    inode_to_file_map[next_avl_inode] = filename;

    file_inode_mapping_arr[next_avl_inode].inode_num = next_avl_inode;
    strcpy(file_inode_mapping_arr[next_avl_inode].file_name, name);

    cout << string(GREEN) << "File Successfully Created :) " << string(DEFAULT) << endl;
    return 1;
}

int delete_file(char *name)
{
    string filename = string(name);

    //check if file exist or not
    if (file_to_inode_map.find(filename) == file_to_inode_map.end())
    {
        cout << string(RED) << "Delete File Error : File doesn't exist !!!" << string(DEFAULT) << endl;
        return -1;
    }

    //getting inode of file
    int cur_inode = file_to_inode_map[filename];

    for (int i = 0; i < NO_OF_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptor_map.find(i) != file_descriptor_map.end() && file_descriptor_map[i].first == cur_inode)
        {
            cout << string(RED) << "Delete File Error : File is opened, Can not delete an opened file !!!" << string(DEFAULT) << endl;
            return -1;
        }
    }

    erase_inode_content(cur_inode);

    free_inode_vector.push_back(cur_inode);
    char emptyname[30] = "";
    strcpy(file_inode_mapping_arr[cur_inode].file_name, emptyname);
    file_inode_mapping_arr[cur_inode].inode_num = -1;

    inode_to_file_map.erase(file_to_inode_map[filename]);
    file_to_inode_map.erase(filename);

    cout << string(GREEN) << "File Deleted successfully :) " << string(DEFAULT) << endl;

    return 0;
}

int open_file(char *name)
{
    string filename = string(name);
    if (file_to_inode_map.find(filename) == file_to_inode_map.end())
    {
        cout << string(RED) << "Open File Error : File not found !!!" << string(DEFAULT) << endl;
        return -1;
    }

    if (free_filedescriptor_vector.size() == 0)
    {
        cout << string(RED) << "Open File Error : File descriptor not available !!!" << string(DEFAULT) << endl;
        return -1;
    }
    /* asking for mode of file  */
    int file_mode = -1;
    do
    {
        cout << "0 : read mode\n1 : write mode\n2 : append mode\n";
        cin >> file_mode;
        if (file_mode < 0 || file_mode > 2)
        {
            cout << string(RED) << "Please make valid choice" << string(DEFAULT) << endl;
        }
    } while (file_mode < 0 || file_mode > 2);

    int cur_inode = file_to_inode_map[filename];

    /* checking if file is already open in write or append mode. */
    if (file_mode == 1 || file_mode == 2)
    {
        for (int i = 0; i < NO_OF_FILE_DESCRIPTORS; i++)
        {
            if (file_descriptor_map.find(i) != file_descriptor_map.end() &&
                file_descriptor_map[i].first == cur_inode &&
                (file_descriptor_mode_map[i] == 1 || file_descriptor_mode_map[i] == 2))
            {
                cout << string(RED) << "File is already in use with file descriptor : " << i << string(DEFAULT) << endl;
                return -1;
            }
        }
    }

    int fd = free_filedescriptor_vector.back();
    free_filedescriptor_vector.pop_back();

    file_descriptor_map[fd].first = cur_inode;
    file_descriptor_map[fd].second = 0;
    file_descriptor_mode_map[fd] = file_mode;
    openfile_count++;

    cout << string(GREEN) << "File " << filename << " opened with file descriptor  : " << fd << string(DEFAULT) << endl;

    return fd;
}

int close_file(int fd)
{
    if (file_descriptor_map.find(fd) == file_descriptor_map.end())
    {
        cout << string(RED) << "close File Error : file is not opened yet !!!" << string(DEFAULT) << endl;
        return -1;
    }

    file_descriptor_map.erase(fd);
    file_descriptor_mode_map.erase(fd);
    openfile_count--;
    free_filedescriptor_vector.push_back(fd);
    cout << string(GREEN) << "File closed successfully :) " << string(DEFAULT) << endl;
    return 1;
}
