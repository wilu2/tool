#pragma once

#include <string>

namespace xwb
{


const std::wstring punctuation = L" !\"#&'():;?@[]^_`{|}~©®·“”℃℉■▶★☑☒✓、。《》";


std::string to_utf8(const std::wstring& wstr);

std::wstring from_utf8(const std::string& str);


int EditDistance(const std::wstring& matchStr, const std::wstring&  patternStr);


std::wstring clean_text(const std::wstring& text);

bool is_punctuation(const std::wstring& puctuation, wchar_t wc);
bool is_punctuation(wchar_t);

} // namespace xwb



