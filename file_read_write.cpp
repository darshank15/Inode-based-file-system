#include "inode.h"

int write_into_file(char *name)
{
    string filename = string(name);

    //check if file exist or not
    if (dir_map.find(filename) == dir_map.end())
    {
        cout << "Write File Error : File doesn't exist !!!" << endl;
        return -1;
    }

    //getting inode of file
    int cur_inode = dir_map[filename];
    bool file_not_opened = false;
    for (int i = 0; i < NO_OF_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptor_map.find(i) != file_descriptor_map.end() && file_descriptor_map[i].first == cur_inode)
        {
            file_not_opened = true;
            break;
        }
    }
    if(!file_not_opened){
        cout << "Write File Error : File is not opened !!!" << endl;
        return -1;
    }
    
    //=========== Milestone 1 ===============//

    

}