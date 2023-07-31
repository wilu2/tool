#pragma once

#include <string>
#include <vector>
#include <opencv2/core.hpp>

namespace xwb
{
    
class Word2Img
{
public:
    int convert(const std::string& filename, std::vector<cv::Mat>& images);
    int convert(const std::vector<unsigned char>& data, std::vector<cv::Mat>& images);
};


} // namespace xwb
