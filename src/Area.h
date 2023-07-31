#pragma once

#include <vector>
#include <string>
#include "nlohmann/my_json.hpp"

namespace xwb
{
    
class Area
{
public:
    void parse(my_json& js);


public:
    int index;
    float score;
    std::wstring type;
    std::vector<int> position;
};


} // namespace xwb