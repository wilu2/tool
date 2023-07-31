#include "xie_filesystem.h"
#include "md5.h"
#include <cstring>
#include <iostream>
#include <dirent.h>
#include <experimental/filesystem>


namespace xwb
{
    



namespace fs
{


int file2Buffer(const std::string& filename, std::vector<unsigned char>& buffer)
{
    int ret = -1;
    FILE * pf = fopen(filename.c_str(),"rb");
    if ( NULL == pf )
    {
       return ret;
    }

    fseek(pf,0L, SEEK_END);
    int size= ftell(pf);
    fseek(pf,0L,SEEK_SET);

    buffer.resize(size);

    int readsize = fread(buffer.data() ,size , 1 , pf);
    if ( size == readsize )
    {
        ret = 0;
    }
    fclose ( pf);
    return true;
}

std::string fileMD5(const std::string& filename)
{
    std::vector<unsigned char> buffer;
    file2Buffer(filename, buffer);
    if(buffer.size())
    {
        return MD5Digest(buffer.data(), buffer.size());
    }
    return "";
}

bool exist(const std::string& filename)
{
    return std::experimental::filesystem::exists(filename);
}

int countFiles(const std::string& folder)
{
    std::vector<std::string> filenames;
    return readFolder(folder, filenames);
}

int readFolder(const std::string& folder, std::vector<std::string>& filenames)
{
    if (folder.empty())
    {
        std::cout << "folder name invalid, plz check." << std::endl;
        return 0;
    }

    DIR *pDir = opendir(folder.c_str());
    if (NULL == pDir)
    {
        std::cout << folder << " don't exist." << std::endl;
        return 0;
    }
    std::string prefix_name = folder;
    dirent *pDRent = NULL;
    while ((pDRent = readdir(pDir)) != NULL)
    {
        if (strlen(pDRent->d_name) > 3)
        {
            std::string postfix_name(pDRent->d_name);
            // file_name.push_back(postfix_name);
            std::string full_path = prefix_name + "/" + postfix_name;
            filenames.push_back(full_path);
        }
    }
    closedir(pDir); 
    return filenames.size();
}



} // namespace fs


} // namespace xwb