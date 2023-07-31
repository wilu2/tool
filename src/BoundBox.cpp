#include "BoundBox.h"
#include <algorithm>
#include <iostream>
using std::min;
using std::max;
using namespace std;
namespace xwb
{

bool BoundBox::is_same_line(const BoundBox& box) const
{
    int min_h = min(bottom-top, box.bottom-box.top);
    int overlap = 0;
    if(min_h > 0)
    {
        overlap = ((min(bottom, box.bottom) - max(top, box.top)) * 100) /min_h ;
    }
    
    return overlap > 50;
}


void BoundBox::merge(const BoundBox& box)
{
    if(box.left == -1 && box.top == -1)
        return;

    if(left == -1 && top == -1)
    {
        *this = box;
        return;
    }

    left = min(left, box.left);
    top = min(top, box.top);
    right = max(right, box.right);
    bottom = max(bottom, box.bottom);
}


BoundBox BoundBox::toBoundBox(const std::vector<int>& polygon)
{
    BoundBox box;
    box.left = min(min(min(polygon[0], polygon[2]), polygon[4]), polygon[6]);
    box.top = min(min(min(polygon[1], polygon[3]), polygon[5]), polygon[7]);
    box.right = max(max(max(polygon[0], polygon[2]), polygon[4]), polygon[6]);
    box.bottom = max(max(max(polygon[1], polygon[3]), polygon[5]), polygon[7]);
    return box;
}

} // namespace xwb


