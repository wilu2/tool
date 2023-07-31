#pragma once
#include <vector>
#include "TextLine.h"
#include "nlohmann/my_json.hpp"
namespace xwb
{
    

class TableCell
{
public:
    int parse(my_json& j);

public:
    //起始行、终止行、起始列和终止列
    std::vector<int> location;

    std::vector<int> position;
    
    std::vector<int> line_indices;


    std::vector<TextLine> lines;
};


class Table
{
public:
    Table()
    {
        area_index = -1;
        table_rows = -1;
        table_cols = -1;
        page_index = -1;
        table_index = -1;
    }


public:
    int parse(my_json& j);

public:
    int area_index;
    int table_rows;
    int table_cols;


    int page_index;
    int table_index;

    std::string table_type;

    std::vector<TableCell> table_cells;
};


} // namespace  xwb
