#pragma once

#include <string>
#include <vector>
#include "nlohmann/my_json.hpp"
#include "BoundBox.h"

namespace xwb
{


enum CharStatus{
    NORMAL,
    INSERT,
    DELETE,
    CHANGE
};

std::string statusString(const CharStatus& cs);



class XChar
{
public:
    XChar();

    void merge(const XChar& c);
    bool can_merge(const XChar& c);
    void parse(my_json& js, int idx);
public:
    std::wstring text;
    CharStatus char_status;

    BoundBox box;
    // std::vector<std::vector<std::wstring> > candi;
    // std::vector<std::vector<float> > candi_score;
    
    int page_index;
    int line_index;

    std::vector<BoundBox> pos_list;
};

struct CharCounter
{
    int count_a;
    int count_b;
    int count_same;

    CharCounter()
        :count_a(0), count_b(0), count_same(0)
    {

    }

    void update(int a, int b, int s)
    {
        count_a += a;
        count_b += b;
        count_same += s;
    }

    float GetSimilarity()
    {
        int total = count_a + count_b;
        return total > 0 ? (2.0*count_same)/total : 0;
    }

    float GetCoverage()
    {
        int total = count_a + count_same;
        return total > 0 ? (2.0*count_same)/total : 0;
    }
};

} // namespace xwb
