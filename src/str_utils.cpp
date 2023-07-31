#include <locale>
#include <codecvt>
#include <algorithm>
#include <regex>
#include <string.h>
#include <unordered_map>
#include "str_utils.h"

using std::min;
using std::max;

namespace xwb
{

std::string to_utf8(const std::wstring& wstr)
{
    //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 0x10ffffUL, std::little_endian>, wchar_t> converter;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

std::wstring from_utf8(const std::string& str)
{
    //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 0x10ffffUL, std::little_endian>, wchar_t> converter;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}



template<class T>
int EditDistance(const T* matchStr, int matchStrLen, const T *patternStr, int patternStrLen)
{
#define D( i, j ) d[(i) * n + (j)]
    int i;
    int j;
    int m = matchStrLen + 1;
    int n = patternStrLen + 1;
    int *d = (int*)malloc(sizeof(int) * (m * n));
    int diff = 0;
    int result;

    memset(d, 0, sizeof(int) * m * n);

    for (i = 1; i < m; i++)
        D(i, 0) = i;
    for (j = 1; j < n; j++)
        D(0, j) = j;
    for (i = 0; i < matchStrLen; i++)
    {
        for (j = 0; j < patternStrLen; j++)
        {
            diff = (matchStr[i] == patternStr[j]) ? 0 : 1;
            D(i + 1, j + 1) = min(min(D(i + 1, j) + 1, D(i, j + 1) + 1), D(i, j) + diff);
        }
    }
    result = D(matchStrLen, patternStrLen);
    free(d);


    return result;
#undef D
}


int EditDistance(const std::wstring& matchStr, const std::wstring&  patternStr)
{
    if(matchStr.size() && patternStr.size())
        return EditDistance(matchStr.c_str(), matchStr.size(), patternStr.c_str(), patternStr.size());
    else if(matchStr.empty() || patternStr.empty())
        return max(matchStr.size(), patternStr.size());
    return 0;
}


//punctuation = """ !\"#&'():;?@[\]^_`{|}~©®·“”℃℉■▶★☑☒✓、。《》"""
//re_punctuation = "[{}]+".format(punctuation)


std::wstring clean_text(const std::wstring& text)
{
    std::wstring result = text;
    result = std::regex_replace(result, std::wregex(L"[ \\!\"#$%&'\\(\\)\\*\\+,-\\./:;<=>\\?@\\[\\]^_`\\{\\|\\}~¥©®·“”€℃℉■▶★☑☒✓、。《》]"), L"");
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    
    return result;
}
    
bool is_punctuation(const std::wstring& puctuation, wchar_t wc)
{ 
    return std::binary_search(puctuation.begin(), puctuation.end(), wc);
}

bool is_punctuation(wchar_t wc)
{ 
    const std::wstring punctuation = L" !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~~¢£¤¥§©«®°±·»àáèéìíòó÷ùúāēěīōūǎǐǒǔǘǚǜΔΣΦΩ฿“”‰₣₤₩₫€₰₱₳₴℃℉≈≠≤≥①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳■▶★☆☑☒✓、。々《》「」『』【】〒〖〗〔〕㎡㎥・･…\r\n";
    static std::unordered_map<wchar_t, wchar_t> m;
    static bool init_flag = false;
    if(!init_flag)
    {
        std::wstring p = punctuation;
        std::sort(p.begin(), p.end());
        for(auto c: p)
        {
            m[c] = 1;
        }
        init_flag = true;
    }

    if(m.find(wc) != m.end())
        return true;

    return false;
}


} // namespace xwb