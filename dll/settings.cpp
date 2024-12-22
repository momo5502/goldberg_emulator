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

#include "settings.h"
#include "dll.h"

std::string Settings::sanitize(std::string name)
{
    name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());
    name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());

    for (auto& i : name)
    {
        if (!isprint(i))
            i = ' ';
    }

    return name;
}

Settings::Settings(CSteamID steam_id, CGameID game_id, std::string name, std::string language, bool offline)
{
    this->steam_id = steam_id;
    this->game_id = game_id;
    this->name = sanitize(name);
    if (this->name.size() == 0) {
        this->name = "  ";
    }

    if (this->name.size() == 1) {
        this->name = this->name + " ";
    }

    this->ui_notification_position = "";

    auto lang = sanitize(language);
    std::transform(lang.begin(), lang.end(), lang.begin(), ::tolower);
    lang.erase(std::remove(lang.begin(), lang.end(), ' '), lang.end());
    this->language = lang;
    this->lobby_id = k_steamIDNil;
    this->unlockAllDLCs = true;

    this->offline = offline;
    this->create_unknown_leaderboards = true;

    this->next_free = 1;
    this->preferred_network_image_type = Image::JPG;

    this->background_thread_exit = false;
    PRINT_DEBUG("%s.\n", "Settings::Settings Creating new background_monitor thread");
    background_monitor_thread = std::thread(Settings::background_monitor_entry, this);
}

Settings::~Settings()
{
    bool wait = false;
    {
        std::lock_guard<std::recursive_mutex> lock(background_thread_mutex);
        if (background_monitor_thread.joinable()) {
            background_thread_exit = true;
            wait = true;
        }
    }
    if (wait) {
        background_monitor_thread.join();
    }
}

CSteamID Settings::get_local_steam_id()
{
    return steam_id;
}

CGameID Settings::get_local_game_id()
{
    return game_id;
}

const char *Settings::get_local_name()
{
    return name.c_str();
}

const char *Settings::get_language()
{
    return language.c_str();
}

void Settings::set_local_name(char *name)
{
    this->name = name;
}

void Settings::set_language(char *language)
{
    this->language = language;
}

void Settings::set_game_id(CGameID game_id)
{
    this->game_id = game_id;
}

void Settings::set_lobby(CSteamID lobby_id)
{
    this->lobby_id = lobby_id;
}

CSteamID Settings::get_lobby()
{
    return this->lobby_id;
}

void Settings::unlockAllDLC(bool value)
{
    this->unlockAllDLCs = value;
}

void Settings::addDLC(AppId_t appID, std::string name, bool available)
{
    auto f = std::find_if(DLCs.begin(), DLCs.end(), [&appID](DLC_entry const& item) { return item.appID == appID; });
    if (DLCs.end() != f) {
        f->name = name;
        f->available = available;
        return;
    }

    DLC_entry new_entry;
    new_entry.appID = appID;
    new_entry.name = name;
    new_entry.available = available;
    DLCs.push_back(new_entry);
}

void Settings::addMod(PublishedFileId_t id, std::string title, std::string path)
{
    auto f = std::find_if(mods.begin(), mods.end(), [&id](Mod_entry const& item) { return item.id == id; });
    if (mods.end() != f) {
        f->title = title;
        f->path = path;
        return;
    }

    Mod_entry new_entry;
    new_entry.id = id;
    new_entry.title = title;
    new_entry.path = path;
    mods.push_back(new_entry);
}

Mod_entry Settings::getMod(PublishedFileId_t id)
{
    auto f = std::find_if(mods.begin(), mods.end(), [&id](Mod_entry const& item) { return item.id == id; });
    if (mods.end() != f) {
        return *f;
    }

    return Mod_entry();
}

bool Settings::isModInstalled(PublishedFileId_t id)
{
    auto f = std::find_if(mods.begin(), mods.end(), [&id](Mod_entry const& item) { return item.id == id; });
    if (mods.end() != f) {
        return true;
    }

    return false;
}

std::set<PublishedFileId_t> Settings::modSet()
{
    std::set<PublishedFileId_t> ret_set;

    for (auto & m: mods) {
        ret_set.insert(m.id);
    }

    return ret_set;
}

unsigned int Settings::DLCCount()
{
    return this->DLCs.size();
}

