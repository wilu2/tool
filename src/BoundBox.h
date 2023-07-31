#pragma once

#include <limits.h>
#include <vector>

namespace xwb
{
    

struct BoundBox
{
    int left;
    int top;
    int right;
    int bottom;
    float score;

    BoundBox()
        :left(INT_MAX), top(INT_MAX), right(0), bottom(0), score(0.)
    {

    }

    std::vector<int> to_polygon() const
    {
        return std::vector<int>{left, top, right, top, right, bottom, left, bottom};
    }

    void assign(int l, int t, int r, int b, float s=0.f)
    {
        left = l;
        top = t;
        right = r;
        bottom = b;
        score = s;
    }
    
    bool is_same_line(const BoundBox& box) const;
    void merge(const BoundBox& box);

    static BoundBox toBoundBox(const std::vector<int>& polygon);
};


} // namespace xwb

