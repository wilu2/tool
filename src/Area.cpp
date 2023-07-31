#include "Area.h"
#include "str_utils.h"

namespace xwb
{

void Area::parse(my_json& js)
{
    index = js["index"].get<int>();
    score = js["score"].get<float>();
    type = from_utf8(js["type"].get<std::string>());
    position = js["position"].get<std::vector<int> >();
}


} // namespace xwb


