#include "DocNode.h"

namespace xwb
{

std::map<DocNodeType, std::wstring> docNodeNames = {
    {DOC, L"DOC"},
    {TABLE, L"TABLE"},
    {ROW, L"ROW"},
    {CELL, L"CELL"}
};
  
std::wstring DocNode::getLabel()
{
    if(label.size())
        return label;

    //return ntype;
    if(ntype != CELL)
    {
        label = docNodeNames[ntype];
        return label;
    }

    int width = cell->location[1]-cell->location[0];
    int height = cell->location[3]-cell->location[2];
    
    wchar_t buffer[64] = {0};
    int len = swprintf(buffer, 64, L"{%d},{%d}:", width, height);
    label = std::wstring(buffer);

    for (auto &line : cell->lines)
    {
        len += line.text.size();
    }
    label.reserve(len);

    for (auto &line : cell->lines)
    {
        label += line.text;
    }
    return label;
}


} // namespace xwb
