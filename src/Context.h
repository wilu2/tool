#pragma once

#include <string>
#include <algorithm>
#include "str_utils.h"
#include "contract_compare.h"

namespace xwb
{
    


struct Context
{
    bool block_compare;
    bool ignore_punctuation;
    std::wstring punctuation;
    bool merge_diff;

    void ReadRuntimeConfig(RuntimeConfig& config)
    {
        block_compare = config.block_compare;
        ignore_punctuation = config.ignore_punctuation;
        merge_diff = config.merge_diff;
        if(!ignore_punctuation)
        {
            ignore_punctuation = true;
            punctuation = L" ";
        }
        else
        {
            if(config.punctuation.empty())
            {
                punctuation = L" !\"#$%&'()*+,-./:：;；<=>?@[\\]^_`{|}~~¢£¤¥§©«®°±·»àáèéìíòó÷ùúāēěīōūǎǐǒǔǘǚǜΔΣΦΩ฿“”‰₣₤₩₫€₰₱₳₴℃℉≈≠≤≥①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳■▶★☆☑☒✓、。々《》「」『』【】〒〖〗〔〕㎡㎥・･…\r\n";
            }
            else
            {
                punctuation = from_utf8(config.punctuation);
                punctuation += L' ';
            }
        }

        if(punctuation.size() > 1)
        {
            std::sort(punctuation.begin(), punctuation.end());
        }
    }
};


} // namespace xwb
