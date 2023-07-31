#include "Document.h"
#include "str_utils.h"
#include <regex>
#include <algorithm>
#include "contract_compare.h"
#include "xie_debug.h"
#include "xie_log.h"

using std::regex_match;
using std::wregex;
using std::min;
using std::max;

#ifdef CV_DEBUG
extern std::string g_root;
#endif

namespace xwb
{
    
int Document::load_pages(my_json& js, int& origin_char_index, Context& context)
{
    for(auto& page : js)
    {
        if(!page.contains("lines"))
            continue;

        int sz = page["lines"].size();
        pages.reserve(sz);
            
        Page p;
        p.load(page, origin_char_index, context);
        pages.push_back(p);

        char_num += p.GetTotalChars();
        total_page_number = std::max(p.page_index+1, total_page_number);
    }
        
    return 0;
}

/**
 * 获取文档原始全文的text拼接成的字符串
 * @param origin_text 文档的原始全文内容拼接成的字符串（包含表格印章页眉页脚的文本内容，包括忽略的标点符号）
 * @return
 */
void Document::get_orgin_text(std::wstring& origin_text)
{
    for(auto& page : pages)
    {
        for(auto& line : page.lines)
        {
            origin_text += line.origin_text;
        }
    }
}

int Document::load_pages_with_layout(my_json& js, std::wstring& origin_text, int& origin_char_index, Context& context)
{
    TIME_COST_FUNCTION;
    load_pages(js, origin_char_index, context);
    get_orgin_text(origin_text);
    layout_analyze();
    return 0;
}

void Document::layout_analyze()
{
    mark_header();
    mark_footer();
    paragraph_segment();
    semantic_seg();
}

int cal_ver_overlap(int pos1_left, int pos1_right, int pos2_left, int pos2_right)
{
    int overlap = 0;
    if (pos2_right - pos2_left != 0)
        overlap = (min(pos1_right, pos2_right) - max(pos1_left, pos2_left)) * 100 / (pos2_right - pos2_left);
    return overlap;
}
    

#define COL_SIZE 3

void Document::mark_header()
{
    std::vector<std::vector<int> > pattern_infor;
    std::vector<std::wstring> line_infor;
        
    for(auto& page : pages)
    {
        if(page.header_candi_index.empty())
            continue;
        
        for(auto& idx : page.header_candi_index)
        {
            auto& line = page.lines[idx];
            if(line.area_type == L"edge")
            {
                line.is_header = true;
            }
        }
    }
}


class FooterIdentify
{
public:
    void push(const TextLine& line)
    {
        bool find = false;
        for(auto it = candi_footers.begin(); it != candi_footers.end(); it++)
        {
            if(same_line(it->first, line))
            {
                candi_footers[it->first]++;
                find = true;
                break;
            }
        }
        if(!find)
            candi_footers[line] = 1;
        count++;
    }

    bool footer(const TextLine& line)
    {
        for(auto it = candi_footers.begin(); it != candi_footers.end(); it++)
        {
            if(same_line(it->first, line) && it->second > 2)
            {
                return true;
            }
        }
        return false;
    }

