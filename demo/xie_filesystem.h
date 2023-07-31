#pragma once

#include <string>
#include <vector>

namespace xwb
{
    



namespace fs
{

int file2Buffer(const std::string& filename, std::vector<unsigned char>& buffer);

std::string fileMD5(const std::string& filename);

bool exist(const std::string& filename);

int countFiles(const std::string& folder);
int readFolder(const std::string& folder, std::vector<std::string>& filenames);

} // namespace fs


} // namespace xwb