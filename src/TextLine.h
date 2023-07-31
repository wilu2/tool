#pragma once

#include <vector>
#include <string>
#include "Context.h"
#include "XChar.h"
#include "nlohmann/my_json.hpp"

namespace xwb
{
    
class TextLine
{
public:
    TextLine(){
        page_index = -1;
        height = -1;
        width = -1;
        char_avg_width = -1;
        form_index = -1;

        is_footer = false;
        is_header = false;
        is_noise = false;
        is_caption = false;
        is_stamp = false;

        line_index = -1;

        area_index = -1;
    };

    void WordSegmentation();
    void IgnorePunctuation(Context& context, int& origin_char_index);
    int init_line(my_json& js, int page_index, int line_index, Context& context, int& origin_char_index);
    
    // 只是为了放入map中。
    bool operator < (const TextLine& rhs) const {
        return line_index < rhs.line_index;
    };

    bool operator > (const TextLine& rhs) const {
        return line_index > rhs.line_index;
  };


public:
    std::wstring text;
    std::wstring origin_text; // 该line的文本内容对应的原始（未过滤标点）文本
    BoundBox poly;
    std::vector<int> origin_text_index; // 该段文本的字符下标映射，f[a] = b，a代表过滤标点符号后的文本的某个字符，b代表该字符对应在文档origin_text中的下标。
    std::vector<XChar> chars;
    std::vector<XChar> word_chars;

    int page_index;
    int width;
    int height;
    int char_avg_width;

    int form_index;
    bool is_header;
    bool is_footer;
    bool is_noise;
    bool is_caption;
    // 是否印章
    bool is_stamp;

    int line_index;

    std::wstring area_type;
    int area_index;
};



} // namespace xwb