bool Settings::hasDLC(AppId_t appID)
{
    if (this->unlockAllDLCs) return true;

    auto f = std::find_if(DLCs.begin(), DLCs.end(), [&appID](DLC_entry const& item) { return item.appID == appID; });
    if (DLCs.end() == f)
        return false;

    return f->available;
}

bool Settings::getDLC(unsigned int index, AppId_t &appID, bool &available, std::string &name)
{
    if (index >= DLCs.size()) return false;

    appID = DLCs[index].appID;
    available = DLCs[index].available;
    name = DLCs[index].name;
    return true;
}

void Settings::setAppInstallPath(AppId_t appID, std::string path)
{
    app_paths[appID] = path;
}

std::string Settings::getAppInstallPath(AppId_t appID)
{
    return app_paths[appID];
}

void Settings::setLeaderboard(std::string leaderboard, enum ELeaderboardSortMethod sort_method, enum ELeaderboardDisplayType display_type)
{
    Leaderboard_config leader;
    leader.sort_method = sort_method;
    leader.display_type = display_type;

    leaderboards[leaderboard] = leader;
}

int Settings::find_next_free_image_ref() {
    int ret = 0;

    std::lock_guard<std::recursive_mutex> lock(images_mutex);

    if (images.size() == 0) {
        ret = 1;
    } else {
        for (auto x : images) {
            auto y = images.upper_bound(x.first);
            if (y == images.end()) {
                if ((x.first + 1) == 0) {
                    ret = 1;
                    break;
                } else {
                    ret = x.first + 1;
                    break;
                }
            } else {
                if ((x.first + 1) < y->first) {
                    if ((x.first + 1) != 0) {
                        ret = x.first + 1;
                        break;
                    }
                }
            }
        }
    }

    next_free = ret;

    return ret;
}

int Settings::remove_image(int ref) {
    int ret = 0;

    std::lock_guard<std::recursive_mutex> lock(images_mutex);

    auto x = images.find(ref);
    if (x != images.end()) {
        images.erase(x);
        ret = find_next_free_image_ref();
    } else {
        ret = next_free;
    }

    return ret;
}

int Settings::replace_image(int ref, std::string data, uint32 width, uint32 height)
{
    std::lock_guard<std::recursive_mutex> lock(images_mutex);
    int ret = 0;
    if (ref == 0) {
        ret = add_image(data, width, height);
    } else {
        auto t = images.find(ref);
        if (t != images.end()) {
            ret = t->first;
            t->second.data = data;
            t->second.width = width;
           t->second.height = height;
        } else {
            ret = add_image(data, width, height);
        }
    }
    return ret;
}

int Settings::add_image(std::string data, uint32 width, uint32 height)
{
    int last = 0;
    std::lock_guard<std::recursive_mutex> lock(images_mutex);
    if (next_free != 0) {
        last = next_free;
        struct Image_Data dt;
        dt.width = width;
        dt.height = height;
        dt.data = data;
        images[last] = dt;
        find_next_free_image_ref();
    } else {
        PRINT_DEBUG("%s.\n",
                    "Settings::add_image failed. Buffer is full");
    }
    return last;
}

int Settings::get_image(int ref, std::string * data, uint32 * width, uint32 * height)
{
    std::lock_guard<std::recursive_mutex> lock(images_mutex);
    int ret = 0;
    auto t = images.find(ref);
    if (t != images.end()) {
        ret = t->first;
        if (data != NULL) {
            *data = t->second.data;
        }
        if (width != NULL) {
            *width = t->second.width;
        }
        if (height != NULL) {
            *height = t->second.height;
        }
    } else {
        ret = 0;
        if (width != NULL) {
            *width = 0;
        }
        if (height != NULL) {
            *height = 0;
        }
    }
    return ret;
}

int Settings::get_profile_image(int eAvatarSize)
{
    std::lock_guard<std::recursive_mutex> lock(images_mutex);
    int ret = 0;
    for (auto i : profile_images) {
        if (i.first == eAvatarSize) {
            ret = i.second;
            break;
        }
    }
    return ret;
}

void Settings::background_monitor_entry(Settings * settings) {
    PRINT_DEBUG("%s.\n", "Settings::background_monitor_entry thread starting");
    if (settings != NULL) {
        settings->background_monitor();
    }
    PRINT_DEBUG("%s.\n", "Settings::background_monitor_entry thread exiting");
    return;
}

