#include "inode.h"

int create_file(char *name)
{
    string filename = string(name);
    //check if file already exist in disk
    if (dir_map.find(filename) != dir_map.end())
    {
        cout << "Create File Error : File already present !!!" << endl;
        return 0;
    }

    //check if inode are available
    if (free_inode_vector.size() == 0)
    {
        cout << "Create File Error : No more Inodes available" << endl;
        return 0;
    }

    //check if datablock are available
    if (free_data_block_vector.size() == 0)
    {
        cout << "Create File Error : No more DataBlock available" << endl;
        return 0;
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
        return 0;
    }

    //getting inode of file
    int cur_inode = dir_map[filename];

    for (int i = 0; i < NO_OF_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptor_map.find(i) != file_descriptor_map.end() && file_descriptor_map[i].first == cur_inode)
        {
            cout << "Delete File Error : File is opened, Can not delete an opened file !!!" << endl;
            return 0;
        }
    }

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
    }

    inode_arr[cur_inode].pointer[10] = -1;
    free_data_block_vector.push_back(indirectptr);

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

    free_inode_vector.push_back(cur_inode);
    char emptyname[30]="";
    strcpy(file_inode_mapping_arr[cur_inode].file_name,emptyname);
    file_inode_mapping_arr[cur_inode].inode_num=-1;

    dir_map.erase(filename);

    cout<<"File Deleted successfully :) "<<endl;

    return 1;

}