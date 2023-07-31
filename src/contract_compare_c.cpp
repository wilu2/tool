#include "contract_compare_c.h"
#include "contract_compare.h"

#ifdef __cplusplus
extern "C"{
#endif

const char* get_version_c()
{
    static std::string version = xwb::get_version();
    return version.c_str();
}


char* contract_compare_c(const char* docs, RuntimeConfigC config_c)
{
    static std::string result_str;
    xwb::RuntimeConfig config;
    config.block_compare = config_c.block_compare;
    config.ignore_punctuation = config_c.ignore_punctuation;
    config.punctuation = std::string(config_c.punctuation);
    config.merge_diff = config_c.merge_diff;
    
    std::string docs_str(docs);
    result_str = xwb::contract_compare(docs_str, config);

    return (char*) result_str.c_str();
}

#ifdef __cplusplus
} // extern "C"
#endif