#define BACKGROUND_MONITOR_RATE 200

void Settings::background_monitor() {
    bool exit = false;
    do {
        {
            std::lock_guard<std::recursive_mutex> lock(background_thread_mutex);
            exit = background_thread_exit;
            if (!exit) {
                if (background_tasks.size() > 0) {
                    for (auto x = background_tasks.begin(); x != background_tasks.end(); x++) {
                        bool task_done = false;
                        Steam_Client * client = try_get_steam_client();
                        if (client != NULL) {
                            switch (x->id) {
                                case Settings_Background_Task_IDs::NOTIFY_AVATAR_IMAGE:
                                    {
                                        PRINT_DEBUG("%s.\n", "Settings::background_monitor Got NOTIFY_AVATAR_IMAGE task");

                                        if (client != NULL && client->steam_friends != NULL) {
                                            if (disable_overlay == true ||
                                                (client->steam_overlay != NULL &&
                                                 client->steam_overlay->RegisteredInternalCallbacks() == true)) {
                                                client->steam_friends->GetFriendAvatar(this->steam_id, k_EAvatarSize32x32);
                                                client->steam_friends->GetFriendAvatar(this->steam_id, k_EAvatarSize64x64);
                                                client->steam_friends->GetFriendAvatar(this->steam_id, k_EAvatarSize184x184);
                                                task_done = true;
                                            }
                                        }
                                    }
                                    break;
                                default:
                                    PRINT_DEBUG("%s %d.\n", "Settings::background_monitor Unknown task", x->id);
                                    task_done = true;
                                    break;
                            };
                            if (task_done) {
                                background_tasks.erase(x);
                                break;
                            }
                        }
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(BACKGROUND_MONITOR_RATE));
            }
        }
    } while(!exit);
    return;
}

void Settings::create_background_notify_task(Settings_Background_Task_IDs id, void *arg = NULL) {
    std::lock_guard<std::recursive_mutex> lock(background_thread_mutex);
    background_tasks.push_back(Settings_Background_Task{id, arg});
    return;
}

int Settings::set_profile_image(int eAvatarSize, Image_Data * image)
{
    bool size_ok = false;
    int ref = 0;

    if (image != NULL) {
        if (eAvatarSize == k_EAvatarSize32x32 && image->width > 0 && image->width <= 32 && image->height > 0 && image->height <= 32)
            size_ok = true;
        if (eAvatarSize == k_EAvatarSize64x64 && image->width > 32 && image->width <= 64 && image->height > 32 && image->height <= 64)
            size_ok = true;
        if (eAvatarSize == k_EAvatarSize184x184 && image->width > 64 && image->width <= 184 && image->height > 64 && image->height <= 184)
            size_ok = true;

        if (size_ok == true && image->data.length() > 0) {
            ref = this->add_image(image->data, image->width, image->height);
            PRINT_DEBUG("Settings::set_profile_image %d -> %d.\n", eAvatarSize, ref);
            std::lock_guard<std::recursive_mutex> lock(images_mutex);
            profile_images[eAvatarSize] = ref;
            create_background_notify_task(Settings_Background_Task_IDs::NOTIFY_AVATAR_IMAGE, NULL);
        } else {
            PRINT_DEBUG("%s %d %dx%d %" PRI_ZU ".\n",
                        "Settings::set_profile_image failed",
                        eAvatarSize,
                        image->width,
                        image->height,
                        image->data.length());
        }
    }

    return ref;
}

void Settings::set_preferred_network_image_type(int new_type)
{
    switch (new_type) {
        case Image::RAW:
        case Image::PNG:
        case Image::JPG:
            this->preferred_network_image_type = new_type;
            break;
        default:
            PRINT_DEBUG("%s %d.\n",
                        "Settings::set_preferred_network_image_type failed. Requested type",
                        new_type);
            break;
    };
    return;
}

std::string Settings::get_ui_notification_position()
{
    return this->ui_notification_position;
}

void Settings::set_ui_notification_position(char * pos)
{
    PRINT_DEBUG("%s 0x%p.\n",
                "Settings::set_ui_notification_position",
                pos);
    if (pos != NULL) {
        size_t len = strlen(pos);
        if (len > 0) {
            this->ui_notification_position = "";
            for (size_t x = 0; x < len && pos[x] != '\0'; x++) {
                this->ui_notification_position += pos[x];
            }
        }
    }
}
