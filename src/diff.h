#pragma once

#include "XChar.h"
#include <string>
#include <vector>
#include <string>
#include <vector>
#include <time.h>

namespace xwb
{

int diff_main(std::vector<XChar>& char_list_x, int start_x, int end_x,
              std::vector<XChar>& char_list_y, int start_y, int end_y,
              std::vector<XChar>& return_list_x, 
              std::vector<XChar>& return_list_y, int& similarity);


int diff_main_call(std::vector<XChar>& char_x,
              std::vector<XChar>& char_y,
              std::vector<XChar>& return_list1, 
              std::vector<XChar>& return_list2, int& similarity);





class Diff 
{
    // The text associated with this diff operation.
public:
    Diff(CharStatus _operation, const std::wstring& _text) 
    {
      // Construct a diff with the specified operation and text.
      operation = _operation;
      text = _text;
    }

    /**
     * Display a human-readable version of this Diff.
     * @return text version.
     */
    std::wstring ToString()
    {
        std::wstring result;
        if(operation == DELETE)
        {
            result += L"del";
        }
        else if(operation == INSERT)
        {
            result += L"insert";
        }
        else if(operation == NORMAL)
        {
            result += L"equal";
        }
        result += L",";
        result += text;
        return result;
    }

    /**
     * Is this Diff equivalent to another Diff?
     * @param d Another Diff to compare against.
     * @return true or false.
     */
    bool Equals(const Diff& diff) 
    {
      // If parameter is null return false.
        return operation == diff.operation && text == diff.text;
    }

public:
        CharStatus operation;
        std::wstring text;
};


std::vector<Diff> diff_main(std::wstring text1, std::wstring text2, bool checklines, clock_t deadline);
std::vector<Diff> diff_main(std::wstring text1, std::wstring text2, bool checklines);
std::vector<Diff> diff_main(std::wstring text1, std::wstring text2);




int diff_main_myers(const std::wstring& para1, 
                    const std::wstring& para2, 
                    std::vector<XChar>& char_x, 
                    std::vector<XChar>& char_y,
                    std::vector<std::vector<XChar> >& return_list1, 
                    std::vector<std::vector<XChar> >& return_list2, 
                    CharCounter& char_counter, std::vector<int>& origin_text_index1, std::vector<int>& origin_text_index2, std::wstring& origin_text1, std::wstring& origin_text2,
                    bool merge_diff=true);

} // namespace xwb




