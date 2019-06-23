#ifndef __ITEM_DB_LOADER_INCLUDED__
#define __ITEM_DB_LOADER_INCLUDED__

#include "base.h"

std::map<SteamItemDef_t, std::map<std::string, std::string>> read_items_db(std::string const& items_db);

#endif//__ITEM_DB_LOADER_INCLUDED__