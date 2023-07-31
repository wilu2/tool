#include "TextLine.h"
#include "nlohmann/my_json.hpp"
#include "str_utils.h"
#include "XChar.h"
#include <regex>

using std::wregex;
using std::regex_replace;

namespace xwb
{

static wchar_t toHalfWidth(wchar_t c)
{
    wchar_t r = c;
    if(c == 12288)
        r = (wchar_t)32;
    else if(c > 65280 && c < 65375)
        r = (wchar_t)(r - 65248);
    return r; 
}
    

// 是否要先置过滤字符为空格？ 有可能过滤掉 1.1 这种
void TextLine::IgnorePunctuation(Context& context, int& i)
{
    if(!context.ignore_punctuation)
        return;

    // std::vector<XChar> tmp_word_chars;
    // tmp_word_chars.reserve(word_chars.size());
    // for(auto& c : word_chars)
    // {
    //     if(c.text.size()== 1 && is_punctuation(context.punctuation, c.text[0]))
    //     {
    //         continue;
    //     }
    //     tmp_word_chars.push_back(c);
    // }
    // word_chars.swap(tmp_word_chars);


    std::vector<XChar> tmp_chars;
    std::wstring tmp_text;
    std::vector<int> tmp_origin_text_index; // 字符下标映射，f[a] = b，a代表过滤标点符号后的文本的某个字符，b代表该字符对应在origin_text中的下标。
    tmp_chars.reserve(chars.size());
    tmp_text.reserve(chars.size());
    tmp_origin_text_index.reserve(chars.size());
    
    for(auto& c : chars)
    {
        i++;
        if(is_punctuation(context.punctuation, c.text[0]))
        {
            continue;
        }
        tmp_origin_text_index.push_back(i);
        tmp_text.append(c.text);
        tmp_chars.push_back(c);
    }
    chars.swap(tmp_chars);
    text.swap(tmp_text);
    origin_text_index.swap(tmp_origin_text_index);
}



void TextLine::WordSegmentation()
{
    if(chars.empty())
        return;

    if(is_stamp)
    {
        XChar ch = chars[0];
        for(int i = 1; i < chars.size(); i++)
        {
            ch.merge(chars[i]);
        }

        ch.box = poly;
        ch.pos_list.resize(1);
        ch.pos_list[0] = ch.box;
        word_chars.push_back(ch);
        return;
    }


    word_chars.reserve(chars.size());

    std::wregex e (L"[a-zA-Z0-9]{2,}");
    std::regex_iterator<std::wstring::const_iterator> rit ( text.begin(), text.end(), e);
    std::regex_iterator<std::wstring::const_iterator> rend;

    int offset = 0;
    while (rit!=rend) 
    {
        int pos = rit->position();
        int len = rit->length();

        while(offset < pos)
        {
            word_chars.push_back(chars[offset++]);
        }

        XChar ch = chars[offset++];
        for(int i = 1; i < len; i++)
        {
            ch.merge(chars[offset++]);
        }

        word_chars.push_back(ch);
        ++rit;
    }

    if(offset < chars.size())
    {
        word_chars.insert(word_chars.end(), chars.begin()+offset, chars.end());
    }
}


int TextLine::init_line(my_json& js, int _page_index, int _line_index, Context& context, int& origin_char_index)
{
    page_index = _page_index;
    line_index = _line_index;

    area_type = from_utf8(js["area_type"].get<std::string>());
    area_index = js["area_index"].get<int>();

    //判断是否是印章
    {
        std::string type_str = js["type"].get<std::string>();
        if(type_str == "stamp")
        {
            is_stamp = true;
        }
    }

    std::string str = js["text"].get<std::string>();
    std::vector<int> polygon = js["position"].get<std::vector<int> >();
    

    text = from_utf8(str);
    poly = BoundBox::toBoundBox(polygon);
    height = poly.bottom - poly.top;
    width = poly.right - poly.left;

    if(text.size())
    {
        int sz = js["char_positions"].size();
        chars.reserve(sz);

        for(int idx = 0; idx < sz; idx++)
        {
            XChar c;
            c.text = text.substr(idx, 1);
            c.text[0] = toHalfWidth(c.text[0]);
            c.page_index = page_index;
            c.line_index = line_index;
            c.parse(js, idx);
            chars.push_back(c);

            char_avg_width += c.box.right - c.box.left;
        }
    }

    if(text.size())
    {
        char_avg_width /= text.size();
    }
    origin_text = text;

    // WordSegmentation();
    IgnorePunctuation(context, origin_char_index);
    return 0;
}



} // namespace xwb
