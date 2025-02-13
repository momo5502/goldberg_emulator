/* Copyright (C) 2019 Mr Goldberg
   This file is part of the Goldberg Emulator

   The Goldberg Emulator is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   The Goldberg Emulator is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Goldberg Emulator; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "settings_parser.h"

static void consume_bom(std::ifstream &input)
{
    int bom[3];
    bom[0] = input.get();
    bom[1] = input.get();
    bom[2] = input.get();
    if (bom[0] != 0xEF || bom[1] != 0xBB || bom[2] != 0xBF) {
        input.seekg(0);
    }
}

static void load_custom_broadcasts(std::string broadcasts_filepath, std::set<IP_PORT> &custom_broadcasts)
{
    PRINT_DEBUG("Broadcasts file path: %s\n", broadcasts_filepath.c_str());
    std::ifstream broadcasts_file(utf8_decode(broadcasts_filepath));
    consume_bom(broadcasts_file);
    if (broadcasts_file.is_open()) {
        std::string line;
        while (std::getline(broadcasts_file, line)) {
            std::set<IP_PORT> ips = Networking::resolve_ip(line);
            custom_broadcasts.insert(ips.begin(), ips.end());
        }
    }
}

template<typename Out>
static void split_string(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

static void load_gamecontroller_settings(Settings *settings)
{
    std::string path = Local_Storage::get_game_settings_path() + "controller";
    std::vector<std::string> paths = Local_Storage::get_filenames_path(path);

    for (auto & p: paths) {
        size_t length = p.length();
        if (length < 4) continue;
        if ( std::toupper(p.back()) != 'T') continue;
        if ( std::toupper(p[length - 2]) != 'X') continue;
        if ( std::toupper(p[length - 3]) != 'T') continue;
        if (p[length - 4] != '.') continue;

        PRINT_DEBUG("controller config %s\n", p.c_str());
        std::string action_set_name = p.substr(0, length - 4);
        std::transform(action_set_name.begin(), action_set_name.end(), action_set_name.begin(),[](unsigned char c){ return std::toupper(c); });

        std::string controller_config_path = path + PATH_SEPARATOR + p;
        std::ifstream input( utf8_decode(controller_config_path) );
        if (input.is_open()) {
            consume_bom(input);
            std::map<std::string, std::pair<std::set<std::string>, std::string>> button_pairs;

            for( std::string line; getline( input, line ); ) {
                if (!line.empty() && line[line.length()-1] == '\n') {
                    line.pop_back();
                }

                if (!line.empty() && line[line.length()-1] == '\r') {
                    line.pop_back();
                }

                std::string action_name;
                std::string button_name;
                std::string source_mode;

                std::size_t deliminator = line.find("=");
                if (deliminator != 0 && deliminator != std::string::npos && deliminator != line.size()) {
                    action_name = line.substr(0, deliminator);
                    std::size_t deliminator2 = line.find("=", deliminator + 1);

                    if (deliminator2 != std::string::npos && deliminator2 != line.size()) {
                        button_name = line.substr(deliminator + 1, deliminator2 - (deliminator + 1));
                        source_mode = line.substr(deliminator2 + 1);
                    } else {
                        button_name = line.substr(deliminator + 1);
                        source_mode = "";
                    }
                }

                std::transform(action_name.begin(), action_name.end(), action_name.begin(),[](unsigned char c){ return std::toupper(c); });
                std::transform(button_name.begin(), button_name.end(), button_name.begin(),[](unsigned char c){ return std::toupper(c); });
                std::pair<std::set<std::string>, std::string> button_config = {{}, source_mode};
                split_string(button_name, ',', std::inserter(button_config.first, button_config.first.begin()));
                button_pairs[action_name] = button_config;
                PRINT_DEBUG("Added %s %s %s\n", action_name.c_str(), button_name.c_str(), source_mode.c_str());
            }

            settings->controller_settings.action_sets[action_set_name] = button_pairs;
            PRINT_DEBUG("Added %" PRI_ZU " action names to %s\n", button_pairs.size(), action_set_name.c_str());
        }
    }

    settings->glyphs_directory = path + (PATH_SEPARATOR "glyphs" PATH_SEPARATOR);
}

uint32 create_localstorage_settings(Settings **settings_client_out, Settings **settings_server_out, Local_Storage **local_storage_out)
{
    std::string program_path = Local_Storage::get_program_path(), save_path = Local_Storage::get_user_appdata_path();;

    PRINT_DEBUG("Current Path %s save_path: %s\n", program_path.c_str(), save_path.c_str());

    char array[10] = {};
    array[0] = '0';
    Local_Storage::get_file_data(Local_Storage::get_game_settings_path() + "steam_appid.txt", array, sizeof(array) - 1);
    uint32 appid = 0;
    try {
        appid = std::stoi(array);
    } catch (...) {}
    if (!appid) {
        memset(array, 0, sizeof(array));
        array[0] = '0';
        Local_Storage::get_file_data("steam_appid.txt", array, sizeof(array) - 1);
        try {
            appid = std::stoi(array);
        } catch (...) {}
        if (!appid) {
            memset(array, 0, sizeof(array));
            array[0] = '0';
            Local_Storage::get_file_data(program_path + "steam_appid.txt", array, sizeof(array) - 1);
            try {
                appid = std::stoi(array);
            } catch (...) {}
        }
    }

    if (!appid) {
        std::string str_appid = get_env_variable("SteamAppId");
        std::string str_gameid = get_env_variable("SteamGameId");
        PRINT_DEBUG("str_appid %s str_gameid: %s\n", str_appid.c_str(), str_gameid.c_str());
        uint32 appid_env = 0;
        uint32 gameid_env = 0;

        if (str_appid.size() > 0) {
            try {
                appid_env = std::stoul(str_appid);
            } catch (...) {
                appid_env = 0;
            }
        }

        if (str_gameid.size() > 0) {
            try {
                gameid_env = std::stoul(str_gameid);
            } catch (...) {
                gameid_env = 0;
            }
        }

        PRINT_DEBUG("appid_env %u gameid_env: %u\n", appid_env, gameid_env);
        if (appid_env) {
            appid = appid_env;
        }

        if (gameid_env) {
            appid = gameid_env;
        }
    }

    bool local_save = false;

    {
        char array[33] = {};
        if (Local_Storage::get_file_data(program_path + "local_save.txt", array, sizeof(array) - 1) != -1) {
            save_path = program_path + Settings::sanitize(array);
            local_save = true;
        }
    }

    PRINT_DEBUG("Set save_path: %s\n", save_path.c_str());
    Local_Storage *local_storage = new Local_Storage(save_path);
    local_storage->setAppId(appid);

    // Listen port
    char array_port[10] = {};
    array_port[0] = '0';
    local_storage->get_data_settings("listen_port.txt", array_port, sizeof(array_port) - 1);
    uint16 port = std::stoi(array_port);
    if (port == 0) {
        port = DEFAULT_PORT;
        snprintf(array_port, sizeof(array_port), "%hu", port);
        local_storage->store_data_settings("listen_port.txt", array_port, strlen(array_port));
    }


    // Custom broadcasts
    std::set<IP_PORT> custom_broadcasts;
    load_custom_broadcasts(local_storage->get_global_settings_path() + "custom_broadcasts.txt", custom_broadcasts);
    load_custom_broadcasts(Local_Storage::get_game_settings_path() + "custom_broadcasts.txt", custom_broadcasts);

    // Custom master server
    std::set<IP_PORT> custom_master_server;
    load_custom_broadcasts(local_storage->get_global_settings_path() + "custom_master_server.txt", custom_master_server);
    load_custom_broadcasts(Local_Storage::get_game_settings_path() + "custom_master_server.txt", custom_master_server);

    // Acount name
    char name[32] = { '\0' };
    if (local_storage->get_data_settings("account_name.txt", name, sizeof(name) - 1) <= 0) {
        PRINT_DEBUG("%s.\n", "Attempting to set steam user name from system user name");
#if defined(STEAM_WIN32)
        DWORD username_dword = 32;
        wchar_t username[32] = { '\0' };
        if (GetUserNameW((wchar_t*)&username, &username_dword) == TRUE) {
            std::wstring username_wstr(username);
            std::string username_str = utf8_encode(username_wstr);
            size_t username_len = username_str.length();
            if (username_len > 0 &&
                username_len < 31) {
                memcpy(&name, username_str.c_str(), username_len);
                name[31] = '\0';
            }
        }
#else
        char * env_username = getenv("USER");
        if (env_username != NULL) {
            size_t username_len = strlen(env_username);
            if (username_len > 0 &&
                username_len < 31) {
                memcpy(&name, env_username, username_len);
                name[31] = '\0';
            }
        }
#endif
        char empty_name[32] = { '\0' };
        if (memcmp(name, empty_name, 32) == 0) {
            PRINT_DEBUG("%s %s.\n", "Setting steam user name to", DEFAULT_NAME);
            strcpy(name, DEFAULT_NAME);
        }
        PRINT_DEBUG("Username: %s.\n", name);
        local_storage->store_data_settings("account_name.txt", name, strlen(name));
    }

    // Language
    char language[32] = {};
    if (local_storage->get_data_settings("language.txt", language, sizeof(language) - 1) <= 0) {
        strcpy(language, DEFAULT_LANGUAGE);
        local_storage->store_data_settings("language.txt", language, strlen(language));
    }

    // UI Notification Position.
    char notification_position[32] = {};
    if (local_storage->get_data_settings("ui_notification_position.txt", notification_position, sizeof(notification_position) - 1) <= 0) {
        strcpy(notification_position, DEFAULT_UI_NOTIFICATION_POSITION);
        local_storage->store_data_settings("ui_notification_position.txt", notification_position, strlen(notification_position));
    }

    // Steam ID
    char array_steam_id[32] = {};
    CSteamID user_id;
    uint64 steam_id = 0;
    bool generate_new = false;
    //try to load steam id from game specific settings folder first
    if (local_storage->get_data(Local_Storage::settings_storage_folder, "user_steam_id.txt", array_steam_id, sizeof(array_steam_id) - 1) > 0) {
        user_id = CSteamID((uint64)std::atoll(array_steam_id));
        if (!user_id.IsValid()) {
            generate_new = true;
        }
    } else {
        generate_new = true;
    }

    if (generate_new) {
        generate_new = false;
        if (local_storage->get_data_settings("user_steam_id.txt", array_steam_id, sizeof(array_steam_id) - 1) > 0) {
            user_id = CSteamID((uint64)std::atoll(array_steam_id));
            if (!user_id.IsValid()) {
                generate_new = true;
            }
        } else {
            generate_new = true;
        }
    }

    if (generate_new) {
        user_id = generate_steam_id_user();
        uint64 steam_id = user_id.ConvertToUint64();
        char temp_text[32] = {};
        snprintf(temp_text, sizeof(temp_text), "%llu", steam_id);
        local_storage->store_data_settings("user_steam_id.txt", temp_text, strlen(temp_text));
    }

    if (Local_Storage::is_directory(Local_Storage::get_user_appdata_path().append(PATH_SEPARATOR).append("minidumps")) == false) {
        local_storage->store_data_settings(std::string("minidumps").append(PATH_SEPARATOR).append("dummy.txt"),
                                           " ",
                                           sizeof(" ") / sizeof(char));
    }

    std::set<std::string> supported_languages;

    {
        std::string lang_config_path = Local_Storage::get_game_settings_path() + "supported_languages.txt";
        std::ifstream input( utf8_decode(lang_config_path) );

        std::string first_language;
        if (input.is_open()) {
            consume_bom(input);
            for( std::string line; getline( input, line ); ) {
                if (!line.empty() && line[line.length()-1] == '\n') {
                    line.pop_back();
                }

                if (!line.empty() && line[line.length()-1] == '\r') {
                    line.pop_back();
                }

                try {
                    std::string lang = line;
                    if (!first_language.size()) first_language = lang;
                    supported_languages.insert(lang);
                    PRINT_DEBUG("Added supported_language %s\n", lang.c_str());
                } catch (...) {}
            }
        }

        if (!supported_languages.count(language)) {
            if (first_language.size()) {
                memset(language, 0, sizeof(language));
                first_language.copy(language, sizeof(language) - 1);
            }
        }
    }

    bool steam_offline_mode = false;
    bool disable_networking = false;
    bool disable_overlay = false;
    bool disable_lobby_creation = false;
    bool enable_achievement_desc_on_unlock = false;
    bool enable_displaying_hidden_achievements = false;
    int build_id = 10;

    bool warn_forced = false;

    Image_Data profile_small {0, 0, std::string()};
    Image_Data profile_medium {0, 0, std::string()};
    Image_Data profile_large {0, 0, std::string()};

    {
        std::string steam_settings_path = local_storage->get_global_settings_path();

        std::vector<std::string> paths = local_storage->get_filenames_path(steam_settings_path);
        for (auto & p: paths) {
            PRINT_DEBUG("global settings path %s\n", p.c_str());
            if (p == "enable_achievement_desc_on_unlock.txt") {
                enable_achievement_desc_on_unlock = true;
            } else if (p == "enable_displaying_hidden_achievements.txt") {
                enable_displaying_hidden_achievements = true;
            } else if (p == "avatar_small.jpg") {
                profile_small.data = convert_vector_image_pixel_t_to_std_string(local_storage->load_image(steam_settings_path + p, &profile_small.width, &profile_small.height));
            } else if (p == "avatar_medium.jpg") {
                profile_medium.data = convert_vector_image_pixel_t_to_std_string(local_storage->load_image(steam_settings_path + p, &profile_medium.width, &profile_medium.height));
            } else if (p == "avatar_large.jpg") {
                profile_large.data = convert_vector_image_pixel_t_to_std_string(local_storage->load_image(steam_settings_path + p, &profile_large.width, &profile_large.height));
            } else if (p == "avatar_small.png") {
                profile_small.data = convert_vector_image_pixel_t_to_std_string(local_storage->load_image(steam_settings_path + p, &profile_small.width, &profile_small.height));
            } else if (p == "avatar_medium.png") {
                profile_medium.data = convert_vector_image_pixel_t_to_std_string(local_storage->load_image(steam_settings_path + p, &profile_medium.width, &profile_medium.height));
            } else if (p == "avatar_large.png") {
                profile_large.data = convert_vector_image_pixel_t_to_std_string(local_storage->load_image(steam_settings_path + p, &profile_large.width, &profile_large.height));
            }
        }
    }

    {
        std::string steam_settings_path = Local_Storage::get_game_settings_path();

        std::vector<std::string> paths = Local_Storage::get_filenames_path(steam_settings_path);
        for (auto & p: paths) {
            PRINT_DEBUG("steam settings path %s\n", p.c_str());
            if (p == "offline.txt") {
                steam_offline_mode = true;
            } else if (p == "disable_networking.txt") {
                disable_networking = true;
            } else if (p == "disable_overlay.txt") {
                disable_overlay = true;
            } else if (p == "disable_lobby_creation.txt") {
                disable_lobby_creation = true;
            } else if (p == "enable_achievement_desc_on_unlock.txt") {
                enable_achievement_desc_on_unlock = true;
            } else if (p == "enable_displaying_hidden_achievements.txt") {
                enable_displaying_hidden_achievements = true;
            } else if (p == "force_language.txt") {
                int len = Local_Storage::get_file_data(steam_settings_path + "force_language.txt", language, sizeof(language) - 1);
                if (len > 0) {
                    language[len] = 0;
                    warn_forced = true;
                }
            } else if (p == "force_ui_notification_position.txt") {
                int len = Local_Storage::get_file_data(steam_settings_path + "force_ui_notification_position.txt", notification_position, sizeof(notification_position) - 1);
                if (len > 0) {
                    notification_position[len] = 0;
                    warn_forced = true;
                }
            } else if (p == "force_steamid.txt") {
                char steam_id_text[32] = {};
                if (Local_Storage::get_file_data(steam_settings_path + "force_steamid.txt", steam_id_text, sizeof(steam_id_text) - 1) > 0) {
                    CSteamID temp_id = CSteamID((uint64)std::atoll(steam_id_text));
                    if (temp_id.IsValid()) {
                        user_id = temp_id;
                        warn_forced = true;
                    }
                }
            } else if (p == "force_account_name.txt") {
                int len = Local_Storage::get_file_data(steam_settings_path + "force_account_name.txt", name, sizeof(name) - 1);
                if (len > 0) {
                    name[len] = 0;
                    warn_forced = true;
                }
            } else if (p == "force_listen_port.txt") {
                char array_port[10] = {};
                int len = Local_Storage::get_file_data(steam_settings_path + "force_listen_port.txt", array_port, sizeof(array_port) - 1);
                if (len > 0) {
                    port = std::stoi(array_port);
                    warn_forced = true;
                }
            } else if (p == "build_id.txt") {
                char array_id[10] = {};
                int len = Local_Storage::get_file_data(steam_settings_path + "build_id.txt", array_id, sizeof(array_id) - 1);
                if (len > 0) build_id = std::stoi(array_id);
            }
        }
    }

    Settings *settings_client = new Settings(user_id, CGameID(appid), name, language, steam_offline_mode);
    Settings *settings_server = new Settings(generate_steam_id_server(), CGameID(appid), name, language, steam_offline_mode);
    settings_client->set_port(port);
    settings_server->set_port(port);
    settings_client->custom_broadcasts = custom_broadcasts;
    settings_server->custom_broadcasts = custom_broadcasts;
    settings_client->custom_master_server = custom_master_server;
    settings_server->custom_master_server = custom_master_server;
    settings_client->disable_networking = disable_networking;
    settings_server->disable_networking = disable_networking;
    settings_client->disable_overlay = disable_overlay;
    settings_server->disable_overlay = disable_overlay;
    settings_client->disable_lobby_creation = disable_lobby_creation;
    settings_server->disable_lobby_creation = disable_lobby_creation;
    settings_client->build_id = build_id;
    settings_server->build_id = build_id;
    settings_client->warn_forced = warn_forced;
    settings_server->warn_forced = warn_forced;
    settings_client->warn_local_save = local_save;
    settings_server->warn_local_save = local_save;
    settings_client->supported_languages = supported_languages;
    settings_server->supported_languages = supported_languages;
    settings_client->set_show_achievement_desc_on_unlock(enable_achievement_desc_on_unlock);
    settings_server->set_show_achievement_desc_on_unlock(enable_achievement_desc_on_unlock);
    settings_client->set_show_achievement_hidden_unearned(enable_displaying_hidden_achievements);
    settings_server->set_show_achievement_hidden_unearned(enable_displaying_hidden_achievements);
    settings_client->set_ui_notification_position(notification_position);
    settings_server->set_ui_notification_position(notification_position);
    if (profile_small.data.length() > 0 && profile_small.width > 0 && profile_small.height > 0) {
        settings_client->set_profile_image(k_EAvatarSize32x32, &profile_small);
        settings_server->set_profile_image(k_EAvatarSize32x32, &profile_small);
    } else {
        PRINT_DEBUG("%s %" PRI_ZU " %d %d\n", "Small user avatar image not defined.", profile_small.data.length(), profile_small.width, profile_small.height);
    }
    if (profile_medium.data.length() > 0 && profile_medium.width > 0 && profile_medium.height > 0) {
        settings_client->set_profile_image(k_EAvatarSize64x64, &profile_medium);
        settings_server->set_profile_image(k_EAvatarSize64x64, &profile_medium);
    } else {
        PRINT_DEBUG("%s %" PRI_ZU " %d %d\n", "Medium user avatar image not defined.", profile_medium.data.length(), profile_medium.width, profile_medium.height);
    }
    if (profile_large.data.length() > 0 && profile_large.width > 0 && profile_large.height > 0) {
        settings_client->set_profile_image(k_EAvatarSize184x184, &profile_large);
        settings_server->set_profile_image(k_EAvatarSize184x184, &profile_large);
    } else {
        PRINT_DEBUG("%s %" PRI_ZU " %d %d\n", "Large user avatar image not defined.", profile_large.data.length(), profile_large.width, profile_large.height);
    }

    {
        std::string dlc_config_path = Local_Storage::get_game_settings_path() + "DLC.txt";
        std::ifstream input( utf8_decode(dlc_config_path) );
        if (input.is_open()) {
            consume_bom(input);
            settings_client->unlockAllDLC(false);
            settings_server->unlockAllDLC(false);
            PRINT_DEBUG("Locking all DLC\n");

            for( std::string line; std::getline( input, line ); ) {
                if (!line.empty() && line.front() == '#') {
                    continue;
                }
                if (!line.empty() && line.back() == '\n') {
                    line.pop_back();
                }

                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                std::size_t deliminator = line.find("=");
                if (deliminator != 0 && deliminator != std::string::npos && deliminator != line.size()) {
                    AppId_t appid = stol(line.substr(0, deliminator));
                    std::string name = line.substr(deliminator + 1);
                    bool available = true;

                    if (appid) {
                        PRINT_DEBUG("Adding DLC: %u|%s| %u\n", appid, name.c_str(), available);
                        settings_client->addDLC(appid, name, available);
                        settings_server->addDLC(appid, name, available);
                    }
                }
            }
        } else {
            //unlock all DLC
            PRINT_DEBUG("Unlocking all DLC\n");
            settings_client->unlockAllDLC(true);
            settings_server->unlockAllDLC(true);
        }
    }

    {
        std::string dlc_config_path = Local_Storage::get_game_settings_path() + "app_paths.txt";
        std::ifstream input( utf8_decode(dlc_config_path) );

        if (input.is_open()) {
            consume_bom(input);
            for( std::string line; getline( input, line ); ) {
                if (!line.empty() && line[line.length()-1] == '\n') {
                    line.pop_back();
                }

                if (!line.empty() && line[line.length()-1] == '\r') {
                    line.pop_back();
                }

                std::size_t deliminator = line.find("=");
                if (deliminator != 0 && deliminator != std::string::npos && deliminator != line.size()) {
                    AppId_t appid = stol(line.substr(0, deliminator));
                    std::string rel_path = line.substr(deliminator + 1);
                    std::string path = canonical_path(program_path + rel_path);

                    if (appid) {
                        if (path.size()) {
                            PRINT_DEBUG("Adding app path: %u|%s|\n", appid, path.c_str());
                            settings_client->setAppInstallPath(appid, path);
                            settings_server->setAppInstallPath(appid, path);
                        } else {
                            PRINT_DEBUG("Error adding app path for: %u does this path exist? |%s|\n", appid, rel_path.c_str());
                        }
                    }
                }
            }
        }
    }

    {
        std::string dlc_config_path = Local_Storage::get_game_settings_path() + "leaderboards.txt";
        std::ifstream input( utf8_decode(dlc_config_path) );
        if (input.is_open()) {
            consume_bom(input);
            settings_client->setCreateUnknownLeaderboards(false);
            settings_server->setCreateUnknownLeaderboards(false);

            for( std::string line; getline( input, line ); ) {
                if (!line.empty() && line[line.length()-1] == '\n') {
                    line.pop_back();
                }

                if (!line.empty() && line[line.length()-1] == '\r') {
                    line.pop_back();
                }

                std::string leaderboard;
                unsigned int sort_method = 0;
                unsigned int display_type = 0;

                std::size_t deliminator = line.find("=");
                if (deliminator != 0 && deliminator != std::string::npos && deliminator != line.size()) {
                    leaderboard = line.substr(0, deliminator);
                    std::size_t deliminator2 = line.find("=", deliminator + 1);
                    if (deliminator2 != std::string::npos && deliminator2 != line.size()) {
                        sort_method = stol(line.substr(deliminator + 1, deliminator2 - (deliminator + 1)));
                        display_type = stol(line.substr(deliminator2 + 1));
                    }
                }

                if (leaderboard.size() && sort_method <= k_ELeaderboardSortMethodDescending && display_type <= k_ELeaderboardDisplayTypeTimeMilliSeconds) {
                    PRINT_DEBUG("Adding leaderboard: %s|%u|%u\n", leaderboard.c_str(), sort_method, display_type);
                    settings_client->setLeaderboard(leaderboard, (ELeaderboardSortMethod)sort_method, (ELeaderboardDisplayType)display_type);
                    settings_server->setLeaderboard(leaderboard, (ELeaderboardSortMethod)sort_method, (ELeaderboardDisplayType)display_type);
                } else {
                    PRINT_DEBUG("Error adding leaderboard for: %s, are sort method %u or display type %u valid?\n", leaderboard.c_str(), sort_method, display_type);
                }
            }
        }
    }

    {
        std::string stats_config_path = Local_Storage::get_game_settings_path() + "stats.txt";
        std::ifstream input( utf8_decode(stats_config_path) );
        if (input.is_open()) {
            consume_bom(input);
            for( std::string line; getline( input, line ); ) {
                if (!line.empty() && line[line.length()-1] == '\n') {
                    line.pop_back();
                }

                if (!line.empty() && line[line.length()-1] == '\r') {
                    line.pop_back();
                }

                std::string stat_name;
                std::string stat_type;
                std::string stat_default_value;

                std::size_t deliminator = line.find("=");
                if (deliminator != 0 && deliminator != std::string::npos && deliminator != line.size()) {
                    stat_name = line.substr(0, deliminator);
                    std::size_t deliminator2 = line.find("=", deliminator + 1);

                    if (deliminator2 != std::string::npos && deliminator2 != line.size()) {
                        stat_type = line.substr(deliminator + 1, deliminator2 - (deliminator + 1));
                        stat_default_value = line.substr(deliminator2 + 1);
                    } else {
                        stat_type = line.substr(deliminator + 1);
                        stat_default_value = "0";
                    }
                }

                std::transform(stat_type.begin(), stat_type.end(), stat_type.begin(),[](unsigned char c){ return std::tolower(c); });
                struct Stat_config config = {};

                try {
                    if (stat_type == "float") {
                        config.type = Stat_Type::STAT_TYPE_FLOAT;
                        config.default_value_float = std::stof(stat_default_value);
                    } else if (stat_type == "int") {
                        config.type = Stat_Type::STAT_TYPE_INT;
                        config.default_value_int = std::stol(stat_default_value);
                    } else if (stat_type == "avgrate") {
                        config.type = Stat_Type::STAT_TYPE_AVGRATE;
                        config.default_value_float = std::stof(stat_default_value);
                    } else {
                        PRINT_DEBUG("Error adding stat %s, type %s isn't valid\n", stat_name.c_str(), stat_type.c_str());
                        continue;
                    }
                } catch (...) {
                    PRINT_DEBUG("Error adding stat %s, default value %s isn't valid\n", stat_name.c_str(), stat_default_value.c_str());
                    continue;
                }

                if (stat_name.size()) {
                    PRINT_DEBUG("Adding stat type: %s|%u|%f|%u\n", stat_name.c_str(), config.type, config.default_value_float, config.default_value_int);
                    settings_client->setStatDefiniton(stat_name, config);
                    settings_server->setStatDefiniton(stat_name, config);
                } else {
                    PRINT_DEBUG("Error adding stat for: %s, empty name\n", stat_name.c_str());
                }
            }
        }
    }

    {
        std::string depots_config_path = Local_Storage::get_game_settings_path() + "depots.txt";
        std::ifstream input( utf8_decode(depots_config_path) );
        if (input.is_open()) {
            consume_bom(input);
            for( std::string line; getline( input, line ); ) {
                if (!line.empty() && line[line.length()-1] == '\n') {
                    line.pop_back();
                }

                if (!line.empty() && line[line.length()-1] == '\r') {
                    line.pop_back();
                }

                try {
                    DepotId_t depot_id = std::stoul(line);
                    settings_client->depots.push_back(depot_id);
                    settings_server->depots.push_back(depot_id);
                    PRINT_DEBUG("Added depot %u\n", depot_id);
                } catch (...) {}
            }
        }
    }

    {
        std::string depots_config_path = Local_Storage::get_game_settings_path() + "subscribed_groups.txt";
        std::ifstream input( utf8_decode(depots_config_path) );
        if (input.is_open()) {
            consume_bom(input);
            for( std::string line; getline( input, line ); ) {
                if (!line.empty() && line[line.length()-1] == '\n') {
                    line.pop_back();
                }

                if (!line.empty() && line[line.length()-1] == '\r') {
                    line.pop_back();
                }

                try {
                    uint64 source_id = std::stoull(line);
                    settings_client->subscribed_groups.insert(source_id);
                    settings_server->subscribed_groups.insert(source_id);
                    PRINT_DEBUG("Added source %llu\n", source_id);
                } catch (...) {}
            }
        }
    }

    {
        std::string mod_path = Local_Storage::get_game_settings_path() + "mods";
        std::vector<std::string> paths = Local_Storage::get_filenames_path(mod_path);
        for (auto & p: paths) {
            PRINT_DEBUG("mod directory %s\n", p.c_str());
            try {
                PublishedFileId_t id = std::stoull(p);
                settings_client->addMod(id, p, mod_path + PATH_SEPARATOR + p);
                settings_server->addMod(id, p, mod_path + PATH_SEPARATOR + p);
            } catch (...) {}
        }
    }

    load_gamecontroller_settings(settings_client);

    settings_client->set_settings_parser_done(true);
    settings_server->set_settings_parser_done(true);

    *settings_client_out = settings_client;
    *settings_server_out = settings_server;
    *local_storage_out = local_storage;

    reset_LastError();
    return appid;
}

void save_global_settings(Local_Storage *local_storage, Settings * client_settings)
{
    if ((local_storage != nullptr) && (client_settings != nullptr)) {
        std::string name = client_settings->get_local_name();
        std::string language = client_settings->get_language();
        std::string ui_notif_pos = client_settings->get_ui_notification_position();

        local_storage->store_data_settings("account_name.txt", (char*)name.c_str(), name.length());
        local_storage->store_data_settings("language.txt", (char*)language.c_str(), language.length());
        local_storage->store_data_settings("ui_notification_position.txt", (char*)ui_notif_pos.c_str(), ui_notif_pos.length());
        if (client_settings->get_show_achievement_desc_on_unlock()) {
            if (local_storage->data_settings_exists("enable_achievement_desc_on_unlock.txt") != true) {
                local_storage->store_data_settings("enable_achievement_desc_on_unlock.txt", " ", sizeof(" "));
            }
        } else {
            if (local_storage->data_settings_exists("enable_achievement_desc_on_unlock.txt") == true) {
                local_storage->delete_data_settings("enable_achievement_desc_on_unlock.txt");
            }
        }
        if (client_settings->get_show_achievement_hidden_unearned()) {
            if (local_storage->data_settings_exists("enable_displaying_hidden_achievements.txt") != true) {
                local_storage->store_data_settings("enable_displaying_hidden_achievements.txt", " ", sizeof(" "));
            }
        } else {
            if (local_storage->data_settings_exists("enable_displaying_hidden_achievements.txt") == true) {
                local_storage->delete_data_settings("enable_displaying_hidden_achievements.txt");
            }
        }
    }
}
