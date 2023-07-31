#pragma once

#include "nlohmann/my_json.hpp"
#include "Area.h"
#include "TextLine.h"
#include "Context.h"
#include "Table.h"

namespace xwb
{
    
class Page
{
public:
    Page()
    {
        line_avg_height = 0;
        page_index = -1;
        width = 0;
        height = 0;
    }


    int load(my_json& js, int& origin_char_index, Context& context);
    void calculate_project_y();
    void find_margin_y();
    void reset_area_infor(std::vector<TextLine>& textlines, std::wstring type_str);
    int GetTotalChars() const;


    int parse_areas(my_json& js);
    int parse_table(my_json& js);
    int parse_stamp(my_json& js);


public:
    std::vector<TextLine> lines;
    std::vector<Area> areas;
    std::vector<Table> tables;
    int line_avg_height;
    BoundBox box;
    int page_index;
    int width;
    int height;
    std::wstring type;
    std::vector<int> x_project;
    std::vector<int> y_project;
    std::vector<int> y_margin_list;
    std::wstring section_text;
    std::vector<XChar> section_char_list;
    std::vector<int> section_origin_text_index; // 字符下标映射，f[a] = b，a代表过滤标点符号后的文本的某个字符，b代表该字符对应在origin_text中的下标。
    std::wstring zh_no_index = L"一二三四五六七八九十";
    std::vector<int> header_candi_index;
    std::vector<int> footer_candi_index;
};



} // namespace xwb
