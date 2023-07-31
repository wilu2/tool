#include "Table.h"


namespace xwb
{


   


int TableCell::parse(my_json& j)
{
    location = j["location"].get<std::vector<int> >();
    position = j["position"].get<std::vector<int> >();

    if(j["line_indices"].is_array())
    {
        line_indices = j["line_indices"].get<std::vector<int> >();    
    }

    return 0;
}



int Table::parse(my_json& j)
{
    area_index = j["area_index"].get<int>();
    table_rows = j["table_rows"].get<int>();
    table_cols = j["table_cols"].get<int>();
    table_type = j["table_type"].get<std::string>();

    for(auto& js_cell : j["table_cells"])
    {
        TableCell cell;
        cell.parse(js_cell);

        table_cells.push_back(cell);
    }
    return 0;
}


} // namespace xwb
