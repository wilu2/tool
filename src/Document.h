#pragma once

#include "Page.h"
#include "nlohmann/my_json.hpp"
#include "contract_compare.h"


namespace xwb
{
    
class Document
{
public:
    Document()
    {
        total_page_number = -1;
        char_num = 0;
        doc_index = 0;
    }

    int load_pages(my_json& js, int& origin_char_index, Context& context);
    int load_pages_with_layout(my_json& js, std::wstring& origin_text, int& origin_char_index, Context& context);
    void layout_analyze();
    void mark_header();
    void mark_footer();
    std::vector<std::vector<TextLine> > generate_paragraphs(Page& page);
    void paragraph_segment();
    void semantic_seg();
    void get_orgin_text(std::wstring& origin_text);


public:
    std::vector<Page> pages;
    int total_page_number;
    std::vector<Page> paragraphs;
    int char_num;
    int doc_index;
};



} // namespace xwb
