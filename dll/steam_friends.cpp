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

#include "steam_friends.h"
#include "dll.h"

void Steam_Friends::Callback(Common_Message *msg)
{
    if (msg->has_low_level()) {
        if (msg->low_level().type() == Low_Level::DISCONNECT) {
            PRINT_DEBUG("Steam_Friends::Callback Disconnect\n");
            uint64 id = msg->source_id();
            auto f = std::find_if(friends.begin(), friends.end(), [&id](Friend const& item) { return item.id() == id; });
            if (friends.end() != f) {
                persona_change((uint64)f->id(), k_EPersonaChangeStatus);
                overlay->FriendDisconnect(*f);
                if ((uint64)f->id() != settings->get_local_steam_id().ConvertToUint64()) {
                    auto nums = avatars.find((uint64)f->id());
                    if (nums != avatars.end()) {
                        if (nums->second.smallest != 0) {
                            settings->remove_image(nums->second.smallest);
                        }
                        if (nums->second.medium != 0) {
                            settings->remove_image(nums->second.medium);
                        }
                        if (nums->second.large != 0) {
                            settings->remove_image(nums->second.large);
                        }
                        avatars.erase(nums);
                    }
                    friends.erase(f);
                }
            }
        }

        if (msg->low_level().type() == Low_Level::CONNECT) {
            PRINT_DEBUG("Steam_Friends::Callback Connect\n");
            Common_Message msg_;
            msg_.set_source_id(settings->get_local_steam_id().ConvertToUint64());
            msg_.set_dest_id(msg->source_id());
            Friend *f = new Friend(us);
            f->set_id(settings->get_local_steam_id().ConvertToUint64());
            f->set_name(settings->get_local_name());
            f->set_appid(settings->get_local_game_id().AppID());
            f->set_lobby_id(settings->get_lobby().ConvertToUint64());
            msg_.set_allocated_friend_(f);
            network->sendTo(&msg_, true);
        }
    }

    if (msg->has_friend_()) {
        PRINT_DEBUG("Steam_Friends::Callback Friend %llu %llu\n", msg->friend_().id(), msg->friend_().lobby_id());
        Friend *f = find_friend((uint64)msg->friend_().id());
        if (!f) {
            if (msg->friend_().id() != settings->get_local_steam_id().ConvertToUint64()) {
                friends.push_back(msg->friend_());
                overlay->FriendConnect(msg->friend_());
                persona_change((uint64)msg->friend_().id(), k_EPersonaChangeName);
            }
        } else {
            std::map<std::string, std::string> map1(f->rich_presence().begin(), f->rich_presence().end());
            std::map<std::string, std::string> map2(msg->friend_().rich_presence().begin(), msg->friend_().rich_presence().end());

            if (map1 != map2) {
                //The App ID of the game. This should always be the current game.
                if (isAppIdCompatible(f)) {
                    rich_presence_updated((uint64)msg->friend_().id(), (uint64)msg->friend_().appid());
                }
            }
            //TODO: callbacks?
            *f = msg->friend_();
        }
    }

    if (msg->has_friend_messages()) {
        if (msg->friend_messages().type() == Friend_Messages::LOBBY_INVITE) {
            PRINT_DEBUG("Steam_Friends::Callback Got Lobby Invite\n");
            Friend *f = find_friend((uint64)msg->source_id());
            if (f) {
                LobbyInvite_t data;
                data.m_ulSteamIDUser = msg->source_id();
                data.m_ulSteamIDLobby = msg->friend_messages().lobby_id();
                data.m_ulGameID = f->appid();
                callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));

                if (overlay->Ready())
                {
                    //TODO: the user should accept the invite first but we auto accept it because there's no gui yet
                    // Then we will handle it !
                    overlay->SetLobbyInvite(*find_friend(static_cast<uint64>(msg->source_id())), msg->friend_messages().lobby_id());
                }
                else
                {
                    GameLobbyJoinRequested_t data;
                    data.m_steamIDLobby = CSteamID((uint64)msg->friend_messages().lobby_id());
                    data.m_steamIDFriend = CSteamID((uint64)msg->source_id());
                    callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));
                }
            }
        }

        if (msg->friend_messages().type() == Friend_Messages::GAME_INVITE) {
            PRINT_DEBUG("Steam_Friends::Callback Got Game Invite\n");
            //TODO: I'm pretty sure that the user should accept the invite before this is posted but we do like above
            if (overlay->Ready())
            {
                // Then we will handle it !
                overlay->SetRichInvite(*find_friend(static_cast<uint64>(msg->source_id())), msg->friend_messages().connect_str().c_str());
            }
            else
            {
                std::string const& connect_str = msg->friend_messages().connect_str();
                GameRichPresenceJoinRequested_t data = {};
                data.m_steamIDFriend = CSteamID((uint64)msg->source_id());
                strncpy(data.m_rgchConnect, connect_str.c_str(), k_cchMaxRichPresenceValueLength - 1);
                callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));
            }
        }
    }

    if (msg->has_friend_avatar()) {
        CSteamID userID((uint64)msg->source_id());
        Friend *f = find_friend(userID.ConvertToUint64());
        if (f) {
            if (msg->friend_avatar().img().type() == Image::NOTIFY) {
                PRINT_DEBUG("%s %"PRIu64".\n", "Steam_Friends::Callback Got Friend_Avatar NOTIFY for", userID.ConvertToUint64());

                std::string raw_image = msg->friend_avatar().img().img_data();
                if (raw_image.length() > 0 && raw_image.length() < FRIEND_AVATAR_MAX_IMAGE_LENGTH) {
                    uint32_t width = (uint32_t)msg->friend_avatar().img().img_width();
                    uint32_t height = (uint32_t)msg->friend_avatar().img().img_height();
                    uint32_t img_type = (uint32_t)msg->friend_avatar().img().img_type();
                    int eAvatarSize = k_EAvatarSizeMAX;

                    auto nums = avatars.find(userID.ConvertToUint64());
                    if (nums == avatars.end()) {
                        struct Avatar_Numbers n;
                        generate_avatar_numbers(n);
                        n.last_update_time = std::chrono::steady_clock::now();
                        avatars[userID.ConvertToUint64()] = n;
                        nums = avatars.find(userID.ConvertToUint64());
                    }

                    int num_replace = 0;
                    if (nums != avatars.end()) {
                        if (width > 0 && width <= 32 && height > 0 && height <= 32) {
                            eAvatarSize = k_EAvatarSize32x32;
                            num_replace = nums->second.smallest;
                        } else {
                            if (width > 32 && width <= 64 && height > 32 && height <= 64) {
                                eAvatarSize = k_EAvatarSize64x64;
                                num_replace = nums->second.medium;
                            } else {
                                if (width > 64 && width <= 184 && height > 64 && height <= 184) {
                                    eAvatarSize = k_EAvatarSize184x184;
                                    num_replace = nums->second.large;
                                }
                            }
                        }

                        if (eAvatarSize != k_EAvatarSizeMAX) {
                            
                            switch (img_type) {
                                case Image::RAW:
                                    PRINT_DEBUG("%s %"PRIu64" %s %d %s.\n",
                                                "Steam_Friends::Callback Got Friend_Avatar NOTIFY for",
                                                userID.ConvertToUint64(),
                                                "size",
                                                eAvatarSize,
                                                "image type RAW");
                                    break;
                                case Image::JPG:
                                    {
                                        std::string convert;
                                        int n_width = 0;
                                        int n_height = 0;
                                        PRINT_DEBUG("%s %"PRIu64" %s %d %s.\n",
                                                    "Steam_Friends::Callback Got Friend_Avatar NOTIFY for",
                                                    userID.ConvertToUint64(),
                                                    "size",
                                                    eAvatarSize,
                                                    "image type JPG");
                                        convert = convert_jpg_buffer_std_string_to_std_string_uint8(raw_image, &n_width, &n_height, NULL, 4);
                                        if (convert.length() > 0 && n_width == width && n_height == height) {
                                            raw_image = convert;
                                            convert.clear();
                                        } else {
                                            raw_image.clear();
                                        }
                                    }
                                    break;
                                case Image::PNG:
                                    {
                                        std::string convert;
                                        int n_width = 0;
                                        int n_height = 0;
                                        PRINT_DEBUG("%s %"PRIu64" %s %d %s.\n",
                                                    "Steam_Friends::Callback Got Friend_Avatar NOTIFY for",
                                                    userID.ConvertToUint64(),
                                                    "size",
                                                    eAvatarSize,
                                                    "image type PNG");
                                        convert = convert_png_buffer_std_string_to_std_string_uint8(raw_image, &n_width, &n_height, NULL, 4);
                                        if (convert.length() > 0 && n_width == width && n_height == height) {
                                            raw_image = convert;
                                            convert.clear();
                                        } else {
                                            raw_image.clear();
                                        }
                                    }
                                    break;
                                default:
                                    raw_image.clear();
                                    PRINT_DEBUG("%s %"PRIu64" %s %d %s %d.\n",
                                                "Steam_Friends::Callback Got Friend_Avatar NOTIFY for",
                                                userID.ConvertToUint64(),
                                                "size",
                                                eAvatarSize,
                                                "with unsupported image type",
                                                img_type);
                                    break;
                            };
                            if (raw_image.length() > 0) {
                                int ref = settings->replace_image(num_replace, raw_image, width, height);
                                if (ref != 0) {
                                    nums->second.last_update_time = std::chrono::steady_clock::now();

                                    AvatarImageLoaded_t ail_data = {};
                                    ail_data.m_steamID = userID.ConvertToUint64();
                                    ail_data.m_iImage = ref;
                                    ail_data.m_iWide = width;
                                    ail_data.m_iTall = height;
                                    callback_results->addCallResult(ail_data.k_iCallback, &ail_data, sizeof(ail_data));      
                                    persona_change(userID, k_EPersonaChangeAvatar);
                                }
                            }
                        }
                    }
                }
            }

            if (msg->friend_avatar().img().type() == Image::REQUEST) {
                CSteamID requestID((uint64)msg->dest_id());
                if (settings->get_local_steam_id() == requestID) {
                    PRINT_DEBUG("%s %"PRIu64".\n", "Steam_Friends::Callback Got Friend_Avatar REQUEST from", userID.ConvertToUint64());

                    uint32_t width = (uint32_t)msg->friend_avatar().img().img_width();
                    uint32_t height = (uint32_t)msg->friend_avatar().img().img_height();
                    uint32_t img_type = (uint32_t)msg->friend_avatar().img().img_type();

                    int eAvatarSize = k_EAvatarSizeMAX;
                    if (width > 0 && width <= 32 && height > 0 && height <= 32) {
                        eAvatarSize = k_EAvatarSize32x32;
                    } else {
                        if (width > 32 && width <= 64 && height > 32 && height <= 64) {
                            eAvatarSize = k_EAvatarSize64x64;
                        } else {
                            if (width > 64 && width <= 184 && height > 64 && height <= 184) {
                                eAvatarSize = k_EAvatarSize184x184;
                            }
                        }
                    }

                    int ref = GetFriendAvatar(settings->get_local_steam_id(), eAvatarSize);
                    if (ref != 0) {

                        uint32 n_width = 0;
                        uint32 n_height = 0;
                        Steam_Utils* steamUtils = get_steam_client()->steam_utils;
                        if ((steamUtils->GetImageSize(ref, &n_width, &n_height) == true) &&
                            (n_width > 0) && (n_height > 0)) {
                            uint8 * raw_image = new uint8[(n_width * n_height * sizeof(uint32))];
                            if (raw_image != NULL) {
                                if (steamUtils->GetImageRGBA(ref,
                                                             raw_image,
                                                             (n_width * n_height * sizeof(uint32))) == true) {
                                    uint32_t img_type = (uint32_t)msg->friend_avatar().img().img_type();
                                    PRINT_DEBUG("%s %"PRIu64" %s %d %s %d.\n",
                                                "Steam_Friends::Callback Got Friend_Avatar REQUEST from",
                                                userID.ConvertToUint64(),
                                                "for image type",
                                                img_type,
                                                "size",
                                                eAvatarSize);

                                    std::string pixdata = "";
                                    if (img_type == Image::PNG) {
                                        pixdata = convert_raw_uint8_to_png_std_string(raw_image, n_width, n_height, 4);
                                        if (pixdata.length() <= 0 || pixdata.length() >= FRIEND_AVATAR_MAX_IMAGE_LENGTH) {
                                            if (pixdata.length() >= FRIEND_AVATAR_MAX_IMAGE_LENGTH) {
                                                PRINT_DEBUG("%s %"PRIu64" %s %d. %s %"PRI_ZU" %s.\n",
                                                            "Steam_Friends::Callback Cannot complete Friend_Avatar REQUEST from",
                                                            userID.ConvertToUint64(),
                                                            "for PNG image size",
                                                            eAvatarSize,
                                                            "Image is over maximum size by ",
                                                            pixdata.length() - FRIEND_AVATAR_MAX_IMAGE_LENGTH,
                                                            "bytes");
                                            } else {
                                                PRINT_DEBUG("%s %"PRIu64" %s %d. %s.\n",
                                                            "Steam_Friends::Callback Cannot complete Friend_Avatar REQUEST from",
                                                            userID.ConvertToUint64(),
                                                            "for PNG image size",
                                                            eAvatarSize,
                                                            "Could not convert image to requested type");
                                            }

                                            // Try again using JPG.
                                            img_type = Image::JPG;
                                            pixdata.clear();
                                        }
                                    }

                                    if (img_type == Image::JPG) {
                                        pixdata = convert_raw_uint8_to_jpg_std_string(raw_image, n_width, n_height, 4);
                                        if (pixdata.length() <= 0 || pixdata.length() >= FRIEND_AVATAR_MAX_IMAGE_LENGTH) {
                                            // Try again using RAW.
                                            if (pixdata.length() >= FRIEND_AVATAR_MAX_IMAGE_LENGTH) {
                                                PRINT_DEBUG("%s %"PRIu64" %s %d. %s %"PRI_ZU" %s.\n",
                                                            "Steam_Friends::Callback Cannot complete Friend_Avatar REQUEST from",
                                                            userID.ConvertToUint64(),
                                                            "for JPG image size",
                                                            eAvatarSize,
                                                            "Image is over maximum size by ",
                                                            pixdata.length() - FRIEND_AVATAR_MAX_IMAGE_LENGTH,
                                                            "bytes");
                                            } else {
                                                PRINT_DEBUG("%s %"PRIu64" %s %d. %s.\n",
                                                            "Steam_Friends::Callback Cannot complete Friend_Avatar REQUEST from",
                                                            userID.ConvertToUint64(),
                                                            "for JPG image size",
                                                            eAvatarSize,
                                                            "Could not convert image to requested type");
                                            }
                                            img_type = Image::RAW;
                                            pixdata.clear();
                                        }
                                    }

                                    if (img_type == Image::RAW) {
                                        for (size_t x = 0; x < (n_width * n_height * sizeof(uint32)); x++) {
                                            char a = (char)(raw_image[x]);
                                            pixdata += a;
                                        }
                                        if (pixdata.length() <= 0 || pixdata.length() >= FRIEND_AVATAR_MAX_IMAGE_LENGTH) {
                                            // No more attempts.
                                            if (pixdata.length() >= FRIEND_AVATAR_MAX_IMAGE_LENGTH) {
                                                PRINT_DEBUG("%s %"PRIu64" %s %d. %s %"PRI_ZU" %s.\n",
                                                            "Steam_Friends::Callback Cannot complete Friend_Avatar REQUEST from",
                                                            userID.ConvertToUint64(),
                                                            "for RAW image size",
                                                            eAvatarSize,
                                                            "Image is over maximum size by ",
                                                            pixdata.length() - FRIEND_AVATAR_MAX_IMAGE_LENGTH,
                                                            "bytes");
                                            } else {
                                                PRINT_DEBUG("%s %"PRIu64" %s %d. %s.\n",
                                                            "Steam_Friends::Callback Cannot complete Friend_Avatar REQUEST from",
                                                            userID.ConvertToUint64(),
                                                            "for RAW image size",
                                                            eAvatarSize,
                                                            "Could not convert image to requested type");
                                            }
                                            pixdata.clear();
                                        }
                                    }

                                    if (img_type != Image::PNG && img_type != Image::JPG && img_type != Image::RAW) {
                                        pixdata.clear();
                                        PRINT_DEBUG("%s %"PRIu64" %s %d %s %d.\n",
                                                    "Steam_Friends::Callback Got Friend_Avatar REQUEST from",
                                                    userID.ConvertToUint64(),
                                                    "for unsupported image type",
                                                    img_type,
                                                    "size",
                                                    eAvatarSize);
                                    }

                                    if (pixdata.length() > 0 && pixdata.length() < FRIEND_AVATAR_MAX_IMAGE_LENGTH) {
                                        PRINT_DEBUG("Steam_Friends::Callback Sending Friend_Avatar NOTIFY size %d type %d\n",
                                                    eAvatarSize,
                                                    img_type);
                                        Common_Message msg_;
                                        msg_.set_source_id(settings->get_local_steam_id().ConvertToUint64());
                                        msg_.set_dest_id(msg->source_id());

                                        Image *img = new Image();
                                        img->set_type(Image::NOTIFY);
                                        img->set_img_type(static_cast<Image_ImgTypes>(img_type));
                                        img->set_img_width(n_width);
                                        img->set_img_height(n_height);
                                        img->set_img_data(pixdata);

                                        Friend_Avatar *friend_avatar = new Friend_Avatar();
                                        friend_avatar->set_allocated_img(img);

                                        msg_.set_allocated_friend_avatar(friend_avatar);
                                        network->sendTo(&msg_, true);
                                    }
                                }

                                delete raw_image;
                                raw_image = NULL;
                            }
                        }
                    }
                }
            }
        }
    }
}