    bool same_line(const TextLine& a, const TextLine& b)
    {
        std::wstring text1 = clean_text(a.text);
        std::wstring text2 = clean_text(b.text);
        
        int threshold = min(4, max(1, min((int)text1.size(), (int)text2.size())/4));

        if(a.poly.is_same_line(b.poly) and EditDistance(text1, text2) < threshold)
        {
            return true;
        }

        return false;
    }

public:
    std::map<TextLine, int> candi_footers;
    int count = 0;
};



void Document::mark_footer()
{
    FooterIdentify footer_identify;
    for(auto& page : pages)
    {
        if(page.footer_candi_index.empty())
            continue;

        for(auto& idx: page.footer_candi_index)
        {
            auto& line = page.lines[idx];
            if(line.area_type == L"edge")
            {
                footer_identify.push(line);
            }
        }
    }

    for(auto& page : pages)
    {
        if(page.footer_candi_index.empty())
            continue;
        
        for(auto& idx : page.footer_candi_index)
        {
            auto& line = page.lines[idx];
            if(line.area_type == L"edge" || footer_identify.footer(line))
            {
                line.is_footer = true;
            }
        }
    }




    for(auto& page : pages)
    {
        if(page.footer_candi_index.empty())
            continue;
        
        for(auto& idx : page.footer_candi_index)
        {
            auto& line = page.lines[idx];
            if(line.area_type == L"edge")
            {
                line.is_footer = true;
            }
        }
    }
}

std::vector<std::vector<TextLine>> Document::generate_paragraphs(Page &page)
{
    // """
    // :param page: page content
    // :return: paragraph list of this page
    // """
    std::vector<std::vector<TextLine>> paragraphs;
    int line_idx = 0;
    std::vector<TextLine> para;

    for (auto margin : page.y_margin_list)
    {
        while (line_idx < page.lines.size())
        {
            auto &line = page.lines[line_idx];
            //# print(' line:', line.text)
            //# if re.search(u'第([一二三四五六七八九十]{1,3})条', line.text, flags=0) != None:
            //#     print(' left:', line.poly.left, ' line:', line.text)

            if (line.is_header || line.is_footer || line.form_index >= 0 || line.is_stamp || page.areas[line.area_index].type == L"image")
            {
                line_idx += 1;
                continue;
            }

            if (line.form_index >= 0)
            {
                if (para.size())
                {
                    paragraphs.push_back(para);
                    para.clear();
                }
                para.push_back(line);
                line_idx += 1;
            }
            else
            {
                // regex_search(line.text, wregex(L"^[1-9一二三四五六七八九十]+[\\u4E00-\\u9FA5]{2,}"))
                 // #plain text
                if (regex_search(line.text, wregex(L"第[一二三四五六七八九十]+[条章]"))
                || regex_search(line.text, wregex(L"^[一二三四五六七八九十]+[、,\\.]")))
                {
                    //# print(' left:', line.poly.left, ' line:', line.text)
                    if (para.size())
                    {
                        paragraphs.push_back(para);
                        para.clear();
                    }

                    para.push_back(line);
                    line_idx += 1;
                }
                else if (line.poly.bottom > margin) //:  # y-projection
                {
                    if (para.size())
                    {
                        paragraphs.push_back(para);
                        para.clear();
                    }
                    break;
                } // 字体变大了。 标题
                else if (line_idx - 1 >= 0 
                        && page.lines[line_idx - 1].is_header == false 
                        && (line.height > 1.5 * page.lines[line_idx - 1].height) 
                        && (page.lines[line_idx - 1].poly.is_same_line(line.poly) == false))
                {
                    if (para.size())
                    {
                        paragraphs.push_back(para);
                        para.clear();
                    }
                    para.push_back(line);
                    line_idx += 1;
                }
                else
                {
                    para.push_back(line);
                    line_idx += 1;
                }
            }
        }
    }

    while (line_idx < page.lines.size())
    {
        auto &line = page.lines[line_idx];
        if (line.is_header || line.is_footer || line.form_index >= 0 || line.is_stamp || page.areas[line.area_index].type == L"image")
        {
            line_idx += 1;
            continue;
        }

        if (line.form_index >= 0) // == True :  # form
        {
            if (para.size())
            {
                paragraphs.push_back(para);
                para.clear();
            }

            para.push_back(line);
            line_idx += 1;
        }
        else
        {
            if (regex_search(line.text, wregex(L"第[一二三四五六七八九十]+[条章]"))
             || regex_search(line.text, wregex(L"^[一二三四五六七八九十]+[、,\\.]")))
            {
                if (para.size())
                {
                    paragraphs.push_back(para);
                    para.clear();
                }

                para.push_back(line);
                line_idx += 1;
            }
            else
            {
                para.push_back(line);
                line_idx += 1;
            }
        }
    }
    if (para.size())
        paragraphs.push_back(para);
    
    return paragraphs;
}

void Document::paragraph_segment()
{
    for(auto& page : pages)
    {
        if(page.type == L"CATALOG")
        {
            std::vector<TextLine> lines;
            for(auto& line : page.lines)
            {
                if (line.is_header || line.is_footer || line.form_index >= 0 || line.is_stamp || page.areas[line.area_index].type == L"image")
                {
                    continue;
                }
                lines.push_back(line);
            }

            if(lines.size() > 0)
            {
                Page para;
                para.reset_area_infor(lines, L"CATALOG");
                paragraphs.push_back(para);
            }            
        }
        else
        {
            page.calculate_project_y();
            page.find_margin_y();
            std::vector<std::vector<TextLine> > para_list = generate_paragraphs(page);
            for(auto& para_lines: para_list)
            {
                Page para;
                para.reset_area_infor(para_lines, L"MAINBODY");
                paragraphs.push_back(para);

#ifdef CV_DEBUG
                {
                    char filename[256] = {0};
                    snprintf(filename, 256, "%s/visual_ocr_result/%d_%d.jpg", g_root.c_str(), doc_index, para_lines[0].page_index);
                    cv::Mat image = cv::imread(filename, 1);

                    BoundBox box = para_lines[0].poly;
                    for(auto& l : para_lines)
                    {
                        box.merge(l.poly);
                    }
                    
                    std::vector<std::vector<cv::Point> > pts;
                   std::vector<cv::Point> pt = {
                       {2*box.left, 2*box.top}, 
                       {2*box.right, 2*box.top}, 
                       {2*box.right, 2*box.bottom}, 
                       {2*box.left, 2*box.bottom}};
                    pts.push_back(pt);

                    cv::Mat painter = image.clone();
                    cv::fillPoly(painter, pts, cv::Scalar(0, 255, 0));
                    cv::addWeighted(image,0.7,painter,0.3,0,painter);

                    snprintf(filename, 256, "%s/paragraph_segment/%d_%d.jpg", g_root.c_str(), doc_index, para_lines[0].page_index);
                    cv::imwrite(filename, painter);
                }
#endif
// #ifdef LOG_DEBUG
//                 LOGD("Para>>>>>>>>>>>>>");
//                 for(auto& l : para_lines)
//                 {
//                     LOGD(to_utf8(l.text).c_str());
//                 }
//                 LOGD("Para<<<<<<<<<<<<<<");
// #endif
            }
        }
    }
}
        
void Document::semantic_seg()
{
    std::vector<Page> new_list;
    for(auto& para : paragraphs)
    {
        if(para.section_text.size() < 150)
        {
            new_list.push_back(para);
        }
        else if(para.type == L"CATALOG")
        {
            new_list.push_back(para);
        }
        else
        {
            std::vector<TextLine> new_para;
            int line_idx = 1;
            new_para.push_back(para.lines[0]);

            while(line_idx < para.lines.size())
            {
                TextLine& first_line = para.lines[line_idx-1];
                TextLine& second_line = para.lines[line_idx];
                
                if(first_line.poly.is_same_line(second_line.poly))
                {
                    new_para.push_back(second_line);
                    line_idx+=1;
                }
                else if(second_line.poly.left - first_line.poly.left < 1.5*second_line.char_avg_width 
                        && second_line.poly.top - first_line.poly.bottom < second_line.height)
                {
                    new_para.push_back(second_line);
                    line_idx += 1;
                }
                else
                {
                    Page p;
                    p.reset_area_infor(new_para, L"MAINBODY");
                    new_list.push_back(p);
                    new_para.clear();
                    new_para.push_back(second_line);

                    line_idx += 1;
                }
            }

            if(new_para.size())
            {
                Page p;
                p.reset_area_infor(new_para, L"MAINBODY");
                new_list.push_back(p);
            }      
        }
    }

    paragraphs.swap(new_list);

#ifdef CV_DEBUG

    if(paragraphs.size())
    {
        char filename[256] = {0};
        snprintf(filename, 256, "%s/visual_ocr_result/%d_%d.jpg", g_root.c_str(), doc_index, paragraphs[0].lines[0].page_index);
        char sfilename[256] = {0};
        snprintf(sfilename, 256, "%s/semantic_seg/%d_%d.jpg", g_root.c_str(), doc_index, paragraphs[0].lines[0].page_index);

        auto cvtCVPoints = [](BoundBox box){
            return std::vector<cv::Point>{
                {2*box.left, 2*box.top}, 
                {2*box.right, 2*box.top}, 
                {2*box.right, 2*box.bottom}, 
                {2*box.left, 2*box.bottom}
            };
        };


        cv::Mat image = cv::imread(filename, 1);
        cv::Mat painter = image.clone();
        std::vector<std::vector<cv::Point> > pts;
        pts.push_back(cvtCVPoints(paragraphs[0].box));

        for(int i = 1; i < paragraphs.size(); i++)
        {
            if(paragraphs[i].lines[0].page_index == paragraphs[i-1].lines[0].page_index)
            {           
                pts.push_back(cvtCVPoints(paragraphs[i].box));
            }
            else
            {
                cv::fillPoly(painter, pts, cv::Scalar(0, 255, 0));
                cv::addWeighted(image,0.7,painter,0.3,0,painter);
                cv::imwrite(sfilename, painter);

                snprintf(filename, 256, "%s/visual_ocr_result/%d_%d.jpg", g_root.c_str(), doc_index, paragraphs[i].lines[0].page_index);
                snprintf(sfilename, 256, "%s/semantic_seg/%d_%d.jpg", g_root.c_str(), doc_index, paragraphs[i].lines[0].page_index);

                image = cv::imread(filename, 1);
                painter = image.clone();
                pts.clear();
                pts.push_back(cvtCVPoints(paragraphs[i].box));
            }
        }
        cv::fillPoly(painter, pts, cv::Scalar(0, 255, 0));
        cv::addWeighted(image,0.7,painter,0.3,0,painter);
        cv::imwrite(sfilename, painter);
    }
#endif
}


} // namespace xwb

