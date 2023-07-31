#ifdef __cplusplus
extern "C"{
#endif

const char* get_version_c();

struct RuntimeConfigC
{
    int block_compare;
    int ignore_punctuation;
    char* punctuation;
    int merge_diff;
};


char* contract_compare_c(const char* docs, struct RuntimeConfigC config);

#ifdef __cplusplus
} // extern "C"
#endif