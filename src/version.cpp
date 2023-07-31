	
#include "contract_compare.h"
    // using gitlab-runner to generate version

namespace xwb
{
    
std::string get_version() 
{
    return "CI_COMMIT_REF_NAME-CI_COMMIT_SHA";
}

} // namespace xwb

