#include "Page.h"
#include "BoundBox.h"
using std::min;
using std::max;
using std::wstring;

namespace xwb
{

int Page::load(my_json& js, int& origin_char_index, Context& context)
{
    js["index"].get_to<int>(page_index);
    js["width"].get_to<int>(width);
    js["height"].get_to<int>(height);

    std::vector<int> catalog_candi;
    bool catalog_flag = false;
    int plain_text_count = 0;
    int form_text_count = 0;

    int sz = js["lines"].size();

    lines.reserve(sz);

    int header_threshold = height / 10;
    int footer_threshold = height - header_threshold;
    
    for(int i = 0; i < js["lines"].size(); i++)
    {
        TextLine line;
        line.init_line(js["lines"][i], page_index, i, context, origin_char_index);

        if(i < 5 && line.text == L"目录")
        {
            catalog_flag = true;
        }

        // 有字才可能是页眉页脚
        // line type 目前分paragrahg, edge, image, table
        if(line.chars.size())
        {
            if(line.poly.bottom < header_threshold && line.area_index < 3)
            {
                header_candi_index.push_back(i);
            }
            else if(line.poly.top > footer_threshold)
            {
                footer_candi_index.push_back(i);
            }
            else if(line.area_type == L"edge")
            {
                if(line.poly.bottom < height/2)
                    header_candi_index.push_back(i);
                else
                    footer_candi_index.push_back(i);
            }
            plain_text_count++;
            line_avg_height += line.height;
        }
       
        lines.push_back(line);
        box.merge(line.poly);
    }

    if(catalog_flag)
    {
        type = L"CATALOG";
    }
    else
    {
        type = L"MAINBODY";
    }

    if(plain_text_count)
        line_avg_height /= plain_text_count;

    x_project.assign(box.right, 0);
    y_project.assign(box.bottom, 0);


    parse_areas(js);
    parse_table(js);
    parse_stamp(js);
    return 0;
}
    
int Page::parse_areas(my_json& js)
{
    if(!js["areas"].is_array())
    {
        return -1;
    }
    int sz = js["areas"].size();
    if(sz == 0)
        return -1;
    areas.reserve(sz);
    for(auto&  js_area : js["areas"])
    {
        Area area;
        area.parse(js_area);
        areas.push_back(area);
    }
    return 0;
}
    
    
int Page::parse_table(my_json& js)
{
    if(!js["tables"].is_array())
    {
        return -1;
    }
    int sz = js["tables"].size();
    if(sz == 0)
        return -1;

    tables.reserve(sz);
    int table_index = 0;
    for(auto& js_table : js["tables"])
    {
        Table table;
        table.parse(js_table);

        if(table.table_type == "table_without_line")
            continue;

        for(auto& cell : table.table_cells)
        {
            for(auto line_index : cell.line_indices)
            {
                lines[line_index-1].form_index = table_index;
                //std::cout << lines.size() << ", " << line_index << std::endl;
                cell.lines.push_back(lines[line_index-1]);
            }
        }
        table.page_index = page_index;
        table.table_index = table_index;

        tables.push_back(table);
        table_index++;
    }
    return 0;
}

int Page::parse_stamp(my_json& js)
{
    for(auto& line : lines)
    {
        if(line.is_stamp)
        {
            areas[line.area_index].type = L"stamp";
            line.area_type = L"stamp";
        }
    }
    return 0;
}


void Page::calculate_project_y()
{

    //    """
    //    :return: calculate line level and character level projection in y direction ,
    //             ignore page header and page footer and form box
    //             compare line high with 2*line avg hight to mark stamper and watermark as noise
    //    """
    for(auto& line : lines)
    {
        if(line.is_header || line.is_footer)
            continue;
        
        if(line.form_index >= 0)
            continue;

        if(areas[line.area_index].type == L"image" || areas[line.area_index].type == L"stamp")
            continue;

        // line level
        for(int i = line.poly.top; i < line.poly.bottom; i++)
        {
            y_project[i]++;
        }

        //character level 
        for(auto& c : line.chars)
        {
            int y_char_start = max(c.box.top, box.top);
            int y_char_end = min(c.box.bottom, box.bottom);
            for(int i = y_char_start; i < y_char_end; i++ )
            {
                y_project[i]+= (c.box.right - c.box.left);
            }
        }
    }
}

void Page::find_margin_y()
{
    // """
    // :return: find suitable margins to mark off paragraphs
    // """

    // y-projection
    int start = 0;
    while(start < y_project.size() && y_project[start] <= 2)
        start++;
    int  end = start + 1;

    while(end < y_project.size())
    {
        if(y_project[end]>2)
        {
            if(end - start > line_avg_height*1.5)
                y_margin_list.push_back(int((start+end)/2));
            // print('start:',start,' end:',end,' ',self.y_project[start:end])
            start = end;
            while(start < y_project.size() && y_project[start] > 2)
            {
                start += 1;
                end = start + 1;
            }
                
        }
        else
            end += 1;
    }
}


void Page::reset_area_infor(std::vector<TextLine>& textlines,wstring type_str)
{
    lines = textlines;
    type = type_str;

    for(auto& line : lines)
    {
        line_avg_height += line.height;
        box.merge(line.poly);
        section_text += line.text;
        //section_char_list.insert(section_char_list.end(), line.chars.begin(), line.chars.end());
        // 比较分词后的词
        //section_char_list.insert(section_char_list.end(), line.word_chars.begin(), line.word_chars.end());
        section_char_list.insert(section_char_list.end(), line.chars.begin(), line.chars.end());
        section_origin_text_index.insert(section_origin_text_index.end(), line.origin_text_index.begin(), line.origin_text_index.end());
    }
    if(lines.size())       
        line_avg_height /= lines.size();
    x_project.assign(box.right, 0);
    y_project.assign(box.bottom, 0);
}


int Page::GetTotalChars() const
{
    int total = 0;

    for(auto& line : lines)
    {
        total += line.chars.size();
        //total += line.word_chars.size();
    }

    return total;
}






} // namespace xwb
