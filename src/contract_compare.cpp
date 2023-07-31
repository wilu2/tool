#include "contract_compare.h"
#include "Document.h"
#include "str_utils.h"
#include <thread>
#include <regex>
#include "DocNode.h"
#include "xie_debug.h"
#include "zss/Node.h"
#include "spdlog/spdlog.h"
#include "zss/zss.h"
#include "diff.h"

#ifdef ENABLE_LUA
extern "C" {
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
};
#endif


using namespace std;

namespace xwb
{


void to_json(my_json& j, const XChar& c)
{
    j["text"] = to_utf8(c.text);
    j["polygon"] = c.box.to_polygon();
    j["char_polygons"] = my_json::array();
    
    if(c.pos_list.empty())
    {
        j["char_polygons"].push_back(c.box.to_polygon());
    }
    else
    {
        std::vector<BoundBox> final_boxs;
        final_boxs.push_back(c.pos_list[0]);

        for(int i = 1; i < c.pos_list.size(); i++)
        {
            if(final_boxs.back().is_same_line(c.pos_list[i]))
            {
                final_boxs.back().merge(c.pos_list[i]);
            }
            else
            {
                final_boxs.push_back(c.pos_list[i]);
            }
        }
        
        for(auto& pos : final_boxs)
        {
            my_json tmp_pos_j = pos.to_polygon();
            j["char_polygons"].push_back(tmp_pos_j); 
        }   
    }
    
    j["char_candidates"] = my_json::array();
    j["page_index"] = c.page_index;
    j["line_index"] = c.line_index;
    j["doc_index"] = 1;
}




int doc_compare(std::vector<Page>& doc1, std::vector<Page>& doc2, 
                std::vector<vector<XChar> >& return_list1, std::vector<vector<XChar> >& return_list2,
                      CharCounter& char_counter);

int fixFoundPath(vector<XChar>& return_list1, vector<XChar>& return_list2)
{
    if(return_list1.size() < 3)
        return -1;


    for(int j = return_list1.size()-1; j > 0; j--)
    {
        CharStatus char_status;

        if(return_list1[j].text == return_list2[j].text
        && return_list1[j].char_status == NORMAL
        && return_list2[j].char_status == NORMAL
        && return_list1[j-1].char_status == return_list2[j-1].char_status
        && (return_list1[j-1].char_status == INSERT || return_list1[j-1].char_status == DELETE))
        {
            char_status = return_list1[j-1].char_status;

            int k = j-2;

            int n1 = return_list1[j-1].text.empty() ? 1: 0;
            int n2 = return_list2[j-1].text.empty() ? 1: 0;

            while(k >= 0 
                && return_list1[k].char_status == char_status 
                && return_list2[k].char_status == char_status)
            {
                if(return_list1[k].text.empty())
                    n1++;
                if(return_list2[k].text.empty())
                    n2++;

                k--;
            }

            k++;

            if(n1 < n2 && return_list1[k].text == return_list1[j].text)
            {
                return_list2[k] = return_list2[j];
                return_list2[j] = return_list2[j-1];
                return_list2[j].char_status = char_status;

                return_list1[k].char_status = NORMAL;
                return_list1[j].char_status = char_status;
            }
            else if(n1 > n2 && return_list2[k].text == return_list2[j].text)
            {
                return_list1[k] = return_list1[j];
                return_list1[j] = return_list1[j-1];
                return_list1[j].char_status = char_status;

                return_list2[k].char_status = NORMAL;
                return_list2[j].char_status = char_status;
            }

        }
    }

    return 0;
}


int combineSmallDiff(vector<XChar>& return_list1, vector<XChar>& return_list2)
{
    if(return_list1.size() < 3)
        return -1;

    auto is_same_line = [](XChar& target, XChar& cur){
        return (cur.page_index == target.page_index && cur.line_index == target.line_index) 
               || min(cur.page_index, target.page_index) == -1;
    };

    int sz = (int)return_list1.size() - 1;

    for(int j = 1; j < sz; j++)
    {
        if(return_list1[j].char_status != NORMAL || return_list1[j-1].char_status == NORMAL)
            continue;

        if(return_list1[j-1].char_status == CHANGE
          && return_list1[j+1].char_status != NORMAL
          && is_same_line(return_list1[j], return_list1[j-1])
          && is_same_line(return_list2[j], return_list2[j-1]))

        {
            return_list1[j].char_status = CHANGE;
            return_list2[j].char_status = CHANGE;
            j+=2;
        }
        else if(return_list1[j-1].char_status != NORMAL
               && return_list1[j+1].char_status == CHANGE
               && is_same_line(return_list1[j], return_list1[j+1])
               && is_same_line(return_list2[j], return_list2[j+1]))
        {
            return_list1[j].char_status = CHANGE;
            return_list2[j].char_status = CHANGE;
            j+=2;
        }
        else if(return_list1[j+1].char_status == NORMAL && j+2 <= sz
               && is_same_line(return_list1[j], return_list1[j+1])
               && is_same_line(return_list2[j], return_list2[j+1]))
        {
            if(return_list1[j-1].char_status == CHANGE
            && return_list1[j+2].char_status != NORMAL
            && is_same_line(return_list1[j], return_list1[j-1])
            && is_same_line(return_list2[j], return_list2[j-1]))
            {
                return_list1[j].char_status = CHANGE;
                return_list1[j+1].char_status = CHANGE;
                return_list2[j].char_status = CHANGE;
                return_list2[j+1].char_status = CHANGE;
                j +=3;
            }
            else if(return_list1[j-1].char_status != NORMAL
            && return_list1[j+2].char_status == CHANGE
            && is_same_line(return_list1[j], return_list1[j+2])
            && is_same_line(return_list2[j], return_list2[j+2]))
            {
                return_list1[j].char_status = CHANGE;
                return_list1[j+1].char_status = CHANGE;
                return_list2[j].char_status = CHANGE;
                return_list2[j+1].char_status = CHANGE;
                j +=3;
            }
        }
    }

    return 0;
}


int find_path(vector<vector<int> >& score, 
              vector<XChar>& char_list1, vector<XChar>& char_list2,
              vector<XChar>& return_list1, vector<XChar>& return_list2, int& similarity)
{
    // wstring align1;
    // wstring align2;
    int j = char_list1.size();
    int i = char_list2.size();
    // std::vector<XChar> diff_seq1;
    // std::vector<XChar> diff_seq2;
    // std::vector<XChar> insert_chars;
    // std::vector<XChar> delete_chars;
    similarity = 0;
    while(i > 0 && j > 0)
    {
        int score_current = score[i][j];
        int score_diag = score[i-1][j-1];
        int score_top = score[i][j-1];
        int score_left = score[i-1][j];

        int s = -1;
        if(char_list1[j - 1].text == char_list2[i - 1].text)
        {
            s = 1;
        }
            
        if(score_current == score_diag + s)
        {
            // align1 += seq1[j-1];
            // align2 += seq2[i-1];

            if(s != 1 /*&& (punctuation.find(seq1[j - 1]) == wstring::npos ||  punctuation.find(seq2[i - 1]) == wstring::npos)*/)// #change
            {
                char_list1[j - 1].char_status = CHANGE;
                char_list2[i - 1].char_status = CHANGE;
                // diff_seq1.push_back(char_list1[j - 1]);
                // diff_seq2.push_back(char_list2[i - 1]);
            }
            else  //#same
            {
                similarity += 1;
            }
                
            return_list1.push_back(char_list1[j - 1]);
            return_list2.push_back(char_list2[i - 1]);
            i -= 1;
            j -= 1;
        }
        else if(score_current == score_top + s) // delete
        {
            // align1 += seq1[j-1];
            // align2 += L'_';
            //if(punctuation.find(seq1[j - 1]) == wstring::npos)
            {
                char_list1[j - 1].char_status = DELETE;
                //delete_chars.push_back(char_list1[j - 1]);
                return_list1.push_back(char_list1[j - 1]);

                XChar del_char;
                del_char.char_status = DELETE;
                del_char.box.assign(-1, -1, -1, -1);
                del_char.page_index = -1;
                del_char.line_index = -1;
                return_list2.push_back(del_char);
            }
            j -= 1;
        }
        else if(score_current == score_left +s) // insert
        {
            // align1 += '_';
            // align2 += seq2[i - 1];
            //if(punctuation.find(seq2[i - 1]) == wstring::npos)
            {
                XChar add_char;
                add_char.char_status = INSERT;
                add_char.box.assign(-1, -1, -1, -1);
                add_char.page_index = -1;
                add_char.line_index = -1;
                return_list1.push_back(add_char);

                char_list2[i - 1].char_status = INSERT;
                //insert_chars.push_back(char_list2[i - 1]);
                return_list2.push_back(char_list2[i - 1]);
            }
            i -= 1;
        }
    }

    while(j > 0) // # delete
    {
        // align1 += seq1[j-1];
        // align2 += '_';
        //# delete.push_back(seq1[j - 1])
        //if(punctuation.find(seq1[j - 1]) == wstring::npos)
        {
            char_list1[j - 1].char_status = DELETE;
            //delete_chars.push_back(char_list1[j - 1]);
            return_list1.push_back(char_list1[j - 1]);

            XChar del_char;
            del_char.char_status = DELETE;
            del_char.box.assign(-1, -1, -1, -1);
            del_char.page_index = -1;
            del_char.line_index = -1;
            return_list2.push_back(del_char);
        }
        j -= 1;
    }

    while(i > 0) //: # insert
    {
        // align1 += L'_';
        // align2 += seq2[i - 1];
        //# insert.push_back(seq2[i - 1])
        //if(punctuation.find(seq2[i - 1]) == wstring::npos)
        {
            XChar add_char;
            add_char.char_status = INSERT;
            add_char.box.assign(-1, -1, -1, -1);
            add_char.page_index = -1;
            add_char.line_index = -1;
            return_list1.push_back(add_char);

            char_list2[i - 1].char_status = INSERT;
            return_list2.push_back(char_list2[i - 1]);
            //insert_chars.push_back(char_list2[i - 1]);
            //# print('INSERT:', add_char.text,' ',add_char.char_status, '-', char_list2[i - 1].text)
        }
        i -= 1;
    }

    // std::reverse(align1.begin(), align1.end());
    // std::reverse(align2.begin(), align2.end());
    // std::reverse(diff_seq1.begin(), diff_seq1.end());
    // std::reverse(diff_seq2.begin(), diff_seq2.end());
    // std::reverse(delete_chars.begin(), delete_chars.end());
    // std::reverse(insert_chars.begin(), insert_chars.end());
    std::reverse(return_list1.begin(), return_list1.end());
    std::reverse(return_list2.begin(), return_list2.end());
    fixFoundPath(return_list1, return_list2);

    return 0;
}


int needleman_wunsch(vector<XChar>& char_list1, vector<XChar>& char_list2,
                     vector<XChar>& return_list1, vector<XChar>& return_list2, int& similarity)
{   
    int n = char_list1.size() + 1;
    int m = char_list2.size() + 1;

    vector<vector<int> > scores(m, vector<int>(n, 0));

    for(int i = 0; i < m; i++)
    {
        scores[i][0] = -i;
    }
    
    for(int j = 0; j < n; j++)
    {
        scores[0][j] = -j;
    }
    
    for(int i = 1; i < m; i++)
    {
        for(int j = 1; j < n; j++)
        {
            if(char_list1[j-1].text == char_list2[i-1].text)
            {
                scores[i][j] = scores[i-1][j-1]+1;
                //scores[i][j] = max(scores[i-1][j-1]+1, max(scores[i-1][j] -1, scores[i][j-1]-1));
            }
            else
            {
                scores[i][j] = max(scores[i-1][j-1]-1, max(scores[i-1][j] -1, scores[i][j-1]-1));
            }
        }
    }

    find_path(scores, char_list1, char_list2, return_list1, return_list2, similarity);

    return 0;
}




int text_compare(wstring& para1, wstring& para2, 
                 vector<XChar>& char_list1, vector<XChar>& char_list2,
                 vector<XChar>& return_list1, vector<XChar>& return_list2, CharCounter& char_counter)
{
    int sz = max(para1.size(), para2.size());

    wstring str1 = para1;
    wstring str2 = para2;
    return_list1.reserve(sz);
    return_list2.reserve(sz);

    int similarity = 0;
    
    if(char_list1.empty() && char_list2.empty())
    {
        return 0;
    }
    else if(char_list1.size() && char_list2.size())
    {   
        //needleman_wunsch(char_list1, char_list2, return_list1, return_list2, similarity);

        diff_main_call(char_list1, char_list2, return_list1, return_list2, similarity);

        combineSmallDiff(return_list1, return_list2);
    }
    else if(char_list1.empty())
    {
        for(auto& c : char_list2)
        {
            XChar add_char;
            add_char.char_status = INSERT;
            add_char.box.assign(-1, -1, -1, -1);
            return_list1.push_back(std::move(add_char));

            c.char_status = INSERT;
            return_list2.push_back(c);
        }
    }
    else if(char_list2.empty())
    {
        for(auto& c : char_list1)
        {
            c.char_status = DELETE;
            return_list1.push_back(c);

            XChar del_char;
            del_char.char_status = DELETE;
            del_char.box.assign(-1, -1, -1, -1);
            return_list2.push_back(std::move(del_char));
        };
    }


    char_counter.update(char_list1.size(), char_list2.size(), similarity);
    return 0;
}


int mergeCharsCrossLine(vector<XChar>& seq1, vector<XChar>& seq2)
{
    auto can_merge = [](XChar& prev, XChar& curr){
        if(prev.text.empty() || curr.text.empty())
            return true;

        // stamp
        if(prev.text.size() > 3 && (prev.box.right-prev.box.left) < (prev.box.bottom-prev.box.top)* 1.5)
            return false;

        // stamp
        if(curr.text.size() > 3 && (curr.box.right-curr.box.left) < (curr.box.bottom-curr.box.top) * 1.5)
            return false;

        return curr.box.top - prev.box.bottom < (curr.box.bottom-curr.box.top)*1.5;
    };



    // 跨行合并

    int index_1 = (int)seq1.size() - 1;
    int index_2 = (int)seq2.size() - 1;
    while(index_1 > 0 && index_2 > 0)
    {
        XChar& char1_first = seq1[index_1-1];
        XChar& char1_second = seq1[index_1];
        XChar& char2_first = seq2[index_2-1];
        XChar& char2_second = seq2[index_2];

        if ((char1_first.line_index != char1_second.line_index || char2_first.line_index != char2_first.line_index)
            && char1_first.char_status != NORMAL && char1_second.char_status != NORMAL
            && can_merge(char1_first, char1_second)
            && can_merge(char2_first, char2_second))
        {
            char1_first.merge(char1_second);
            char1_first.page_index = max(char1_first.page_index, char1_second.page_index);
            char1_first.line_index = max(char1_first.page_index, char1_second.line_index);
            char1_first.char_status = CHANGE;
            seq1.erase(seq1.begin() + index_1);
            
            char2_first.merge(char2_second);
            char2_first.char_status = CHANGE;
            char2_first.page_index = max(char2_first.page_index, char2_second.page_index);
            char2_first.line_index = max(char2_first.page_index, char2_second.line_index);
            seq2.erase(seq2.begin() + index_2);
        }

        index_1 --;
        index_2 --;
    }

    return 0;
}


int mergeCharsSameTypeSameLine(vector<XChar>& seq1, vector<XChar>& seq2)
{
    int index_1 = (int)seq1.size() - 1;
    int index_2 = (int)seq2.size() - 1;

    while(index_1 > 0 && index_2 > 0)
    {
        XChar& char1_first =seq1[index_1-1];
        XChar& char1_second =seq1[index_1];
        XChar& char2_first =seq2[index_2-1];
        XChar& char2_second =seq2[index_2];

        if (char1_first.char_status == char1_second.char_status 
            && char2_first.char_status == char2_second.char_status
            && char1_first.char_status == char2_first.char_status
            && char1_first.page_index == char1_second.page_index
            && char1_first.line_index == char1_second.line_index
            && char2_first.page_index == char2_second.page_index
            && char2_first.line_index == char2_second.line_index)
        {
            char1_first.merge(char1_second);
            seq1.erase(seq1.begin() + index_1);
            
            char2_first.merge(char2_second);
            seq2.erase(seq2.begin() + index_2);
        }
        index_1 --;
        index_2 --;
    }

    while(index_1 > 0)
    {   
        XChar& char1_first = seq1[index_1 - 1];
        XChar& char1_second = seq1[index_1];
        if(char1_first.page_index == char1_second.page_index && char1_first.line_index == char1_second.line_index)
        {
            char1_first.merge(char1_second);
            seq1.erase(seq1.begin() + index_1);
        }
        index_1 -= 1;
    }

    while(index_2 > 0)
    {
        XChar& char2_first = seq2[index_2 - 1];
        XChar& char2_second = seq2[index_2];
        if(char2_first.page_index == char2_second.page_index and char2_first.line_index == char2_second.line_index)
        {   
            char2_first.merge(char2_second);
            seq2.erase(seq2.begin()+index_2);
        }
            
        index_2 -= 1;
    }
    return 0; 
}


int mergeCharsDifTypeSameLine(vector<XChar>& seq1, vector<XChar>& seq2)
{
        // 合并非同类差异。如下所示，会有两个差异。 张三和李四为修改，五为新增。统一改为修改。
    // 姓名:张三
    // 姓名:李四五
    int index_1 = (int)seq1.size() - 1;
    int index_2 = (int)seq2.size() - 1;
    while(index_1 > 0 && index_2 > 0)
    {
        XChar& char1_first = seq1[index_1-1];
        XChar& char1_second = seq1[index_1];
        XChar& char2_first = seq2[index_2-1];
        XChar& char2_second = seq2[index_2];

        if (char1_first.char_status != char1_second.char_status
            && max(char1_first.char_status, char1_second.char_status) == CHANGE
            && min(char1_first.char_status, char1_second.char_status) != NORMAL
            && ((char1_first.page_index == char1_second.page_index && char1_first.line_index == char1_second.line_index)
                || (char2_first.page_index == char2_second.page_index && char2_first.line_index == char2_second.line_index)))
        {
            char1_first.merge(char1_second);
            char1_first.page_index = max(char1_first.page_index, char1_second.page_index);
            char1_first.line_index = max(char1_first.page_index, char1_second.line_index);
            char1_first.char_status = CHANGE;
            seq1.erase(seq1.begin() + index_1);
            
            char2_first.merge(char2_second);
            char2_first.char_status = CHANGE;
            char2_first.page_index = max(char2_first.page_index, char2_second.page_index);
            char2_first.line_index = max(char2_first.page_index, char2_second.line_index);
            seq2.erase(seq2.begin() + index_2);
        }

        index_1 --;
        index_2 --;
    }

    return 0;
}

int merge_char_list(vector<XChar>& seq1, vector<XChar>& seq2)
{
    mergeCharsSameTypeSameLine(seq1, seq2);
    mergeCharsDifTypeSameLine(seq1, seq2);

    mergeCharsCrossLine(seq1, seq2);
    return 0;
}


int textlines_compare(std::vector<TextLine>& lines1, 
                      std::vector<TextLine>& lines2, 
                      vector<XChar>& return_list1, 
                      vector<XChar>& return_list2,
                      CharCounter& char_counter)
{
    auto join_lines = [](std::vector<TextLine>& lines, wstring& text, std::vector<XChar>& char_list){
        if(lines.empty())
            return;
        int sz = 0;
        for(auto& line: lines)
        {
            sz += line.text.size();
        }

        text.reserve(sz+1);
        char_list.reserve(sz+1);

        for(auto& line : lines)
        {
            text.append(line.text);
            //char_list.insert(char_list.end(), line.word_chars.begin(), line.word_chars.end());
            char_list.insert(char_list.end(), line.chars.begin(), line.chars.end());
        }
    };


    wstring text1;
    std::vector<XChar> char_list1;

    wstring text2;
    std::vector<XChar> char_list2;

    join_lines(lines1, text1, char_list1);
    join_lines(lines2, text2, char_list2);

    text_compare(text1, text2, char_list1, char_list2, return_list1, return_list2, char_counter);
    merge_char_list(return_list1, return_list2);
    
    return 0;
}


int cellTextlinesCompare(std::vector<TextLine>& lines1, 
                      std::vector<TextLine>& lines2, 
                      vector<XChar>& return_list1, 
                      vector<XChar>& return_list2,
                      CharCounter& char_counter)
{
    auto join_lines = [](std::vector<TextLine>& lines, wstring& text, std::vector<XChar>& char_list){
        if(lines.empty())
            return;
        int sz = 0;
        for(auto& line: lines)
        {
            sz += line.text.size();
        }

        text.reserve(sz+1);
        char_list.reserve(sz+1);

        for(auto& line : lines)
        {
            text.append(line.text);
            //char_list.insert(char_list.end(), line.word_chars.begin(), line.word_chars.end());
            char_list.insert(char_list.end(), line.chars.begin(), line.chars.end());
        }
    };


    wstring text1;
    std::vector<XChar> char_list1;

    wstring text2;
    std::vector<XChar> char_list2;

    join_lines(lines1, text1, char_list1);
    join_lines(lines2, text2, char_list2);

    text_compare(text1, text2, char_list1, char_list2, return_list1, return_list2, char_counter);
    merge_char_list(return_list1, return_list2);
    
    return 0;
}

int paragraph_compare(std::vector<Page>& doc1, std::vector<Page>& doc2, 
                      vector<XChar>& return_list1, vector<XChar>& return_list2,
                      CharCounter& char_counter)
{
    TIME_COST_FUNCTION;
    auto join_pages = [](const std::vector<Page>& doc, wstring& text, std::vector<XChar>& char_list){
        int sz = 0;
        for(auto& para: doc)
        {
            sz += para.section_char_list.size();
        }

        text.reserve(sz+1);
        char_list.reserve(sz+1);

        for(auto& para : doc)
        {
            text.append(para.section_text);
            char_list.insert(char_list.end(), para.section_char_list.begin(), para.section_char_list.end());
        }
    };


    wstring text1;
    std::vector<XChar> char_list1;

    wstring text2;
    std::vector<XChar> char_list2;

    join_pages(doc1, text1, char_list1);
    join_pages(doc2, text2, char_list2);

    //cout << to_utf8(text1) << endl;
    //cout << to_utf8(text2) << endl;
    text_compare(text1, text2, char_list1, char_list2, return_list1, return_list2, char_counter);
    merge_char_list(return_list1, return_list2);
    
    return 0;
}


int doc_compare(std::vector<Page>& doc1, std::vector<Page>& doc2, 
                std::vector<vector<XChar> >& return_list1, std::vector<vector<XChar> >& return_list2,
                      CharCounter& char_counter)
{

    return 0;
}

int alignParagraph(Document& doc1, Document& doc2, std::vector<std::pair<int, int> >& match_pairs)
{
    int m = doc1.paragraphs.size() + 1;
    int n = doc2.paragraphs.size() + 1;

    if(m < 1 || n < 1)
        return 0;


    vector<vector<int> > scores(m, vector<int>(n, 0));

    int total_dis = 0;
    for(int i = 1; i < m; i++)
    {
        scores[i][0] = total_dis + doc1.paragraphs[i-1].section_text.size();
        total_dis += doc1.paragraphs[i-1].section_text.size();
    }
    total_dis = 0;
    for(int j = 1; j < n; j++)
    {
        scores[0][j] = total_dis + doc2.paragraphs[j-1].section_text.size();
        total_dis += doc2.paragraphs[j-1].section_text.size();
    }
    
    for(int i = 0; i < m-1; i++)
    {
        for(int j = 0; j < n-1; j++)
        {
            int dis = EditDistance(doc1.paragraphs[i].section_text, doc2.paragraphs[j].section_text);

            scores[i+1][j+1] = min(scores[i][j]+dis , 
                                   min(scores[i][j+1] + (int)doc1.paragraphs[i].section_text.size(), 
                                       scores[i+1][j] + (int)doc2.paragraphs[j].section_text.size()));
        }
    }


    int i = m-1;
    int j = n-1;

    while(i > 0 && j > 0)
    {
        int dis = EditDistance(doc1.paragraphs[i-1].section_text, doc2.paragraphs[j-1].section_text);

        if(scores[i][j] == scores[i-1][j-1] + dis)
        {
            match_pairs.emplace_back(i-1, j-1);
            i--;
            j--;
        }
        else if(scores[i][j] == scores[i-1][j] + (int)doc1.paragraphs[i-1].section_text.size())
        {
            i--;
        }
        else if(scores[i][j] == scores[i][j-1] + (int)doc2.paragraphs[j-1].section_text.size())
        {
            j--;
        }
    }

    if(match_pairs.empty())
        return 0;




    std::reverse(match_pairs.begin(), match_pairs.end());

    if(match_pairs.back().first == doc1.paragraphs.size() || match_pairs.back().second == doc2.paragraphs.size())
    {
        match_pairs.back().first = doc1.paragraphs.size();
        match_pairs.back().second = doc2.paragraphs.size();
    }
    else
    {
        match_pairs.emplace_back(doc1.paragraphs.size(), doc2.paragraphs.size());
    }

    if(match_pairs[0].first == 0 || match_pairs[0].second == 0)
    {
        match_pairs[0].first = 0;
        match_pairs[0].second = 0;
    }
    else
    {
        match_pairs.insert(match_pairs.begin(), std::pair<int, int>(0, 0));
    }

    return 0;
}


struct CompareItem{
    wstring caption;
    wstring assist;
    wstring assist_after;
};

int compare_text(Document& doc1, 
                             Document& doc2, 
                             std::vector<std::vector<XChar> >& resutl1, 
                             std::vector<std::vector<XChar> >& resutl2,
                             CharCounter& char_counter, std::wstring& origin_text1, std::wstring& origin_text2, Context& context)
{
    TIME_COST_FUNCTION;
    auto join_pages = [](const std::vector<Page>& doc, wstring& text, std::vector<XChar>& char_list, std::vector<int>& origin_text_index) {
        int sz = 0;
        for(auto& para: doc)
        {
            sz += para.section_char_list.size();
        }

        text.reserve(sz+1);
        char_list.reserve(sz+1);
        origin_text_index.reserve(sz+1); 

        for(auto& para : doc)
        {
            text.append(para.section_text);
            char_list.insert(char_list.end(), para.section_char_list.begin(), para.section_char_list.end());
            origin_text_index.insert(origin_text_index.end(), para.section_origin_text_index.begin(), para.section_origin_text_index.end());
        }
    };


    wstring text1;
    std::vector<XChar> char_list1;
    std::vector<int> origin_text_index1;

    wstring text2;
    std::vector<XChar> char_list2;
    std::vector<int> origin_text_index2;

    join_pages(doc1.paragraphs, text1, char_list1, origin_text_index1);
    join_pages(doc2.paragraphs, text2, char_list2, origin_text_index2);


    diff_main_myers(text1, text2, char_list1, char_list2, resutl1, resutl2, char_counter, origin_text_index1, origin_text_index2, origin_text1, origin_text2, context.merge_diff);
    
    return 0;
}


Node<DocNode>* generateTableTree(Document& doc, vector<Node<DocNode>*>& tables)
{
    TIME_COST_FUNCTION;
    DocNode* doc_root_node = new DocNode(DOC);
    Node<DocNode>* root_node = new Node<DocNode>(doc_root_node);

    for(auto& page: doc.pages)
    {
        for(auto& table : page.tables)
        {
            DocNode* doc_table_node = new DocNode(TABLE);
            doc_table_node->page_index = page.page_index;
            doc_table_node->table_index = table.table_index;
            Node<DocNode>* table_node = new Node<DocNode>(doc_table_node);

            for(int i = 0; i < table.table_rows; i++)
            {
                DocNode *doc_row_node = new DocNode(ROW);
                doc_row_node->page_index = page.page_index;
                doc_row_node->table_index = table.table_index;
                doc_row_node->row_index = i;

                Node<DocNode>* row_node = new Node<DocNode>(doc_row_node);

                for(int j = 0; j < table.table_cols; j++)
                {
                    for(auto& cell : table.table_cells)
                    {
                        if(cell.location[0] == i && cell.location[2] == j)
                        {   
                            DocNode* doc_cell_node = new DocNode(CELL);
                            doc_cell_node->page_index = page.page_index;
                            doc_cell_node->table_index = table.table_index;
                            doc_cell_node->row_index = i;
                            doc_cell_node->cell = &cell;

                            Node<DocNode>* node_cell = new Node<DocNode>(doc_cell_node);

                            row_node->addChild(node_cell);
                        }
                    }
                }

                table_node->addChild(row_node);
                root_node->addChild(row_node);
            }
            tables.push_back(table_node);
            // 树中不再行表格级别节点
            // 表格级别也返回，做为表格删除或者新增的判断。
            //root_node->addChild(table_node);
        }
    }
    //cout << "root" << endl; 
    return root_node;
}


vector<Node<DocNode>* > postTreeNodes(Node<DocNode>* t)
{
    vector<Node<DocNode>* > result;
    stack<Node<DocNode>* > stc;
    stc.push(t);

    while(stc.size())
    {
        Node<DocNode>* anc = stc.top(); stc.pop();
        
        for(auto& n : anc->children)
            stc.push(n);
        result.push_back(anc);
    }

    std::reverse(result.begin(), result.end());
    return result;
}

void setVisitedFlag(Node<DocNode>* root)
{
    if(root == nullptr)
        return;

    root->visited = true;

    for(auto sub_node : root->children)
    {
        setVisitedFlag(sub_node);
    }
}


void updatePageAndLineIndex(Document& doc1, 
                            Document& doc2,
                            const std::vector<std::vector<XChar> >& result1, 
                            const std::vector<std::vector<XChar> >& result2, 
                            const XChar& xch1, XChar& xch2)
{
    int max_dis = INT_MAX;

    bool left_flag = false;
    int best_i = 0;
    int best_j = 0;

    for(int i = 0; i < result1.size(); i++)
    {
        if(result1[i][0].char_status != xwb::NORMAL)
        {
            continue;
        }

        for(int j = 0; j < result1[i].size(); j++)
        {
            int line_idx_dis = 0;
            bool cur_line_left_flag = false;

            if(result1[i][j].page_index == xch1.page_index)
            {
                if(result1[i][j].line_index < xch1.line_index)
                {
                    line_idx_dis = xch1.line_index - result1[i][j].line_index;
                    cur_line_left_flag = false;
                }
                else
                {
                    line_idx_dis = result1[i][j].line_index - xch1.line_index;
                    cur_line_left_flag = true;
                }
            }
            else if(result1[i][j].page_index < xch1.page_index)
            {
                line_idx_dis += (doc1.pages[result1[i][j].page_index].lines.size() - result1[i][j].line_index);
                for(int p = result1[i][j].page_index+1; p < xch1.page_index; p++)
                {
                    line_idx_dis += doc1.pages[p].lines.size();
                }
                line_idx_dis += xch1.line_index;
                cur_line_left_flag = false;
            }
            else if(result1[i][j].page_index > xch1.page_index)
            {
                line_idx_dis += (doc1.pages[xch1.page_index].lines.size() - xch1.line_index);
                for(int p = xch1.page_index+1; p < result1[i][j].page_index; p++)
                {
                    line_idx_dis += doc1.pages[p].lines.size();
                }
                line_idx_dis += result1[i][j].line_index;
                cur_line_left_flag = true;
            }

            if(line_idx_dis < max_dis)
            {
                max_dis = line_idx_dis;
                best_i = i;
                best_j = j;
                left_flag = cur_line_left_flag;
            }
        }
    }

    if(result1.size())
    {
        int char_idx = 0;
        for(int j = 0; j < best_j; j++)
        {
            char_idx += result1[best_i][j].pos_list.size();
        }
        if(!left_flag)
        {
            char_idx += result1[best_i][best_j].pos_list.size() - 1;
        }

        for(int j = 0; j < result2[best_i].size(); j++)
        {
            if(result2[best_i][j].pos_list.size() > char_idx)
            {
                auto& target = result2[best_i][j];
                xch2.page_index = target.page_index;
                xch2.line_index = target.line_index;
                xch2.box.left = target.box.left;
                xch2.box.right = target.box.right;
                xch2.box.top = target.box.top;
                xch2.box.bottom = target.box.bottom;
                break;
            }
            else
            {
                char_idx -= result2[best_i][j].pos_list.size();
            }
        }


        if(left_flag)
        {
            xch2.box.right = xch2.box.left+1;
        }
        else
        {
            xch2.box.left = xch2.box.right-1;
        }
    }
}



void updatePageAndLineIndex_backup(const std::vector<std::vector<XChar> >& result1, 
                         const std::vector<std::vector<XChar> >& result2, 
                         const XChar& xch1, XChar& xch2)
{
    int target_index = -1;
    bool find_flag = false;
    for(int i = 0; i < result1.size(); i++)
    {
        for(int j = 0; j < result1[i].size(); j++)
        {
            if(result1[i][j].page_index == xch1.page_index && result1[i][j].line_index > xch1.line_index
              || result1[i][j].page_index > xch1.page_index)
            {
                if(j > 0)
                {
                    j--;
                }
                else if( j== 0 && i > 0)
                {
                    i--;
                    j = (int)result1[i].size()-1;
                }
                target_index = i;
                find_flag = true;
                break;
            }
            else
            {
                target_index = max(target_index, i);
            }
        }

        if(find_flag)
        {
            break;
        }
    }

    if(target_index >=0&& result2[target_index][0].page_index >= 0 && result2[target_index][0].line_index >= 0)
    {
        auto& target = result2[target_index][0];
        xch2.page_index = target.page_index;
        xch2.line_index = target.line_index;
        xch2.box.left = target.box.right+1;
        xch2.box.right = xch2.box.left;
        xch2.box.top = target.box.top;
        xch2.box.bottom = target.box.bottom;
    }
}


XChar textline2XChar(const TextLine& line)
{
    XChar xch;
    xch.text = line.origin_text; // 返回原始文本内容（未过滤标点）
    xch.box = line.poly;
    xch.page_index = line.page_index;
    xch.line_index = line.line_index;
    return xch;
}


int table2XChar(Table& table, vector<int>& pos, vector<XChar>& xchars)
{
    int char_count = 0;
    for(auto& cell : table.table_cells)
    {
        for(auto& l : cell.lines)
        {
            xchars.push_back(textline2XChar(l));
            char_count += l.chars.size();
        }
    }
    return char_count;
}


int row2XChars(Table& table, int row_index, vector<XChar>& chars, XChar& area)
{
    int char_count = 0;

    for(auto& cell: table.table_cells)
    {
        if(cell.location[0] == row_index)
        {
            if(area.box.left == -1 && area.box.top == -1)
            {
                area.box = BoundBox::toBoundBox(cell.position);
            }
            else
            {
                area.box.merge(BoundBox::toBoundBox(cell.position));
            }

            for(auto& l : cell.lines)
            {
                if(area.page_index == -1)
                {
                    area.page_index = l.page_index;
                    area.line_index = l.line_index;
                }
                
                chars.push_back(textline2XChar(l));
                char_count += l.chars.size();
            }
        }
    }

    return char_count;
}


int cell2XChars(Node<DocNode>* node, vector<XChar>& chars, XChar& area)
{
    int char_count = 0;

    area.page_index = node->nodeData->page_index;
    area.box = BoundBox::toBoundBox(node->nodeData->cell->position);

    for(auto& l : node->nodeData->cell->lines)
    {
        area.line_index = std::max(area.line_index, l.line_index);
        chars.push_back(textline2XChar(l));
        char_count += l.chars.size();
    }
    
    return char_count;
}

void to_json(my_json& j, const vector<XChar>& chars)
{
    for(const auto& a : chars)
    {
        j.push_back(a);
    }
}


int DeleteInsertTable2XChar(Document& doc1, 
                            Document& doc2, 
                            TEDOp<DocNode>& op,
                            std::vector<std::vector<XChar> >& resutl1,
                            std::vector<std::vector<XChar> >& resutl2,
                            CharCounter& char_counter,
                            my_json& detail_result)
{
    bool deleteFlag = op.arg1 ? true : false;

    vector<XChar> chars1;
    vector<XChar> chars2;

    XChar area1;
    XChar area2;

    int word_count=0;
    if(deleteFlag)
    {
        int page_index = op.arg1->nodeData->page_index;
        int table_index = op.arg1->nodeData->table_index;
        auto& tbl = doc1.pages[page_index].tables[table_index];
        auto& pos = doc1.pages[page_index].areas[tbl.area_index].position;

        word_count = table2XChar(tbl, pos, chars1);

        if(chars1.empty())
            return 0;

        area1.page_index = page_index;
        area1.line_index = chars1[0].line_index;
        area1.box = BoundBox::toBoundBox(pos);

        updatePageAndLineIndex(doc1, doc2, resutl1, resutl2, area1, area2);
        char_counter.update(word_count, 0, 0);

        chars2.push_back(area2);
    }
    else
    {
        int page_index = op.arg2->nodeData->page_index;
        int table_index = op.arg2->nodeData->table_index;
        auto& tbl = doc2.pages[page_index].tables[table_index];
        auto& pos = doc2.pages[page_index].areas[tbl.area_index].position;
        word_count = table2XChar(tbl, pos, chars2);

        if(chars2.empty())
            return 0;

        area2.page_index = page_index;
        area2.line_index = chars2[0].line_index;
        area2.box = BoundBox::toBoundBox(pos);

        updatePageAndLineIndex(doc2, doc1, resutl2, resutl1, area2, area1);
        char_counter.update(0, word_count, 0);

        chars1.push_back(area1);
    }

    if(word_count)
    {
        my_json result;
        result["status"] = deleteFlag ? "DELETE" : "INSERT";
        result["type"] = "table";
        result["area"] = my_json::array();

        result["area"].push_back(area1);
        result["area"].push_back(area2);
        result["area"][1]["doc_index"] = 2;
        
        result["diff"] = my_json::array();

        my_json sub_result;
        sub_result["status"] = result["status"];
        sub_result["type"] = "text";
        sub_result["diff"] = my_json::array();
        sub_result["diff"].push_back(chars1);
        sub_result["diff"].push_back(chars2);
        for(auto& s : sub_result["diff"][1])
        {
            s["doc_index"] = 2;
        }

        result["diff"].push_back(sub_result);
        detail_result.push_back(result);
    }
    return 0;
}




bool isNodeMatch(TEDOp<DocNode>& op, DocNodeType type)
{
    if(op.arg1 && op.arg2 && op.arg1->nodeData->ntype == type && op.arg2->nodeData->ntype == type)
        return true;
    return false;    
}




bool a_before_b(DocNode* a, DocNode* b)
{
    if(a->page_index < b->page_index
    || (a->page_index == b->page_index && a->table_index < b->table_index)
    || (a->page_index == b->page_index && a->table_index == b->table_index && a->row_index < b->row_index))
    {
        return true;
    }
    return false;
}



int getRowIndexInfo(vector<TEDOp<DocNode>>& ops, Node<DocNode>* row, int& page_index, int& table_index, int& row_index, bool deleteFlag)
{
    // ret = 1, 表示取 top 位置, 0 表示取 bottom 位置
    int ret = 0;
    int min_dis = INT_MAX;
    
    for(auto& op : ops)
    {
        if(isNodeMatch(op, ROW))
        {
            int dis = 0;

            if(deleteFlag)
            {
                
                dis += 100 * abs(op.arg1->nodeData->page_index - row->nodeData->page_index);
                dis += 10 * abs(op.arg1->nodeData->table_index - row->nodeData->table_index);
                dis += abs(op.arg1->nodeData->row_index - row->nodeData->row_index);
                if(dis < min_dis)
                {
                    min_dis = dis;
                    page_index = op.arg2->nodeData->page_index;
                    table_index = op.arg2->nodeData->table_index;
                    row_index = op.arg2->nodeData->row_index;
                    ret = a_before_b(row->nodeData, op.arg1->nodeData);
                }
            }
            else
            {
                dis += 100 * abs(op.arg2->nodeData->page_index - row->nodeData->page_index);
                dis += 10 * abs(op.arg2->nodeData->table_index - row->nodeData->table_index);
                dis += abs(op.arg2->nodeData->row_index - row->nodeData->row_index);
                if(dis < min_dis)
                {
                    min_dis = dis;
                    page_index = op.arg1->nodeData->page_index;
                    table_index = op.arg1->nodeData->table_index;
                    row_index = op.arg1->nodeData->row_index;
                    ret = a_before_b(row->nodeData, op.arg2->nodeData);
                }
            }
        }
    }
    return ret;
}




int DeleteInsertRow2XChar(Document& doc1, 
                          Document& doc2, 
                          vector<TEDOp<DocNode> > & ops,
                          TEDOp<DocNode>& op,
                          std::vector<std::vector<XChar> >& resutl1,
                          std::vector<std::vector<XChar> >& resutl2,
                          CharCounter& char_counter,
                          my_json& detail_result)
{
    bool deleteFlag = op.arg1 ? true : false;

    int page_index1 = -1; 
    int table_index1 = -1; 
    int row_index1 = -1;

    int page_index2 = -1;
    int table_index2 = -1;
    int row_index2 = -1;
    
    int get_top_flag = 0;

    if(deleteFlag)
    {
        page_index1 = op.arg1->nodeData->page_index;
        table_index1 = op.arg1->nodeData->table_index;
        row_index1 = op.arg1->nodeData->row_index;
        get_top_flag = getRowIndexInfo(ops, op.arg1, page_index2, table_index2, row_index2, deleteFlag);
    }
    else
    {
        page_index2 = op.arg2->nodeData->page_index;
        table_index2 = op.arg2->nodeData->table_index;
        row_index2 = op.arg2->nodeData->row_index;
        get_top_flag = getRowIndexInfo(ops, op.arg2, page_index1, table_index1, row_index1, deleteFlag);
    }


    vector<XChar> chars1;
    vector<XChar> chars2;

    XChar area1;
    XChar area2;

    auto assignPageLineIndex = [&get_top_flag](Document& doc, int page_index, int table_index, int row_index, XChar& xch){
        if(page_index < 0 || table_index < 0 || row_index < 0)
            return;

        auto& table = doc.pages[page_index].tables[table_index];
    
        xch.page_index = page_index;
        xch.line_index = 0;

        for(auto& cell : table.table_cells)
        {
            if(cell.location[0] == row_index && cell.line_indices.size())
            {
                xch.line_index = cell.line_indices[0];
                BoundBox cur_cell_box = BoundBox::toBoundBox(cell.position);
                xch.box.left = min(xch.box.left, cur_cell_box.left);
                xch.box.right = max(xch.box.right, cur_cell_box.right);
                xch.box.top = min(xch.box.top, cur_cell_box.top);
                xch.box.bottom = max(xch.box.bottom, cur_cell_box.bottom);
            }
        }

        if(get_top_flag)
        {
            xch.box.bottom = xch.box.top;
            xch.box.bottom++;
        }
        else
        {
            xch.box.top = xch.box.bottom;
            xch.box.bottom++;
        }
    };


    int word_count = 0;
    if(deleteFlag)
    {
        auto& tbl = doc1.pages[page_index1].tables[table_index1];
        auto& pos = doc1.pages[page_index1].areas[tbl.area_index].position;
        word_count = row2XChars(tbl, row_index1, chars1, area1);

        if(page_index2 >= 0 && table_index2 >= 0 && row_index2 >= 0)
        {
            assignPageLineIndex(doc2, page_index2, table_index2, row_index2, area2);
        }
        else
        {
            updatePageAndLineIndex(doc1, doc2, resutl1, resutl2, area1, area2);
        }
        chars2.push_back(area2);
        char_counter.update(word_count, 0, 0);
    }
    else
    {
        auto& tbl = doc2.pages[page_index2].tables[table_index2];
        auto& pos = doc2.pages[page_index2].areas[tbl.area_index].position;
        word_count = row2XChars(tbl, row_index2, chars2, area2);

        if(page_index1 >= 0 && table_index1 >= 0 && row_index1 >= 0)
        {
            assignPageLineIndex(doc1, page_index1, table_index1, row_index1, area1);
        }
        else
        {
            updatePageAndLineIndex(doc2, doc1, resutl2, resutl1, area2, area1);
        }
        chars1.push_back(area1);
        char_counter.update(0, word_count, 0);
    }
    
    if(word_count)
    {
        for(auto& c : chars1)
        {
            c.char_status = deleteFlag ? xwb::DELETE : xwb::INSERT;
        }

        for(auto& c : chars2)
        {
            c.char_status = deleteFlag ? xwb::DELETE : xwb::INSERT;
        }

        my_json result;
        result["status"] = deleteFlag ? "DELETE" : "INSERT";
        result["type"] = "row";
        result["area"] = my_json::array();

        result["area"].push_back(area1);
        result["area"].push_back(area2);
        result["area"][1]["doc_index"] = 2;

        result["diff"] = my_json::array();

        my_json sub_result;
        sub_result["status"] = result["status"];
        sub_result["type"] = "text";
        sub_result["diff"] = my_json::array();
        sub_result["diff"].push_back(chars1);
        sub_result["diff"].push_back(chars2);
        for(auto& s : sub_result["diff"][1])
        {
            s["doc_index"] = 2;
        }

        result["diff"].push_back(sub_result);
        detail_result.push_back(result);
    }

    return 0;
}



int getCellIndexInfo(vector<TEDOp<DocNode>>& ops, Node<DocNode>* cell, int& page_index, int& table_index, int& row_index, int& col_index, bool deleteFlag)
{
    for(auto& op : ops)
    {
        if(isNodeMatch(op, ROW))
        {
            if(deleteFlag)
            {
                if(cell->parent == op.arg1)
                {
                    page_index = op.arg2->nodeData->page_index;
                    table_index = op.arg2->nodeData->table_index;
                    row_index = op.arg2->nodeData->row_index;
                    col_index = 0;
                    int min_dis = INT_MAX;
                    for(auto& t_op : ops)
                    {
                        if(isNodeMatch(t_op, CELL) && t_op.arg2->parent == op.arg2)
                        {
                            int dis = abs(t_op.arg2->nodeData->cell->location[2] - cell->nodeData->cell->location[2]);
                            if (dis < min_dis && dis > 0)
                            {
                                col_index = max(col_index, t_op.arg2->nodeData->cell->location[2]);
                                min_dis = dis;
                            }
                            
                        }
                    }
                    return 0;
                }
            }
            else
            {
                if(cell->parent == op.arg2)
                {
                    page_index = op.arg1->nodeData->page_index;
                    table_index = op.arg1->nodeData->table_index;
                    row_index = op.arg1->nodeData->row_index;
                    col_index = 0;
                    int min_dis = INT_MAX; //abs(col_index - cell->nodeData->cell->location[2]);
                    for(auto& t_op : ops)
                    {
                        if(isNodeMatch(t_op, CELL) && t_op.arg1->parent == op.arg1)
                        {
                            int dis = abs(t_op.arg1->nodeData->cell->location[2] - cell->nodeData->cell->location[2]);
                            if(dis < min_dis && dis > 0)
                            {
                                min_dis = min(dis, min_dis);
                                col_index = t_op.arg1->nodeData->cell->location[2];
                            }
                        }
                    }
                    return 0;
                }
            }
        }
    }

    return -1;
}



int DeleteInsertCell2XChar(Document& doc1, 
                          Document& doc2, 
                          std::vector<TEDOp<DocNode> >& ops,
                          TEDOp<DocNode>& op,       
                          std::vector<std::vector<XChar> >& resutl1,
                          std::vector<std::vector<XChar> >& resutl2,
                          CharCounter& char_counter,
                          my_json& detail_result)
{
    bool deleteFlag = op.arg1 ? true : false;
    int page_index1 = -1;
    int table_index1 = -1;
    int row_index1 = -1;
    int col_index1 = -1;
    
    int page_index2 = -1;
    int table_index2 = -1;
    int row_index2 = -1;
    int col_index2 = -1;
    
    if(deleteFlag)
    {
        page_index1 = op.arg1->nodeData->page_index;
        table_index1 = op.arg1->nodeData->table_index;
        row_index1 = op.arg1->nodeData->row_index;
        col_index1 = op.arg1->nodeData->cell->location[2];
        getCellIndexInfo(ops, op.arg1, page_index2, table_index2, row_index2, col_index2, deleteFlag);
    }
    else
    {
        page_index2 = op.arg2->nodeData->page_index;
        table_index2 = op.arg2->nodeData->table_index;
        row_index2 = op.arg2->nodeData->row_index;
        col_index2 = op.arg2->nodeData->cell->location[2];
        getCellIndexInfo(ops, op.arg2, page_index1, table_index1, row_index1, col_index1, deleteFlag);
    }

    vector<XChar> chars1;
    vector<XChar> chars2;

    XChar area1;
    XChar area2;

    auto assignPageLineIndex = [](Document& doc, int page_index, int table_index, int row_index, int col_index, XChar& xch){
        if(page_index < 0 || table_index < 0 || row_index < 0)
            return;
        
        auto& table = doc.pages[page_index].tables[table_index];
        xch.page_index = page_index;
        xch.line_index = 0;

        for(auto& cell : table.table_cells)
        {
            //if(cell.location[0] == row_index && cell.location[2] == col_index && cell.line_indices.size())
            if(cell.location[0] == row_index && cell.location[2] == col_index)
            {
                if(cell.line_indices.size())
                    xch.line_index = cell.line_indices[0];
                xch.box = BoundBox::toBoundBox(cell.position);
                break;
            }
        }
        
        xch.box.left = xch.box.right++;
    };


    int word_count = 0;
    if(deleteFlag)
    {
        auto& tbl = doc1.pages[page_index1].tables[table_index1];
        auto& pos = doc1.pages[page_index1].areas[tbl.area_index].position;
        word_count = cell2XChars(op.arg1, chars1, area1);

        if(page_index2 >= 0 && table_index2 >= 0 && row_index2 >= 0)
        {
            assignPageLineIndex(doc2, page_index2, table_index2, row_index2, col_index2, area2);
        }
        else
        {
            updatePageAndLineIndex(doc1, doc2, resutl1, resutl2, area1, area2);
        }
        chars2.push_back(area2);
        char_counter.update(word_count, 0, 0);
    }
    else
    {
        auto& tbl = doc2.pages[page_index2].tables[table_index2];
        auto& pos = doc2.pages[page_index2].areas[tbl.area_index].position;
        word_count = cell2XChars(op.arg2, chars2, area2);

        if(page_index1 >= 0 && table_index1 >= 0 && row_index1 >= 0)
        {
            assignPageLineIndex(doc1, page_index1, table_index1, row_index1, col_index1, area1);
        }
        else
        {
            updatePageAndLineIndex(doc2, doc1, resutl2, resutl1, area2, area1);
        }
        char_counter.update(0, word_count, 0);
        chars1.push_back(area1);
    }
    
    if(word_count)
    {
        my_json result;
        result["status"] = deleteFlag ? "DELETE" : "INSERT";
        result["type"] = "cell";
        result["area"] = my_json::array();

        result["area"].push_back(area1);
        result["area"].push_back(area2);
        result["area"][1]["doc_index"] = 2;

        result["diff"] = my_json::array();

        my_json sub_result;
        sub_result["status"] = result["status"];
        sub_result["type"] = "text";
        sub_result["diff"] = my_json::array();
        sub_result["diff"].push_back(chars1);
        sub_result["diff"].push_back(chars2);
        for(auto& s : sub_result["diff"][1])
        {
            s["doc_index"] = 2;
        }

        result["diff"].push_back(sub_result);
        detail_result.push_back(result);
    }

    return 0;
}


int compare_cell(TEDOp<DocNode>& op, 
                 vector<vector<XChar> >& cell_result1, 
                 vector<vector<XChar> >& cell_result2, 
                 CharCounter& char_counter, std::wstring& origin_text1, std::wstring& origin_text2)
{
    vector<TextLine>& textlines1 = op.arg1->nodeData->cell->lines;
    vector<TextLine>& textlines2 = op.arg2->nodeData->cell->lines;
    
    std::wstring textstr1;
    vector<XChar> chars1;
    vector<int> origin_text_index1; // 字符下标映射，f[a] = b，a代表过滤标点符号后的文本的某个字符，b代表该字符对应在文档origin_text中的下标。

    std::wstring textstr2;
    vector<XChar> chars2;
    vector<int> origin_text_index2;
    std::for_each(textlines1.begin(), textlines1.end(), [&textstr1, &chars1, &origin_text_index1](const TextLine& textline){
        textstr1 += textline.text;
        chars1.insert(chars1.end(), textline.chars.begin(), textline.chars.end());
        origin_text_index1.insert(origin_text_index1.end(), textline.origin_text_index.begin(), textline.origin_text_index.end());
    });
    std::for_each(textlines2.begin(), textlines2.end(), [&textstr2, &chars2, &origin_text_index2](const TextLine& textline){
        textstr2 += textline.text;
        chars2.insert(chars2.end(), textline.chars.begin(), textline.chars.end());
        origin_text_index2.insert(origin_text_index2.end(), textline.origin_text_index.begin(), textline.origin_text_index.end());
    });

    if(textstr1.size() && textstr2.size())
    {
        diff_main_myers(textstr1, textstr2, chars1, chars2, cell_result1, cell_result2, char_counter, origin_text_index1, origin_text_index2, origin_text1, origin_text2);
    }
    else if(textstr1.size() || textstr2.size())
    {
        if(textstr1.size())
        {
            vector<XChar> result1;
            for(auto& line : textlines1)
            {
                XChar xch = textline2XChar(line);
                xch.char_status = DELETE;
                result1.push_back(xch);
            }
            vector<XChar> result2;
            XChar ch;
            ch.page_index = op.arg2->nodeData->page_index;
            ch.char_status = DELETE;
            // wrong
            ch.line_index = 0;
            ch.box = BoundBox::toBoundBox(op.arg2->nodeData->cell->position);
            result2.push_back(ch);
            cell_result1.push_back(result1);
            cell_result2.push_back(result2);

            char_counter.update(textstr1.size(), 0, 0);
        }
        else
        {
            vector<XChar> result2;
            for(auto& line : textlines2)
            {
                XChar xch = textline2XChar(line);
                xch.char_status = INSERT;
                result2.push_back(xch);
            }
            vector<XChar> result1;
            XChar ch;
            ch.page_index = op.arg1->nodeData->page_index;
            ch.char_status = INSERT;
            // wrong
            ch.line_index = 0;
            ch.box = BoundBox::toBoundBox(op.arg1->nodeData->cell->position);
            result1.push_back(ch);
            cell_result1.push_back(result1);
            cell_result2.push_back(result2);

            char_counter.update(0, textstr2.size(), 0);
        }
    }
    return 0;
}


vector<TextLine> getStamps(Document& doc)
{
    vector<TextLine> results;

    for(int page_index = 0; page_index < doc.pages.size(); page_index++)
    {
        auto& lines = doc.pages[page_index].lines;
        for(int line_index = 0; line_index < lines.size(); line_index++)
        {
            if(lines[line_index].is_stamp)
            {
                results.emplace_back(lines[line_index]);
            }
        }
    }
    return results;
}

XChar stamp2XChar(TextLine& stamp)
{
    XChar ch;
    ch.page_index = stamp.page_index;
    ch.box = stamp.poly;
    ch.line_index = stamp.line_index;
    ch.text = stamp.text;
    ch.pos_list.push_back(stamp.poly);
    return ch;
}


XChar findCorrespondingPosForStamp(Document& doc1, 
                                 Document& doc2, 
                                 std::vector<std::vector<XChar> >& result1,
                                 std::vector<std::vector<XChar> >& result2, 
                                 const XChar& stamp)
{
    int target_page_index = -1;
    int target_line_index = 0;
    int target_offset = 0;

    int r_index = -1;
    int min_dis = INT_MAX;
    int char_index = -1;
    for(int i = 0; i < result1.size(); i++)
    {
        auto& para1 = result1[i];
        auto& para2 = result2[i];

        CharStatus char_status = para1[0].char_status;
        if (char_status != NORMAL)
        {
            continue;
        }
            
        // 文字对齐的位置
        int char_count_1 = 0;
        for(int j = 0; j < para1.size(); j++)
        {
            char_count_1 += para1[j].pos_list.size(); // 字符偏移量（从pos_list获取是为了避免标点带来的影响，此时para2[j].text的内容是原始文本内容，包含标点空格）

            if(stamp.page_index == para1[j].page_index)
            {
                int dis = abs( (para1[j].box.bottom + para1[j].box.top) - (stamp.box.bottom + stamp.box.top) );
                if (dis < min_dis)
                {
                    min_dis = dis;
                    r_index = i;
                    char_index = char_count_1;
                }
            }
        }
    }

    if(r_index >= 0)
    {
        auto& para1 = result1[r_index];
        auto& para2 = result2[r_index];
        int char_count_2 = 0;
        for(int j = 0; j < para2.size(); j++)
        {
            char_count_2 += para2[j].pos_list.size(); // 字符偏移量（从pos_list获取是为了避免标点带来的影响，此时para2[j].text的内容是原始文本内容，包含标点空格）
            if(char_count_2 >= char_index)
            {
                target_page_index = para2[j].page_index;
                target_line_index = para2[j].line_index;
                // 计算offset也要考虑页面比率
                float rate = 1.0;
                if (doc2.pages[target_page_index].width > 0)
                    rate = (float)doc1.pages[stamp.page_index].width / doc2.pages[target_page_index].width;
                target_offset = para2[j].box.top * rate - para1[j].box.top;
                break;
            }
        }
    }

    if(target_page_index == -1)
    {
        int min_page_dis = INT_MAX;
        int r_index = -1;
        int char_index = -1;

        for(int i = 0; i < result1.size(); i++)
        {
            auto& para1 = result1[i];
            auto& para2 = result2[i];

            if(para1[0].char_status != NORMAL)
            {
                continue;
            }

            int char_count_1 = 0;
            for(int j = 0; j < para1.size(); j++)
            {
                char_count_1 += para1[j].pos_list.size(); // 字符偏移量（从pos_list获取是为了避免标点带来的影响，此时para1[j].text的内容是原始文本内容，包含标点空格）
                int dis = abs(para1[j].page_index - stamp.page_index);
                if(dis < min_page_dis)
                {
                    min_page_dis = dis;
                    r_index = i;
                    char_index = char_count_1;
                }
            }
        }

        if(r_index >= 0)
        {
            auto& para1 = result1[r_index];
            auto& para2 = result2[r_index];
            int char_count_2 = 0;
            for(int j = 0; j < para2.size(); j++)
            {
                char_count_2 += para2[j].pos_list.size(); // 字符偏移量（从pos_list获取是为了避免标点带来的影响，此时para2[j].text的内容是原始文本内容，包含标点空格）
                if(char_count_2 >= char_index)
                {
                    target_page_index = para2[j].page_index;
                    target_line_index = para2[j].line_index;
                    // 计算offset也要考虑页面比率
                    float rate = 1.0;
                    if (doc2.pages[target_page_index].width > 0)
                        rate = (float)doc1.pages[stamp.page_index].width / doc2.pages[target_page_index].width;
                    target_offset = para2[j].box.top * rate - para1[j].box.top;
                    break;
                }
            }
        }
    }

    if(target_page_index == -1)
    {
        target_page_index = 0;
    }

    float rate = 1.0;
    if(doc1.pages[stamp.page_index].width > 0 && doc2.pages.size() > 0)
        rate = (float)doc2.pages[target_page_index].width / doc1.pages[stamp.page_index].width;   


    XChar ch2;
    ch2.page_index = target_page_index;
    ch2.line_index = target_line_index;
    ch2.box = stamp.box;
    ch2.box.top += target_offset;
    ch2.box.bottom += target_offset;

    ch2.box.left = ch2.box.left * rate;
    ch2.box.top = ch2.box.top * rate;
    ch2.box.right = ch2.box.right * rate;
    ch2.box.bottom = ch2.box.bottom * rate;


    ch2.pos_list.push_back(ch2.box);

    return ch2;
}

static bool stampOverlap(XChar& ch1, XChar& ch2)
{
    return min(ch1.box.bottom, ch2.box.bottom) - max(ch1.box.top, ch2.box.top) > 0
           && min(ch1.box.right, ch2.box.right) - max(ch1.box.left, ch2.box.left) > 0;
}


int compare_stamps(Document& doc1, 
                   Document& doc2, 
                  std::vector<std::vector<XChar> >& result1,
                  std::vector<std::vector<XChar> >& result2,
                  CharCounter& char_counter,
                  my_json& detail_result)
{
    TIME_COST_FUNCTION;
    vector<TextLine> stamps1 = getStamps(doc1);
    vector<TextLine> stamps2 = getStamps(doc2);

    // 如何对齐章
    // 通过文本比较结果，获取左边章对应右边的位置
    // 然后在范围内的右边章

    for(int i = 0; i < stamps1.size(); i++)
    {
        XChar ch1 = stamp2XChar(stamps1[i]);
        XChar ch2 = findCorrespondingPosForStamp(doc1, doc2, result1, result2, ch1);

        string status = "DELETE";
        std::wstring stamp_origin_text2;
        if(stamps2.size())
        {
            for(int j = 0; j < stamps2.size(); j++)
            {
                XChar ch3 = stamp2XChar(stamps2[j]);

                if(ch2.page_index == ch3.page_index && stampOverlap(ch2, ch3))
                {
                    ch2 = ch3;
                    if(ch1.text == ch3.text)
                    {
                        status = "NORMAL";
                    }
                    else
                    {
                        status = "CHANGE";
                    }
                    stamp_origin_text2 = stamps2[j].origin_text;
                    stamps2.erase(stamps2.begin() + j);
                    break;
                }
            }
        }

        if(status == "NORMAL")
        {
            char_counter.update(1, 1, 1);
            continue;
        }
            

        char_counter.update(1, ch2.text.size() ? 1 : 0, 0);

        my_json diff_js;
        diff_js["status"] = status;
        diff_js["type"] = "stamp";
        diff_js["diff"] = my_json::array();
        {
            my_json diff_doc1 = my_json::array();
            ch1.text = stamps1[i].origin_text; // 返回原始文本内容
            my_json char_js = ch1;
            diff_doc1.push_back(char_js);
        
            diff_js["diff"].push_back(diff_doc1);
        }

        {
            my_json diff_doc2 = my_json::array();
            ch2.text = stamp_origin_text2; // 返回原始文本内容
            my_json char_js = ch2;
            char_js["doc_index"] = 2;
            diff_doc2.push_back(char_js);
            diff_js["diff"].push_back(diff_doc2);
        }

        detail_result.push_back(diff_js);


    }

    // 新增
    if(stamps2.size())
    {
        for(auto& stamp : stamps2)
        {
            XChar ch2 = stamp2XChar(stamp);
            XChar ch1 = findCorrespondingPosForStamp(doc2, doc1, result2, result1, ch2);
            
            my_json diff_js;
            diff_js["status"] = "INSERT";
            diff_js["type"] = "stamp";
            diff_js["diff"] = my_json::array();
            {
                my_json diff_doc1 = my_json::array();
                
                my_json char_js = ch1;
                diff_doc1.push_back(char_js);
            
                diff_js["diff"].push_back(diff_doc1);
            }

            {
                my_json diff_doc2 = my_json::array();
                ch2.text = stamp.origin_text; // 返回原始文本内容
                my_json char_js = ch2;
                char_js["doc_index"] = 2;
                diff_doc2.push_back(char_js);
                diff_js["diff"].push_back(diff_doc2);
            }
            char_counter.update(0, 1, 0);
            detail_result.push_back(diff_js);
        }
    }
    
    return 0;
}



int compare_table(Document& doc1, 
                  Document& doc2, 
                  std::vector<std::vector<XChar> >& resutl1,
                  std::vector<std::vector<XChar> >& resutl2,
                  CharCounter& char_counter,
                  my_json& detail_result, std::wstring orgin_text1, std::wstring orgin_text2)
{
    TIME_COST_FUNCTION;

    vector<Node<DocNode>*> tables_1;
    vector<Node<DocNode>*> tables_2;

    Node<DocNode>* t1 = generateTableTree(doc1, tables_1);
    Node<DocNode>* t2 = generateTableTree(doc2, tables_2);
    vector<TEDOp<DocNode>> ops;

    int dis = zss_ted_backtrack(t1, t2, ops);


    // 如果一个表格的所有行都是删除或者新增，那么这个表格就是删除或者新增
    if(true)
    {
        unordered_map<Node<DocNode>*, int> delete_rows;
        unordered_map<Node<DocNode>*, int> insert_rows;

        for(auto& op : ops)
        {
            if(op.arg1 && op.arg2)
            {
                continue;
            }
            if(op.arg1 && op.arg1->nodeData->ntype == ROW)
            {
                delete_rows[op.arg1] = 1;
            }
            else if(op.arg2 && op.arg2->nodeData->ntype == ROW)
            {
                insert_rows[op.arg2] = 1;
            }
        }

        for(auto& table : tables_1)
        {
            bool delete_flag = true;
            for(int i = 0; i < table->children.size(); i++)
            {
                if(delete_rows.find(table->children[i]) == delete_rows.end())
                {
                    delete_flag = false;
                    break;
                }
            }

            // 此表格为删除
            if(delete_flag)
            {
                TEDOp<DocNode> d(2, table, nullptr);
                setVisitedFlag(d.arg1);
                DeleteInsertTable2XChar(doc1, doc2, d, resutl1, resutl2, char_counter, detail_result);
            }
        }

        for(auto& table : tables_2)
        {
            bool insert_flag = true;
            for(int i = 0; i < table->children.size(); i++)
            {
                if(insert_rows.find(table->children[i]) == insert_rows.end())
                {
                    insert_flag = false;
                    break;
                }
            }

            // 此表格为新增
            if(insert_flag)
            {
                TEDOp<DocNode> d(1, nullptr, table);
                setVisitedFlag(d.arg2);
                DeleteInsertTable2XChar(doc1, doc2, d, resutl1, resutl2, char_counter, detail_result);
            }
        }
    }
    
    // 先处理增加表格和删除表格
    // 由于现在树中不含表格节点，可能这一段代码不做任何实质动作。
    for(auto& op : ops)
    {
        if(op.arg1 && op.arg2)
        {
            // do nothing
        }
        else if(op.arg1 && op.arg1->nodeData->ntype == TABLE) // 删除表格
        {
            setVisitedFlag(op.arg1);
            DeleteInsertTable2XChar(doc1, doc2, op, resutl1, resutl2, char_counter, detail_result);
        }
        else if(op.arg2 && op.arg2->nodeData->ntype == TABLE) // 新增表格
        {
            setVisitedFlag(op.arg2);
            DeleteInsertTable2XChar(doc1, doc2, op, resutl1, resutl2, char_counter, detail_result);
        }
    }

    // 处理删除行和新增行
    for(auto& op : ops)
    {
        if(op.arg1 && op.arg2)
        {

        }
        else if(op.arg1 && op.arg1->visited == false && op.arg1->nodeData->ntype == ROW) // 删除一行
        {
            setVisitedFlag(op.arg1);
            DeleteInsertRow2XChar(doc1, doc2, ops, op, resutl1, resutl2, char_counter, detail_result);
        }
        else if(op.arg2 && op.arg2->visited == false && op.arg2->nodeData->ntype == ROW) // 增加一行
        {
            setVisitedFlag(op.arg2);
            DeleteInsertRow2XChar(doc1, doc2, ops, op, resutl1, resutl2, char_counter, detail_result);
        }
    }

    // 处理删除单元格和新增单元格
    for(auto& op : ops)
    {
        if(op.arg1 && op.arg2)
        {

        }
        else if(op.arg1 && op.arg1->visited == false && op.arg1->nodeData->ntype == CELL) // 删除一个单元格
        {
            setVisitedFlag(op.arg1);
            DeleteInsertCell2XChar(doc1, doc2, ops, op, resutl1, resutl2, char_counter, detail_result);
        }
        else if(op.arg2 && op.arg2->visited == false && op.arg2->nodeData->ntype == CELL) // 新增一个单元格
        {
            setVisitedFlag(op.arg2);
            DeleteInsertCell2XChar(doc1, doc2, ops, op, resutl1, resutl2, char_counter, detail_result);
        }
    }

    for(auto& op : ops)
    {
        if(isNodeMatch(op, CELL))
        {
            vector<vector<XChar> > cell_result1;
            vector<vector<XChar> > cell_result2;

            compare_cell(op, cell_result1, cell_result2, char_counter, orgin_text1, orgin_text2);

            if(cell_result1.size())
            {
                vector<TextLine>& textlines1 = op.arg1->nodeData->cell->lines;
                vector<TextLine>& textlines2 = op.arg2->nodeData->cell->lines;

                XChar area1;
                area1.page_index = op.arg1->nodeData->page_index;
                if(textlines1.size())
                    area1.line_index = textlines1[0].line_index;
                area1.box = BoundBox::toBoundBox(op.arg1->nodeData->cell->position);
                

                XChar area2;
                area2.page_index = op.arg2->nodeData->page_index;
                if(textlines2.size())
                    area2.line_index = textlines2[0].line_index;
                area2.box = BoundBox::toBoundBox(op.arg2->nodeData->cell->position);

                my_json result;
                result["status"] = "CHANGE";
                result["type"] = "cell";
                result["area"] = my_json::array();

                result["area"].push_back(area1);
                result["area"].push_back(area2);
                result["area"][1]["doc_index"] = 2;

                result["diff"] = my_json::array();

                for(int i = 0; i < cell_result1.size(); i++)
                {
                    if(cell_result1[i][0].char_status == NORMAL)
                        continue;

                    my_json sub_result;

                    sub_result["status"] = statusString(cell_result1[i][0].char_status);
                    sub_result["type"] = "text";
                    sub_result["diff"] = my_json::array();
                    sub_result["diff"].push_back(cell_result1[i]);
                    sub_result["diff"].push_back(cell_result2[i]);
                    for(auto&s : sub_result["diff"][1])
                    {
                        s["doc_index"] = 2;
                    }

                    result["diff"].push_back(sub_result);
                }

                if(result["diff"].size())
                {
                    detail_result.push_back(result);
                }
            }
        }
    }

    delete t1;
    delete t2;
    
    for(auto& t: tables_1)
    {
        t->children.clear();
        delete t;
    }

    for(auto& t: tables_2)
    {
        t->children.clear();
        delete t;
    }

    return 0;
}



void update_pos_after(vector<XChar>& para, int start, int end, int target_id)
{
    XChar& target = para[target_id];
    for(int id = start; id < end; id++)
    {
        XChar& line = para[id];
        line.page_index = target.page_index;
        line.line_index = target.line_index;
        
        line.box.left = target.box.left;
        line.box.right = line.box.left;
        line.box.top = target.box.top;
        line.box.bottom = target.box.bottom;
    }
}
    


void update_pos_before(std::vector<XChar>& para,int start, int end, int target_id)
{
    XChar& target = para[target_id];
    for(int id = start; id < end; id++)
    {
        XChar& line = para[id];
        line.page_index = target.page_index;
        line.line_index = target.line_index;
        line.box.left = target.box.right;
        line.box.right = line.box.left;
        line.box.top = target.box.top;
        line.box.bottom = target.box.bottom;
    }
}

int line_process(vector<XChar>& para, int para_index, vector<int>& invalid_para_list)
{
    int start = 0;
    int end = 0;

    while(end < para.size())
    {
        if(para[start].page_index > -1)
        {
            start += 1;
            end = start;
            continue;
        }
            

        XChar& line = para[end];
        
        if(line.page_index > -1 && end - start > 0)
        {
            if(end < para.size())
            {
                update_pos_after(para, start, end, end);
            }
            else if(start > 0)
            {
                update_pos_before(para, start, end, start - 1);
            }
            else//:  # total para can't find pos infor
            {
                invalid_para_list.push_back(para_index);
            }
                
            start = end + 1;
        
        }
        end += 1;
    }

    if(start > 0 && end - start > 0)
    {
        update_pos_before(para, start, end, start - 1);
    }
        
    if(start == 0 && end == para.size())
    {
        invalid_para_list.push_back(para_index);
    }
    return 0;
}

int position_align(std::vector<std::vector<XChar> >& resutl1, std::vector<std::vector<XChar> >& resutl2)
{
    TIME_COST_FUNCTION;

    vector<int> invalid_para_list1;
    vector<int> invalid_para_list2;

    for(int para_index = 0; para_index < resutl1.size(); para_index++)
    {
        line_process(resutl1[para_index], para_index, invalid_para_list1);
        line_process(resutl2[para_index], para_index, invalid_para_list2);
    }

    for(auto para_index : invalid_para_list1)
    {
        auto& para = resutl1[para_index];
        
        if(para_index + 1 < resutl1.size())
        {
            XChar& target = resutl1[para_index + 1][0];
            for(auto& line : para)
            {
                line.page_index = target.page_index;
                line.line_index = target.line_index;
                line.box.left = target.box.left;
                line.box.right = line.box.left;
                line.box.top = target.box.top;
                line.box.bottom = target.box.bottom;
            } 
        }
        else if(para_index - 1 >= 0)
        {
            XChar &target = resutl1[para_index - 1].back();
            for(auto& line : para)
            {
                line.page_index = target.page_index;
                line.line_index = target.line_index;
                line.box.left = target.box.right;
                line.box.right = line.box.left;
                line.box.top = target.box.top;
                line.box.bottom = target.box.bottom;
            }
        }
    }

    for(auto para_index : invalid_para_list2)
    {
        auto& para = resutl2[para_index];
        
        if(para_index + 1 < resutl2.size())
        {
            auto& target = resutl2[para_index + 1][0];
            for(auto& line: para)
            {
                line.page_index = target.page_index;
                line.line_index = target.line_index;
                line.box.left = target.box.left;
                line.box.right = line.box.left;
                line.box.top = target.box.top;
                line.box.bottom = target.box.bottom;
            }
        }
        else if(para_index - 1 >= 0)
        {
            auto& target = resutl2[para_index - 1].back();
            for(auto &line : para)
            {
                line.page_index = target.page_index;
                line.line_index = target.line_index;
                line.box.left = target.box.right;
                line.box.right = line.box.left;
                line.box.top = target.box.top;
                line.box.bottom = target.box.bottom;
            }       
        }
    }
    return 0;
}







void update_result(my_json& result, XChar& char1, XChar& char2)
{
    CharStatus char_status = char1.char_status;
    // if(char_status != CHANGE)
    // {
    //     // char1.candi.clear();
    //     // char1.candi_score.clear();
    //     char1.pos_list.clear();

    //     // char2.candi.clear();
    //     // char2.candi_score.clear();
    //     char2.pos_list.clear();
    // }

    if(char_status == DELETE)
    {   
        char2.text.clear();
    }
    else if(char_status == INSERT)
    {
        char1.text.clear();
    }

    my_json pair_result = my_json::array();
    my_json char_js1 = char1;
    my_json char_js2 = char2;
    char_js1["status"] = statusString(char_status);
    char_js2["doc_index"] = 2;
    char_js2["status"] = statusString(char_status);

    pair_result.push_back(char_js1);
    pair_result.push_back(char_js2);

    result.push_back(pair_result);
}

int generate_result(std::vector<std::vector<XChar> >& resutl1, 
                    std::vector<std::vector<XChar> >& resutl2, 
                    my_json& result)
{
    TIME_COST_FUNCTION;

    for(int i = 0; i < resutl1.size(); i++)
    {
        auto& para1 = resutl1[i];
        auto& para2 = resutl2[i];
    
        int idx = 0;
        while(idx < para1.size())
        {
            auto& line1 = para1[idx];
            auto& line2 = para2[idx];
            if(line1.char_status == line2.char_status && line1.char_status != NORMAL)
            {
                update_result(result, line1, line2);
            }            
            idx += 1;
        
        }
    }
    return 0;
}

int generate_result_diff(std::vector<std::vector<XChar> >& result1, 
                         std::vector<std::vector<XChar> >& result2, 
                         my_json& result)
{
    for(int i = 0; i < result1.size(); i++)
    {
        auto& para1 = result1[i];
        auto& para2 = result2[i];

        CharStatus char_status = para1[0].char_status;
        if(char_status == NORMAL)
            continue;

        my_json diff_js;
        diff_js["status"] = statusString(char_status);
        diff_js["type"] = "text";
        diff_js["diff"] = my_json::array();
        {
            my_json diff_doc1 = my_json::array();
            for(auto& a : para1)
            {
                my_json char_js = a;
                diff_doc1.push_back(char_js);
            }
            diff_js["diff"].push_back(diff_doc1);
        }

        {
            my_json diff_doc2 = my_json::array();
            for(auto& a : para2)
            {
                my_json char_js = a;
                char_js["doc_index"] = 2;
                diff_doc2.push_back(char_js);
            }
            diff_js["diff"].push_back(diff_doc2);
        }
        result.push_back(diff_js);
    }
    return 0;
}



std::string contract_compare(const std::string& docs, RuntimeConfig& config)
{
    initLOGX();

    TIME_COST_FUNCTION;
    

    Context context;
    context.ReadRuntimeConfig(config);

    my_json detail_result = my_json::array();
    CharCounter char_counter;
    
    try
    {
        Document src;
        Document cnd;

        std::wstring src_origin_text; // 文档1的原始文本内容（包括印章表格页眉页脚，未过滤标点符号）
        std::wstring cnd_origin_text; // 文档2的原始文本内容（包括印章表格页眉页脚，未过滤标点符号）

        my_json js = my_json::parse(docs);

        int doc_index = js[0]["doc_index"].get<int>();
        int first_doc_index = (doc_index == 1 ? 0 : 1);

        src.doc_index = first_doc_index;
        cnd.doc_index = 1-first_doc_index;

        int origin_char_index1 = -1; // 文档1对应的单个字符相对文档src_origin_text的下标
        int origin_char_index2 = -1; // 文档2对应的单个字符相对文档cnd_origin_text的下标

        src.load_pages_with_layout(js[first_doc_index]["pages"], src_origin_text, origin_char_index1, context);
        cnd.load_pages_with_layout(js[1-first_doc_index]["pages"], cnd_origin_text, origin_char_index2, context);

        std::vector<std::vector<XChar> > result1;
        std::vector<std::vector<XChar> > result2;

        compare_text(src, cnd, result1, result2, char_counter, src_origin_text, cnd_origin_text, context);

        generate_result_diff(result1, result2, detail_result);

        compare_table(src, cnd, result1, result2, char_counter, detail_result, src_origin_text, cnd_origin_text);

        compare_stamps(src, cnd, result1, result2, char_counter, detail_result);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    my_json result;
    result["result"]["detail"] = detail_result;
    result["result"]["similarity"] = char_counter.GetSimilarity();

    return result.dump(4);
}


} // namespace xwb

#ifdef ENABLE_LUA
static int lua_func_contract_compare(lua_State* L)
{
    const char *p_docs = lua_tostring(L, 1);
    int ignore_punctuation = lua_tointeger(L, 2);
    const char *p_punctuation = lua_tostring(L, 3);
    int merge_diff = lua_tointeger(L, 4);

    std::string docs(p_docs);
    xwb::RuntimeConfig config;
    config.ignore_punctuation = ignore_punctuation;
    config.punctuation = std::string(p_punctuation);
    config.merge_diff = merge_diff;

    std::string result = xwb::contract_compare(docs, config);    
    // 压栈输出
    if(result.size())
    {
        lua_pushinteger(L,result.size());
        lua_pushlstring(L,(char*)result.data(), result.size());
    }
    else
    {
        lua_pushinteger(L,0);
        lua_pushlstring(L,"", 0);
    }

    return 2;
}

static int lua_func_get_version(lua_State* L)
{
    std::string version = xwb::get_version();
    lua_pushinteger(L,version.size());
    lua_pushlstring(L,(char*)version.data(), version.size());
    return 2;
}

static luaL_Reg luaLibs[] = {
    { "contract_compare", lua_func_contract_compare},
    { "get_version", lua_func_get_version},
    { NULL, NULL }
};


extern "C"
int luaopen_libcontract_compare(lua_State* luaEnv) {
    // libConvertWordToImage 是生成得.so 名
    luaL_register(luaEnv, "libcontract_compare",luaLibs);
    return 1;
}

#endif