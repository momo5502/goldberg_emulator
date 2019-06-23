#include "item_db_loader.h"

#include <fstream>
#include "json.h"

std::map<SteamItemDef_t, std::map<std::string, std::string>> read_items_db(std::string const& items_db)
{
    std::map<SteamItemDef_t, std::map<std::string, std::string>> items;
   
    std::ifstream items_file(items_db);
    items_file.seekg(0, std::ios::end);
    size_t size = items_file.tellg();
    std::string buffer(size, '\0');
    items_file.seekg(0);
    items_file.read(&buffer[0], size);
    items_file.close();

    try
    {
        nemir::json_value val = nemir::parse_json(buffer);
        if (val.type() == nemir::json_type::object)
        {
            for (auto& i : static_cast<nemir::json_value::json_object&>(val))
            {
                SteamItemDef_t key = std::stoi(i.first);
                if (i.second.type() == nemir::json_type::object)
                {
                    for (auto& j : static_cast<nemir::json_value::json_object&>(i.second))
                    {
                        items[key][j.first] = j.second;
                    }
                }
            }
        }
    }
    catch (std::exception& e)
    {
        // Error while parsing json items db
        PRINT_DEBUG(e.what());
        items.clear();
    }

    return items;
}