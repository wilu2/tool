#pragma once

#include <string>

namespace xwb
{

std::string get_version();

struct RuntimeConfig
{
    bool block_compare;
    bool ignore_punctuation;
    std::string punctuation;
    bool merge_diff;


    RuntimeConfig()
        :block_compare(true), ignore_punctuation(false), merge_diff(true){}
};


std::string contract_compare(const std::string& docs, RuntimeConfig& config);

} // namespace xwb
