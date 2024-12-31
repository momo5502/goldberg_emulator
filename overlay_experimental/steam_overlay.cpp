#include "steam_overlay.h"

#ifdef EMU_OVERLAY

#include <thread>
#include <string>
#include <sstream>
#include <cctype>
#include <imgui.h>

#include "../dll/dll.h"
#include "../dll/settings_parser.h"

#include "Renderer_Detector.h"

static constexpr int max_window_id = 10000;
static constexpr int base_notif_window_id  = 0 * max_window_id;
static constexpr int base_friend_window_id = 1 * max_window_id;
static constexpr int base_friend_item_id   = 2 * max_window_id;

static constexpr char *valid_languages[] = {
    "english",
    "arabic",
    "bulgarian",
    "schinese",
    "tchinese",
    "czech",
    "danish",
    "dutch",
    "finnish",
    "french",
    "german",
    "greek",
    "hungarian",
    "italian",
    "japanese",
    "koreana",
    "norwegian",
    "polish",
    "portuguese",
    "brazilian",
    "romanian",
    "russian",
    "spanish",
    "latam",
    "swedish",
    "thai",
    "turkish",
    "ukrainian",
    "vietnamese"
};

static constexpr char *valid_ui_notification_position_labels[] = {
    "top left",
    "top right",
    "bottom left",
    "bottom right"
};

#define URL_WINDOW_NAME "URL Window"

class Steam_Overlay_CCallback
{
    private:
        GOLDBERG_CALLBACK_INTERNAL(Steam_Overlay_CCallback, OnAvatarImageLoaded, AvatarImageLoaded_t);

    public:
        bool is_ready() {
            return (AvatarImageLoaded_t_is_registered());
        }
};

int find_free_id(std::vector<int> & ids, int base)
{
    std::sort(ids.begin(), ids.end());

    int id = base;
    for (auto i : ids)
    {
        if (id < i)
            break;
        id = i + 1;
    }

    return id > (base+max_window_id) ? 0 : id;
}

int find_free_friend_id(std::map<Friend, friend_window_state, Friend_Less> const& friend_windows)
{
    std::vector<int> ids;
    ids.reserve(friend_windows.size());

    std::for_each(friend_windows.begin(), friend_windows.end(), [&ids](std::pair<Friend const, friend_window_state> const& i)
    {
        ids.emplace_back(i.second.id);
    });
    
    return find_free_id(ids, base_friend_window_id);
}

int find_free_notification_id(std::vector<Notification> const& notifications)
{
    std::vector<int> ids;
    ids.reserve(notifications.size());

    std::for_each(notifications.begin(), notifications.end(), [&ids](Notification const& i)
    {
        ids.emplace_back(i.id);
    });
    

    return find_free_id(ids, base_friend_window_id);
}

bool operator<(const Profile_Image_ID &a, const Profile_Image_ID &b) {
    return ((a.id.ConvertToUint64() < b.id.ConvertToUint64()) && (a.eAvatarSize < b.eAvatarSize));
}

bool operator>(const Profile_Image_ID &a, const Profile_Image_ID &b) {
    return (b < a);
}

bool operator<=(const Profile_Image_ID &a, const Profile_Image_ID &b) {
    return !(a > b);
}

bool operator>=(const Profile_Image_ID &a, const Profile_Image_ID &b) {
    return !(a < b);
}

bool operator==(const Profile_Image_ID &a, const Profile_Image_ID &b) {
    return ((a.id.ConvertToUint64() == b.id.ConvertToUint64()) && (a.eAvatarSize == b.eAvatarSize));
}

bool operator!=(const Profile_Image_ID &a, const Profile_Image_ID &b) {
    return !(a == b);
}

void Steam_Overlay::populate_initial_profile_images(CSteamID id = k_steamIDNil) {
    bool found = false;
    if (id == k_steamIDNil) {
        id = settings->get_local_steam_id().ConvertToUint64();
    }
    for (auto & x : profile_images) {
        if (x.first.ConvertToUint64() == id.ConvertToUint64()) {
            found = true;
            break;
        }
    }
    if (!found) {
        profile_images[id] = Profile_Image_Set();
    }
    return;
}

#ifdef __WINDOWS__
#include "windows/Windows_Hook.h"
#endif

#include "notification.h"

bool Steam_Overlay::LoadProfileImage(const CSteamID & id, const int eAvatarSize) {
    bool ret = false;
    Profile_Image_Set new_images;
    Profile_Image_Set old_images;

    ret = LoadProfileImage(id, eAvatarSize, new_images);
    if (ret == true) {
        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
        auto entry = profile_images.find(id);
        if (entry != profile_images.end()) {
            if (eAvatarSize == k_EAvatarSize32x32) {
                profile_images[id].small = new_images.small;
            } else {
                if (eAvatarSize == k_EAvatarSize64x64) {
                    profile_images[id].medium = new_images.medium;
                } else {
                    if (eAvatarSize == k_EAvatarSize184x184) {
                        profile_images[id].large = new_images.large;
                    }
                }
            }
        } else {
            profile_images[id] = new_images;
        }
    }

    return ret;
}

bool Steam_Overlay::LoadProfileImage(const CSteamID & id, const int eAvatarSize, Profile_Image_Set & images) {
    PRINT_DEBUG("Steam_Overlay::LoadProfileImage() profile id %" PRIu64 " size %d.\n", id.ConvertToUint64(), eAvatarSize);

    bool ret = false;
    Profile_Image * image = NULL;
    uint32 width = 0;
    uint32 height = 0;
    Steam_Utils* steamUtils = get_steam_client()->steam_utils;
    Steam_Friends* steamFriends = get_steam_client()->steam_friends;

    if (eAvatarSize == k_EAvatarSize32x32)
        image = &images.small;
    if (eAvatarSize == k_EAvatarSize64x64)
        image = &images.medium;
    if (eAvatarSize == k_EAvatarSize184x184)
        image = &images.large;

    if (image != NULL) {
        int image_handle = steamFriends->GetFriendAvatar(id, eAvatarSize);
        if (image_handle != 0) {
            if (steamUtils->GetImageSize(image_handle, &width, &height) == true &&
                width > 0 &&
                height > 0) {

                PRINT_DEBUG("Steam_Overlay::LoadProfileImage() profile id %" PRIu64 " size %d image_handle %d width %d height %d.\n",
                            id.ConvertToUint64(), eAvatarSize, image_handle, width, height);

                if (image->raw_image != NULL) {
                    delete image->raw_image;
                    image->raw_image = NULL;
                }
                image->width = 0;
                image->height = 0;
                DestroyProfileImageResource(id, eAvatarSize);

                uint8 * raw_image = new uint8[(width * height * sizeof(uint32))];
                if (raw_image != NULL) {
                    if (steamUtils->GetImageRGBA(image_handle, raw_image, (width * height * sizeof(uint32))) == true) {
                        image->raw_image = raw_image;
                        image->width = width;
                        image->height = height;

                        ret = true;
                    } else {
                        delete raw_image;
                        raw_image = NULL;
                        PRINT_DEBUG("Steam_Overlay::LoadProfileImage() profile id %" PRIu64 " size %d could not get pixel data.\n",
                                    id.ConvertToUint64(), eAvatarSize);
                    }
                }
            } else {
                PRINT_DEBUG("Steam_Overlay::LoadProfileImage() profile id %" PRIu64 " size %d pixel data has invalid size.\n",
                            id.ConvertToUint64(), eAvatarSize);
            }
        } else {
            PRINT_DEBUG("Steam_Overlay::LoadProfileImage() profile id %" PRIu64 " size %d profile pixel data not loaded.\n",
                        id.ConvertToUint64(), eAvatarSize);
        }
    }

    return ret;
}

void Steam_Overlay::DestroyProfileImage(const CSteamID & id, const int eAvatarSize) {
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);

    for (auto & x : profile_images) {
        if (x.first.ConvertToUint64() == id.ConvertToUint64()) {
            DestroyProfileImage(id, eAvatarSize, x.second);
            break;
        }
    }

    return;
}

void Steam_Overlay::DestroyProfileImage(const CSteamID & id, const int eAvatarSize, Profile_Image_Set & images) {
    PRINT_DEBUG("Steam_Overlay::DestroyProfileImage() %" PRIu64 " size %d.\n", id.ConvertToUint64(), eAvatarSize);

    Profile_Image * image = NULL;
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);

    if (eAvatarSize == k_EAvatarSize32x32)
        image = &images.small;
    if (eAvatarSize == k_EAvatarSize64x64)
        image = &images.medium;
    if (eAvatarSize == k_EAvatarSize184x184)
        image = &images.large;

    if (image != NULL) {
        if (image->raw_image != NULL) {
            delete image->raw_image;
            image->raw_image = NULL;
        }
        image->width = 0;
        image->height = 0;
    }

    return;
}

bool Steam_Overlay::CreateProfileImageResource(const CSteamID & id, const int eAvatarSize) {
    bool ret = false;
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);

    for (auto & x : profile_images) {
        if (x.first.ConvertToUint64() == id.ConvertToUint64()) {
            ret = CreateProfileImageResource(id, eAvatarSize, x.second);
            break;
        }
    }

    return ret;
}

bool Steam_Overlay::CreateProfileImageResource(const CSteamID & id, const int eAvatarSize, Profile_Image_Set & images) {
    PRINT_DEBUG("Steam_Overlay::CreateProfileImageResource() %" PRIu64 " size %d.\n", id.ConvertToUint64(), eAvatarSize);

    bool ret = false;
    Profile_Image * image = NULL;
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);

    if (eAvatarSize == k_EAvatarSize32x32)
        image = &images.small;
    if (eAvatarSize == k_EAvatarSize64x64)
        image = &images.medium;
    if (eAvatarSize == k_EAvatarSize184x184)
        image = &images.large;

    if (_renderer) {
        if (image->raw_image != NULL && image->width > 0 && image->height > 0 &&
            image->image_resource.expired() == true) {
            std::weak_ptr<uint64_t> test;
            test = _renderer->CreateImageResource(image->raw_image,
                                                  image->width,
                                                  image->height);
            std::shared_ptr<uint64_t> test2;
            test2 = test.lock();
            if (!test2) {
                PRINT_DEBUG("Steam_Overlay::CreateProfileImageResource() Unable to create resource for profile id %" PRIu64 " size %d.\n",
                            id.ConvertToUint64(), eAvatarSize);
            } else {
                image->image_resource = test;
                ret = true;
                PRINT_DEBUG("Steam_Overlay::CreateProfileImageResource() created resource for profile id %" PRIu64 " size %d -> %" PRIu64 ".\n",
                            id.ConvertToUint64(), eAvatarSize, *test2);
            }
        } else {
            PRINT_DEBUG("Steam_Overlay::CreateProfileImageResource() invalid raw data for profile id %" PRIu64 " size %d.\n",
                        id.ConvertToUint64(), eAvatarSize);
        }
    }

    return ret;
}

void Steam_Overlay::DestroyProfileImageResource(const CSteamID & id, const int eAvatarSize) {
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);

    for (auto & x : profile_images) {
        if (x.first.ConvertToUint64() == id.ConvertToUint64()) {
            DestroyProfileImageResource(id, eAvatarSize, x.second);
            break;
        }
    }

    return;
}

void Steam_Overlay::DestroyProfileImageResource(const CSteamID & id, const int eAvatarSize, Profile_Image_Set & images)
{
    PRINT_DEBUG("Steam_Overlay::DestroyProfileImageResource() %" PRIu64 " size %d.\n", id.ConvertToUint64(), eAvatarSize);

    Profile_Image * image = NULL;
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);

    if (eAvatarSize == k_EAvatarSize32x32)
        image = &images.small;
    if (eAvatarSize == k_EAvatarSize64x64)
        image = &images.medium;
    if (eAvatarSize == k_EAvatarSize184x184)
        image = &images.large;

    if (_renderer != NULL && image != NULL && image->image_resource.expired() == false) {
        _renderer->ReleaseImageResource(image->image_resource);
        image->image_resource.reset();
    }

    return;
}

void Steam_Overlay::LoadAchievementImage(Overlay_Achievement & ach)
{
    PRINT_DEBUG("LoadAchievementImage() %s.\n", ach.name.c_str());

    // Image load.
    Steam_Utils* steamUtils = get_steam_client()->steam_utils;
    Steam_User_Stats* steamUserStats = get_steam_client()->steam_user_stats;
    int32 image_handle = steamUserStats->GetAchievementIcon(ach.name.c_str());
    if (image_handle != 0) {
        uint32 width = 0;
        uint32 height = 0;
        if ((steamUtils->GetImageSize(image_handle, &width, &height) == true) &&
            (width > 0) && (height > 0)) {

            PRINT_DEBUG("LoadAchievementImage() %d %d %d.\n", image_handle, width, height);

            {
                std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                if (ach.raw_image != NULL) {
                    delete ach.raw_image;
                    ach.raw_image = NULL;
                }
                ach.raw_image_width = 0;
                ach.raw_image_height = 0;
                DestroyAchievementImageResource(ach);
            }

            uint8 * raw_image = new uint8[(width * height * sizeof(uint32))];
            if (raw_image != NULL) {
                if (steamUtils->GetImageRGBA(image_handle,
                                             raw_image,
                                             (width * height * sizeof(uint32))) == true) {
                    PRINT_DEBUG("LoadAchievementImage() %d -> %p.\n", image_handle, raw_image);
                    {
                        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                        ach.raw_image = raw_image;
                        ach.raw_image_width = width;
                        ach.raw_image_height = height;
                    }
                } else {
                    delete raw_image;
                    PRINT_DEBUG("LoadAchievementImage() Achievement %s could not get pixel data.\n", ach.name.c_str());
                }
            } else {
                PRINT_DEBUG("LoadAchievementImage() Achievement %s could not allocate memory for pixel data.\n", ach.name.c_str());
            }
        } else {
            PRINT_DEBUG("LoadAchievementImage() Achievement %s image pixel data has an invalid size.\n", ach.name.c_str());
        }
    } else {
        PRINT_DEBUG("LoadAchievementImage() Achievement %s is not loaded.\n", ach.name.c_str());
    }
}

void Steam_Overlay::CreateAchievementImageResource(Overlay_Achievement & ach)
{
    PRINT_DEBUG("CreateAchievementImageResource() %s. %d x %d -> %p\n", ach.name.c_str(), ach.raw_image_width, ach.raw_image_height, ach.raw_image);

    if (_renderer) {
        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
        if (ach.raw_image != NULL && ach.raw_image_width > 0 && ach.raw_image_height > 0 && ach.image_resource.expired() == true) {
            std::weak_ptr<uint64_t> test;
            test = _renderer->CreateImageResource(ach.raw_image,
                                                  ach.raw_image_width,
                                                  ach.raw_image_height);
            ach.image_resource = test;
            std::shared_ptr<uint64_t> test2;
            test2 = test.lock();
            if (!test2) {
                PRINT_DEBUG("CreateAchievementImageResource() Unable to create resource for %s.\n", ach.name.c_str());
            } else {
                PRINT_DEBUG("CreateAchievementImageResource() created resource for %s -> %PRIu64.\n", ach.name.c_str(), *test2);
            }

        } else {
            PRINT_DEBUG("CreateAchievementImageResource() invalid raw data for %s.\n", ach.name.c_str());
        }
    }
}

void Steam_Overlay::DestroyAchievementImageResource(Overlay_Achievement & ach)
{
    PRINT_DEBUG("DestroyAchievementImageResource() %s.\n", ach.name.c_str());

    if (_renderer) {
        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
        if (ach.image_resource.expired() == false) {
            _renderer->ReleaseImageResource(ach.image_resource);
            ach.image_resource.reset();
        }
    }
}

void Steam_Overlay::steam_overlay_run_every_runcb(void* object)
{
    Steam_Overlay* _this = reinterpret_cast<Steam_Overlay*>(object);
    _this->RunCallbacks();
}

void Steam_Overlay::steam_overlay_callback(void* object, Common_Message* msg)
{
    Steam_Overlay* _this = reinterpret_cast<Steam_Overlay*>(object);
    _this->Callback(msg);
}

Steam_Overlay::Steam_Overlay(Settings* settings, SteamCallResults* callback_results, SteamCallBacks* callbacks, RunEveryRunCB* run_every_runcb, Networking* network) :
    settings(settings),
    callback_results(callback_results),
    callbacks(callbacks),
    run_every_runcb(run_every_runcb),
    network(network),
    setup_overlay_called(false),
    show_overlay(false),
    is_ready(false),
    notif_position(ENotificationPosition::k_EPositionTopRight),
    current_ui_notification_position_selection(1), // valid_ui_notification_position_labels[1] == "top right"
    h_inset(0),
    v_inset(0),
    overlay_state_changed(false),
    i_have_lobby(false),
    show_achievements(false),
    show_settings(false),
    show_profile_image_select(false),
    show_drive_list(false),
    tried_load_new_profile_image(false),
    cleared_new_profile_images_struct(true),
    _renderer(nullptr),
    fonts_atlas(nullptr),
    earned_achievement_count(0)
{
    strncpy(username_text, settings->get_local_name(), sizeof(username_text));

    show_achievement_desc_on_unlock = settings->get_show_achievement_desc_on_unlock();
    show_achievement_hidden_unearned = settings->get_show_achievement_hidden_unearned();

    radio_btn_new_profile_image_size[0] = false;
    radio_btn_new_profile_image_size[1] = false;
    radio_btn_new_profile_image_size[2] = true;

    if (settings->warn_forced) {
        this->disable_forced = true;
        this->warning_forced = true;
    } else {
        this->disable_forced = false;
        this->warning_forced = false;
    }

    if (settings->warn_local_save) {
        this->local_save = true;
    } else {
        this->local_save = false;
    }

    int i = 0;
    std::string ui_notif_pos = settings->get_ui_notification_position();
    if (ui_notif_pos.length() > 0) {
        for (auto n : valid_ui_notification_position_labels) {
            if (strcmp(n, ui_notif_pos.c_str()) == 0) {
                this->current_ui_notification_position_selection = i;
                break;
            }
            ++i;
        }
        switch (i) {
            case 0:
                PRINT_DEBUG("%s.\n",
                            "Steam_Overlay::Steam_Overlay Got ui notification position from Settings: k_EPositionTopLeft");
                this->notif_position = k_EPositionTopLeft;
                break;
            case 1:
                PRINT_DEBUG("%s.\n",
                            "Steam_Overlay::Steam_Overlay Got ui notification position from Settings: k_EPositionTopRight");
                this->notif_position = k_EPositionTopRight;
                break;
            case 2:
                PRINT_DEBUG("%s.\n",
                            "Steam_Overlay::Steam_Overlay Got ui notification position from Settings: k_EPositionBottomLeft");
                this->notif_position = k_EPositionBottomLeft;
                break;
            case 3:
                PRINT_DEBUG("%s.\n",
                            "Steam_Overlay::Steam_Overlay Got ui notification position from Settings: k_EPositionBottomRight");
                this->notif_position = k_EPositionBottomRight;
                break;
            default:
                PRINT_DEBUG("%s %s. %s.\n",
                            "Steam_Overlay::Steam_Overlay Unrecognized ui notification position received from Settings: ",
                            ui_notif_pos.c_str(),
                            "Defaulting to k_EPositionTopRight");
                break;
        };
    } else {
        PRINT_DEBUG("%s.\n",
                    "Steam_Overlay::Steam_Overlay Settings does not have a ui notification position defined. Defaulting to k_EPositionTopRight");
    }

    current_language = 0;
    const char *language = settings->get_language();

    i = 0;
    for (auto l : valid_languages) {
        if (strcmp(l, language) == 0) {
            current_language = i;
            break;
        }

        ++i;
    }

    populate_initial_profile_images();

    run_every_runcb->add(&Steam_Overlay::steam_overlay_run_every_runcb, this);
    this->network->setCallback(CALLBACK_ID_STEAM_MESSAGES, settings->get_local_steam_id(), &Steam_Overlay::steam_overlay_callback, this);
    this->overlay_CCallback = new Steam_Overlay_CCallback();
}

Steam_Overlay::~Steam_Overlay()
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    run_every_runcb->remove(&Steam_Overlay::steam_overlay_run_every_runcb, this);
    if (this->overlay_CCallback != NULL) {
        delete this->overlay_CCallback;
        this->overlay_CCallback = NULL;
    }

    if (achievements.size()) {
        for (auto & x : achievements) {
            if (x.raw_image != NULL) {
                delete x.raw_image;
                x.raw_image = NULL;
                x.raw_image_width = 0;
                x.raw_image_height = 0;
            }
            DestroyAchievementImageResource(x);
        }
    }

    if (profile_images.size()) {
        for (auto & x : profile_images) {
            if (x.second.small.raw_image != NULL) {
                delete x.second.small.raw_image;
                x.second.small.raw_image = NULL;
            }
            x.second.small.width = 0;
            x.second.small.height = 0;

            if (x.second.medium.raw_image != NULL) {
                delete x.second.medium.raw_image;
                x.second.medium.raw_image = NULL;
            }
            x.second.medium.width = 0;
            x.second.medium.height = 0;

            if (x.second.large.raw_image != NULL) {
                delete x.second.large.raw_image;
                x.second.large.raw_image = NULL;
            }
            x.second.large.width = 0;
            x.second.large.height = 0;
        }
    }

    DestroyProfileImageResources();

    new_profile_image_handles.small.raw_image = NULL; // Handle
    new_profile_image_handles.medium.raw_image = NULL; // Handle
    new_profile_image_handles.large.raw_image = NULL; // Handle

    DestroyTemporaryImageResources();
    DestroyTemporaryImages();
}

bool Steam_Overlay::Ready() const
{
    return is_ready;
}

bool Steam_Overlay::NeedPresent() const
{
    return true;
}

void Steam_Overlay::SetNotificationPosition(ENotificationPosition eNotificationPosition)
{
    switch (eNotificationPosition) {
        case k_EPositionTopLeft:
            PRINT_DEBUG("%s.\n",
                        "Steam_Overlay::SetNotificationPosition Got ui notification position: k_EPositionTopLeft");
            this->notif_position = k_EPositionTopLeft;
            this->current_ui_notification_position_selection = 0;
            break;
        case k_EPositionTopRight:
            PRINT_DEBUG("%s.\n",
                        "Steam_Overlay::SetNotificationPosition Got ui notification position: k_EPositionTopRight");
            this->notif_position = k_EPositionTopRight;
            this->current_ui_notification_position_selection = 1;
            break;
        case k_EPositionBottomLeft:
            PRINT_DEBUG("%s.\n",
                        "Steam_Overlay::SetNotificationPosition Got ui notification position: k_EPositionBottomLeft");
            this->notif_position = k_EPositionBottomLeft;
            this->current_ui_notification_position_selection = 2;
            break;
        case k_EPositionBottomRight:
            PRINT_DEBUG("%s.\n",
                        "Steam_Overlay::SetNotificationPosition Got ui notification position: k_EPositionBottomRight");
            this->notif_position = k_EPositionBottomRight;
            this->current_ui_notification_position_selection = 3;
            break;
        default:
            PRINT_DEBUG("%s %d.\n",
                        "Steam_Overlay::SetNotificationPosition Unrecognized ui notification position received: ",
                        eNotificationPosition);
            break;
    };
}

void Steam_Overlay::SetNotificationInset(int nHorizontalInset, int nVerticalInset)
{
    h_inset = nHorizontalInset;
    v_inset = nVerticalInset;
}

void Steam_Overlay::SetupOverlay()
{
    PRINT_DEBUG("%s\n", __FUNCTION__);
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    if (!setup_overlay_called)
    {
        setup_overlay_called = true;
        future_renderer = ingame_overlay::DetectRenderer();
    }
}


void Steam_Overlay::UnSetupOverlay()
{
    ingame_overlay::StopRendererDetection();
    if (!Ready() && future_renderer.valid()) {
        if (future_renderer.wait_for(std::chrono::milliseconds{500} + std::chrono::seconds{MAX_RENDERER_API_DETECT_TIMEOUT}) ==  std::future_status::ready) {
            future_renderer.get();
            ingame_overlay::FreeDetector();
        }
    }
}

void Steam_Overlay::HookReady(bool ready)
{
    PRINT_DEBUG("%s %u\n", __FUNCTION__, ready);
    {
        // TODO: Uncomment this and draw our own cursor (cosmetics)
        ImGuiIO &io = ImGui::GetIO();
        //io.WantSetMousePos = false;
        //io.MouseDrawCursor = false;
        //io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

        io.IniFilename = NULL;

        is_ready = ready;
    }
}

void Steam_Overlay::OpenOverlayInvite(CSteamID lobbyId)
{
    ShowOverlay(true);
}

void Steam_Overlay::OpenOverlay(const char* pchDialog)
{
    // TODO: Show pages depending on pchDialog
    ShowOverlay(true);
}

void Steam_Overlay::OpenOverlayWebpage(const char* pchURL)
{
    show_url = pchURL;
    ShowOverlay(true);
}

bool Steam_Overlay::ShowOverlay() const
{
    return show_overlay;
}

bool Steam_Overlay::OpenOverlayHook(bool toggle)
{
    if (toggle) {
        ShowOverlay(!show_overlay);
    }

    return show_overlay;
}

void Steam_Overlay::ShowOverlay(bool state)
{
    if (!Ready() || show_overlay == state)
        return;

    ImGuiIO &io = ImGui::GetIO();

    if(state)
    {
        io.MouseDrawCursor = true;
    }
    else
    {
        io.MouseDrawCursor = false;
    }

#ifdef __WINDOWS__
    static RECT old_clip;

    if (state)
    {
        HWND game_hwnd = Windows_Hook::Inst()->GetGameHwnd();
        RECT cliRect, wndRect, clipRect;

        GetClipCursor(&old_clip);
        // The window rectangle has borders and menus counted in the size
        GetWindowRect(game_hwnd, &wndRect);
        // The client rectangle is the window without borders
        GetClientRect(game_hwnd, &cliRect);

        clipRect = wndRect; // Init clip rectangle

        // Get Window width with borders
        wndRect.right -= wndRect.left;
        // Get Window height with borders & menus
        wndRect.bottom -= wndRect.top;
        // Compute the border width
        int borderWidth = (wndRect.right - cliRect.right) / 2;
        // Client top clip is the menu bar width minus bottom border
        clipRect.top += wndRect.bottom - cliRect.bottom - borderWidth;
        // Client left clip is the left border minus border width
        clipRect.left += borderWidth;
        // Here goes the same for right and bottom
        clipRect.right -= borderWidth;
        clipRect.bottom -= borderWidth;

        ClipCursor(&clipRect);
    }
    else
    {
        ClipCursor(&old_clip);
    }

#else

#endif

    show_overlay = state;
    overlay_state_changed = true;
}

void Steam_Overlay::NotifyUser(friend_window_state& friend_state)
{
    if (!(friend_state.window_state & window_state_show) || !show_overlay)
    {
        friend_state.window_state |= window_state_need_attention;
#ifdef __WINDOWS__
        PlaySound((LPCSTR)notif_invite_wav, NULL, SND_ASYNC | SND_MEMORY);
#endif
    }
}

void Steam_Overlay::SetLobbyInvite(Friend friendId, uint64 lobbyId)
{
    if (!Ready())
        return;

    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    auto i = friends.find(friendId);
    if (i != friends.end())
    {
        auto& frd = i->second;
        frd.lobbyId = lobbyId;
        frd.window_state |= window_state_lobby_invite;
        // Make sure don't have rich presence invite and a lobby invite (it should not happen but who knows)
        frd.window_state &= ~window_state_rich_invite;
        AddInviteNotification(*i);
        NotifyUser(i->second);
    }
}

void Steam_Overlay::SetRichInvite(Friend friendId, const char* connect_str)
{
    if (!Ready())
        return;

    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    auto i = friends.find(friendId);
    if (i != friends.end())
    {
        auto& frd = i->second;
        strncpy(frd.connect, connect_str, k_cchMaxRichPresenceValueLength - 1);
        frd.window_state |= window_state_rich_invite;
        // Make sure don't have rich presence invite and a lobby invite (it should not happen but who knows)
        frd.window_state &= ~window_state_lobby_invite;
        AddInviteNotification(*i);
        NotifyUser(i->second);
    }
}

void Steam_Overlay::FriendConnect(Friend _friend)
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    int id = find_free_friend_id(friends);
    if (id != 0)
    {
        auto& item = friends[_friend];
        item.window_title = std::move(_friend.name() + " playing " + std::to_string(_friend.appid()));
        item.window_state = window_state_none;
        item.id = id;
        memset(item.chat_input, 0, max_chat_len);
        item.joinable = false;
    }
    else
        PRINT_DEBUG("No more free id to create a friend window\n");
}

void Steam_Overlay::FriendDisconnect(Friend _friend)
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    auto it = friends.find(_friend);
    if (it != friends.end())
        friends.erase(it);
}

void Steam_Overlay::AddMessageNotification(std::string const& message, CSteamID frd = k_steamIDNil)
{
    std::lock_guard<std::recursive_mutex> lock(notifications_mutex);
    int id = find_free_notification_id(notifications);
    if (id != 0)
    {
        Notification notif;
        notif.id = id;
        notif.type = notification_type_message;
        notif.message = message;
        notif.steam_id = frd;
        notif.start_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
        notifications.emplace_back(notif);
        have_notifications = true;
    }
    else
        PRINT_DEBUG("No more free id to create a notification window\n");
}

void Steam_Overlay::AddAchievementNotification(nlohmann::json const& ach)
{
    std::lock_guard<std::recursive_mutex> lock(notifications_mutex);
    int id = find_free_notification_id(notifications);
    if (id != 0)
    {
        Notification notif;
        notif.id = id;
        notif.type = notification_type_achievement;
        // Achievement image
        if (achievements.size() > 0) {
            for (auto & x : achievements) {
                if (x.name == ach["name"].get<std::string>()) {
                    // Reload the image due to the pop.
                    LoadAchievementImage(x);
                    DestroyAchievementImageResource(x);
                    // Cannot call CreateAchievementImageResource(x) here. OpenGL displays bad texture.
                    notif.ach_name = x.name;
                }
            }
        }
        // Achievement count.
        if (total_achievement_count > earned_achievement_count) {
            earned_achievement_count++;
        }
        notif.message = "Achievement Unlocked!\n";
        if (show_achievement_desc_on_unlock) {
            notif.message += ach["displayName"].get<std::string>() + "\n" + ach["description"].get<std::string>();
        } else {
            notif.message += "\n" + ach["displayName"].get<std::string>();
        }
        notif.start_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
        notifications.emplace_back(notif);
        have_notifications = true;
    }
    else
        PRINT_DEBUG("No more free id to create a notification window\n");

    std::string ach_name = ach.value("name", "");
    for (auto &a : achievements) {
        if (a.name == ach_name) {
            bool achieved = false;
            uint32 unlock_time = 0;
            get_steam_client()->steam_user_stats->GetAchievementAndUnlockTime(a.name.c_str(), &achieved, &unlock_time);
            a.achieved = achieved;
            a.unlock_time = unlock_time;
        }
    }
}

void Steam_Overlay::AddInviteNotification(std::pair<const Friend, friend_window_state>& wnd_state)
{
    std::lock_guard<std::recursive_mutex> lock(notifications_mutex);
    int id = find_free_notification_id(notifications);
    if (id != 0)
    {
        Notification notif;
        notif.id = id;
        notif.type = notification_type_invite;
        notif.frd = &wnd_state;
        notif.message = wnd_state.first.name() + " invited you to join a game";
        notif.steam_id = CSteamID((uint64)wnd_state.first.id());
        notif.start_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
        notifications.emplace_back(notif);
        have_notifications = true;
    }
    else
        PRINT_DEBUG("No more free id to create a notification window\n");
}

bool Steam_Overlay::FriendJoinable(std::pair<const Friend, friend_window_state> &f)
{
    Steam_Friends* steamFriends = get_steam_client()->steam_friends;

    if( std::string(steamFriends->GetFriendRichPresence(f.first.id(), "connect")).length() > 0 )
        return true;

    FriendGameInfo_t friend_game_info = {};
    steamFriends->GetFriendGamePlayed(f.first.id(), &friend_game_info);
    if (friend_game_info.m_steamIDLobby.IsValid() && (f.second.window_state & window_state_lobby_invite))
        return true;

    return false;
}

bool Steam_Overlay::IHaveLobby()
{
    Steam_Friends* steamFriends = get_steam_client()->steam_friends;
    if (std::string(steamFriends->GetFriendRichPresence(settings->get_local_steam_id(), "connect")).length() > 0)
        return true;

    if (settings->get_lobby().IsValid())
        return true;

    return false;
}

void Steam_Overlay::BuildContextMenu(Friend const& frd, friend_window_state& state)
{
    if (ImGui::BeginPopupContextItem("Friends_ContextMenu", 1))
    {
        bool close_popup = false;

        if (ImGui::Button("Chat"))
        {
            state.window_state |= window_state_show;
            close_popup = true;
        }
        // If we have the same appid, activate the invite/join buttons
        if (settings->get_local_game_id().AppID() == frd.appid())
        {
            if (i_have_lobby && ImGui::Button("Invite###PopupInvite"))
            {
                state.window_state |= window_state_invite;
                has_friend_action.push(frd);
                close_popup = true;
            }
            if (state.joinable && ImGui::Button("Join###PopupJoin"))
            {
                state.window_state |= window_state_join;
                has_friend_action.push(frd);
                close_popup = true;
            }
        }
        if( close_popup)
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Steam_Overlay::BuildFriendWindow(Friend const& frd, friend_window_state& state)
{
    if (!(state.window_state & window_state_show))
        return;

    bool show = true;
    bool send_chat_msg = false;

    float width = ImGui::CalcTextSize("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA").x;
    
    if (state.window_state & window_state_need_attention && ImGui::IsWindowFocused())
    {
        state.window_state &= ~window_state_need_attention;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2{ width, ImGui::GetFontSize()*8 + ImGui::GetFrameHeightWithSpacing()*4 },
        ImVec2{ std::numeric_limits<float>::max() , std::numeric_limits<float>::max() });

    // Window id is after the ###, the window title is the friend name
    std::string friend_window_id = std::move("###" + std::to_string(state.id));
    if (ImGui::Begin((state.window_title + friend_window_id).c_str(), &show))
    {
        ImGuiStyle currentStyle = ImGui::GetStyle();
        ImVec2 image_offset(((ImGui::GetWindowWidth() * 0.04f) + currentStyle.FramePadding.x),
                            ((ImGui::GetWindowHeight() * 0.04f) + currentStyle.FramePadding.y +
                              ImGui::GetFontSize() + currentStyle.ItemSpacing.y)); // calc border.
        ImVec2 image_max_resolution((ImGui::GetWindowWidth() - image_offset.x), (ImGui::GetWindowHeight() - image_offset.y)); // calc total space for image.
        if (image_max_resolution.x > image_max_resolution.y) { // fix image aspect ratio. (square)
            image_max_resolution.x = image_max_resolution.x - (image_max_resolution.x - image_max_resolution.y);
        } else {
            image_max_resolution.y = image_max_resolution.y - (image_max_resolution.y - image_max_resolution.x);
        }
        ImVec2 image_scale(image_max_resolution.x * 0.3f,
                           image_max_resolution.y * 0.3f);
        ImVec2 text_offset(image_offset.x + image_scale.x + currentStyle.ItemSpacing.x,
                           image_offset.y + (image_scale.y * 0.2f) + currentStyle.ItemSpacing.y);
        ImVec2 next_line_offset(currentStyle.FramePadding.x + currentStyle.ItemInnerSpacing.x,
                                image_offset.y + image_scale.y + (currentStyle.ItemSpacing.y * 2.0f));
        ImGui::SetCursorPos(image_offset);

        display_imgui_avatar(image_scale.x,
                             image_scale.y,
                             1.0f, 1.0f, 1.0f, 1.0f,
                             CSteamID((uint64)frd.id()),
                             k_EAvatarSize184x184,
                             0x1);
        ImGui::SetCursorPos(text_offset);

        ImGui::TextWrapped(std::string(frd.name() + " (" + std::to_string((uint64)frd.id()) + ")").c_str());
        ImGui::SetCursorPos(next_line_offset);

        if (state.window_state & window_state_need_attention && ImGui::IsWindowFocused())
        {
            state.window_state &= ~window_state_need_attention;
        }

        // Fill this with the chat box and maybe the invitation
        if (state.window_state & (window_state_lobby_invite | window_state_rich_invite))
        {
            ImGui::LabelText("##label", "%s invited you to join the game.", frd.name().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Accept"))
            {
                state.window_state |= window_state_join;
                this->has_friend_action.push(frd);
            }
            ImGui::SameLine();
            if (ImGui::Button("Refuse"))
            {
                state.window_state &= ~(window_state_lobby_invite | window_state_rich_invite);
            }
        }

        ImGui::InputTextMultiline("##chat_history", &state.chat_history[0], state.chat_history.length(), { -1.0f, -2.0f * ImGui::GetFontSize() }, ImGuiInputTextFlags_ReadOnly);
        // TODO: Fix the layout of the chat line + send button.
        // It should be like this: chat input should fill the window size minus send button size (button size is fixed)
        // |------------------------------|
        // | /--------------------------\ |
        // | |                          | |
        // | |       chat history       | |
        // | |                          | |
        // | \--------------------------/ |
        // | [____chat line______] [send] |
        // |------------------------------|
        //
        // And it is like this
        // |------------------------------|
        // | /--------------------------\ |
        // | |                          | |
        // | |       chat history       | |
        // | |                          | |
        // | \--------------------------/ |
        // | [__chat line__] [send]       |
        // |------------------------------|
        float wnd_width = ImGui::GetWindowContentRegionWidth();
        wnd_width -= ImGui::CalcTextSize("Send").x + currentStyle.FramePadding.x * 2 + currentStyle.ItemSpacing.x + 1;

        ImGui::PushItemWidth(wnd_width);
        if (ImGui::InputText("##chat_line", state.chat_input, max_chat_len, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            send_chat_msg = true;
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();

        if (ImGui::Button("Send"))
        {
            send_chat_msg = true;
        }

        if (send_chat_msg)
        {
            if (!(state.window_state & window_state_send_message))
            {
                has_friend_action.push(frd);
                state.window_state |= window_state_send_message;
            }
        }
    }
    // User closed the friend window
    if (!show)
        state.window_state &= ~window_state_show;

    ImGui::End();
}

ImFont *font_default;
ImFont *font_notif;

void Steam_Overlay::BuildNotifications(int width, int height)
{
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    int i = 0;

    int font_size = ImGui::GetFontSize();

    std::queue<Friend> friend_actions_temp;

    ImVec2 notification_rez(((Notification::width >= 1.0f) ? (width / Notification::width) : (width * Notification::width)),
                            ((Notification::height >= 1.0f) ? (height / Notification::height) : (height * Notification::height))); // calc notification screen size.

    {
        std::lock_guard<std::recursive_mutex> lock(notifications_mutex);

        for (auto it = notifications.begin(); it != notifications.end(); ++it, ++i)
        {
            auto elapsed_notif = now - it->start_time;
            float alpha = 0;
            ImGuiStyle currentStyle = ImGui::GetStyle();
            if ( elapsed_notif < Notification::fade_in)
            {
                alpha = Notification::max_alpha * (elapsed_notif.count() / static_cast<float>(Notification::fade_in.count()));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, alpha));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(Notification::r, Notification::g, Notification::b, alpha));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, alpha*2));
            }
            else if ( elapsed_notif > Notification::fade_out_start)
            {
                alpha = Notification::max_alpha * ((Notification::show_time - elapsed_notif).count() / static_cast<float>(Notification::fade_out.count()));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, alpha));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(Notification::r, Notification::g, Notification::b, alpha));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, alpha*2));
            }
            else
            {
                alpha = Notification::max_alpha;
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, Notification::max_alpha));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(Notification::r, Notification::g, Notification::b, Notification::max_alpha));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, Notification::max_alpha*2));
            }

            ImVec2 window_pos((float)width - width * Notification::width, Notification::height * font_size * i);

            ImGuiViewport * viewport = ImGui::GetMainViewport();
            if (viewport != NULL) {
                if (notif_position == ENotificationPosition::k_EPositionBottomLeft ||
                    notif_position == ENotificationPosition::k_EPositionBottomRight) {
                    window_pos.y = ((viewport->Pos.y + viewport->Size.y) - Notification::height * font_size) - window_pos.y;
                }
            }
            if (notif_position == ENotificationPosition::k_EPositionTopLeft ||
                notif_position == ENotificationPosition::k_EPositionBottomLeft) {
                window_pos.x = 0;
            }

            // Avoid overlay titlebar.
            if (show_overlay == true &&
                (notif_position == ENotificationPosition::k_EPositionTopLeft || 
                notif_position == ENotificationPosition::k_EPositionTopRight)) {
                window_pos.x += currentStyle.FramePadding.x + currentStyle.ItemSpacing.x;
                window_pos.y += currentStyle.FramePadding.y + ImGui::GetFontSize() + currentStyle.ItemSpacing.y;
            }

            ImGui::SetNextWindowPos(window_pos);
            ImGui::SetNextWindowSize(ImVec2( width * Notification::width, Notification::height * font_size ));
            ImGui::Begin(std::to_string(it->id).c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | 
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDecoration);

            // Notification image.
            ImVec4 image_color_multipler(1, 1, 1, (alpha*2)); // fade in / out.
            ImVec2 image_offset(((notification_rez.x * 0.04f) + currentStyle.FramePadding.x), ((notification_rez.y * 0.04f) + currentStyle.FramePadding.y)); // calc border.
            ImVec2 image_max_resolution((notification_rez.x - image_offset.x), (notification_rez.y - image_offset.y)); // calc total space for image.
            if (image_max_resolution.x > image_max_resolution.y) { // fix image aspect ratio. (square)
                image_max_resolution.x = image_max_resolution.x - (image_max_resolution.x - image_max_resolution.y);
            } else {
                image_max_resolution.y = image_max_resolution.y - (image_max_resolution.y - image_max_resolution.x);
            }
            ImVec2 image_scale(image_max_resolution.x * 0.4f,
                               image_max_resolution.y * 0.4f);
            ImVec2 text_offset(image_offset.x + currentStyle.ItemSpacing.x + image_scale.x, image_offset.y);
            ImGui::SetCursorPos(image_offset);

            switch (it->type)
            {
                case notification_type_achievement:
                    for (auto &a : achievements) {
                        if (a.name == it->ach_name) {
                            CreateAchievementImageResource(a);
                            if ((a.image_resource.expired() == false) &&
                                (a.raw_image_width > 0) &&
                                (a.raw_image_height > 0)) {
                                std::shared_ptr<uint64_t> s_ptr = a.image_resource.lock();
                                ImGui::Image((ImTextureID)(intptr_t)*s_ptr, image_scale, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), image_color_multipler);
                            }
                            break;
                        }
                    }
                    ImGui::SetCursorPos(text_offset);
                    ImGui::PushTextWrapPos(0.0f);
                    ImGui::TextWrapped("%s", it->message.c_str());
                    ImGui::PopTextWrapPos();
                    break;
                case notification_type_invite:
                    {
                        display_imgui_avatar(image_scale.x,
                                             image_scale.y,
                                             image_color_multipler.x, // r
                                             image_color_multipler.y, // g
                                             image_color_multipler.z, // b
                                             image_color_multipler.w, // a
                                             it->steam_id,
                                             k_EAvatarSize184x184,
                                             0x1);
                        ImGui::SetCursorPos(text_offset);
                        ImGui::TextWrapped("%s", it->message.c_str());
                        if (ImGui::Button("Join"))
                        {
                            it->frd->second.window_state |= window_state_join;
                            friend_actions_temp.push(it->frd->first);
                            it->start_time = std::chrono::seconds(0);
                        }
                    }
                    break;
                case notification_type_message:
                    display_imgui_avatar(image_scale.x,
                                         image_scale.y,
                                         image_color_multipler.x, // r
                                         image_color_multipler.y, // g
                                         image_color_multipler.z, // b
                                         image_color_multipler.w, // a
                                         it->steam_id,
                                         k_EAvatarSize184x184,
                                         0x1);
                    ImGui::SetCursorPos(text_offset);
                    ImGui::TextWrapped("%s", it->message.c_str());
                    break;
            }

            ImGui::End();

            ImGui::PopStyleColor(3);
        }
        notifications.erase(std::remove_if(notifications.begin(), notifications.end(), [&now](Notification &item) {
            return (now - item.start_time) > Notification::show_time;
        }), notifications.end());

        have_notifications = !notifications.empty();
    }

    if (!friend_actions_temp.empty()) {
        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
        while (!friend_actions_temp.empty()) {
            has_friend_action.push(friend_actions_temp.front());
            friend_actions_temp.pop();
        }
    }
}

#if defined(__WINDOWS__)
struct cb_font_str
{
    float font_size;
    ImFontConfig fontcfg;
    ImFontAtlas * atlas;
    ImFont * defaultFont;
    HDC hDevice;
    int foundBits;
    std::recursive_mutex mutex;
};

static cb_font_str CBSTR;

DWORD LoadWindowsFontFromMem_GetSize_Helper(const LOGFONT *lf, DWORD * type) {
    DWORD ret = 0;

    if (lf != NULL && type != NULL) {
        ret = GetFontData(CBSTR.hDevice, 0x66637474, 0, NULL, 0);
        if (ret != GDI_ERROR && ret > 4) {
            *type = 0x66637474;
        } else {
            ret = GetFontData(CBSTR.hDevice, 0, 0, NULL, 0);
            if (ret != GDI_ERROR && ret > 4) {
                *type = 0x0;
            } else {
                *type = ~((DWORD)0);
            }
        }
        PRINT_DEBUG("%s %s %d 0x%x\n", "LoadWindowsFontFromMem_GetSize_Helper Result for font", lf->lfFaceName, ret, *type);
    }

    return ret;
}

int LoadWindowsFontFromMem(const LOGFONT *lf)
{
    int ret = 0;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;

    if (lf != NULL) {
        hFont = CreateFontIndirect(lf);
        if (hFont != NULL) {
            oldFont = SelectObject(CBSTR.hDevice, hFont);
            uint8_t metsize = GetOutlineTextMetrics(CBSTR.hDevice, 0, NULL);
            if (metsize > 0) {
                OUTLINETEXTMETRIC * metric = (OUTLINETEXTMETRIC*)malloc(metsize);
                if (metric != NULL) {
                    memset(metric, '\0', metsize);
                    if (GetOutlineTextMetrics(CBSTR.hDevice, metsize, metric) != 0) {
                        // otmfsType: Bit 1 (May not be embedded if set.) Bit 2 (Read-Only embedding.)
                        // See also: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-outlinetextmetrica
                        if ((((UINT)(metric->otmfsType)) & ((UINT)0x1) << 1) == 0) {
                            DWORD type = 0;
                            DWORD fontDataSize = LoadWindowsFontFromMem_GetSize_Helper(lf, &type);
                            if (fontDataSize != GDI_ERROR && fontDataSize > 4 && type != ~((DWORD)0)) {
                                uint8_t * fontData = (uint8*)malloc(fontDataSize);
                                if (fontData != NULL) {
                                    ImFont * font = NULL;
                                    DWORD fontDataRet = GetFontData(CBSTR.hDevice, type, 0, fontData, fontDataSize);
                                    if (fontDataRet != GDI_ERROR && fontDataRet == fontDataSize) {

                                        if (memcmp("\0\0\0\0", fontData, sizeof("\0\0\0\0")) == 0) {
                                            PRINT_DEBUG("%s\n", "Invalid font tag. Skipping.");
                                        } else {

                                            if (type == 0x66637474) {
                                                if (memcmp(fontData, "ttcf", sizeof("ttcf")) != 0) {
                                                    PRINT_DEBUG("TAG %x %x %x %x %s\n", fontData[0], fontData[1], fontData[2], fontData[3], "NOT A TrueType Collection file, despite detection result.");
                                                    memset(fontData, '\0', fontDataSize);
                                                    free(fontData);
                                                    fontData = NULL;
                                                } else {
                                                    PRINT_DEBUG("%s\n", "Found TrueType Collection file.");
                                                }
                                            }

                                            if (fontData != NULL) {
                                                PRINT_DEBUG("%s %s %s\n", "Font considered valid. Attempting to import", lf->lfFaceName, "into ImGUI.");
                                                if ((CBSTR.foundBits & 0xF) != 0x0) {
                                                    CBSTR.fontcfg.MergeMode = true;
                                                    PRINT_DEBUG("%s\n", "Merging fonts.");
                                                }

                                                font = CBSTR.atlas->AddFontFromMemoryTTF(fontData, fontDataSize, CBSTR.font_size, &(CBSTR.fontcfg));
                                                if (font != NULL) {
                                                    PRINT_DEBUG("%s %s %s\n", "ImGUI loaded font", lf->lfFaceName, "successfully.");
                                                    if (CBSTR.defaultFont == NULL) {
                                                        CBSTR.defaultFont = font;
                                                    }
                                                    ret = 1;
                                                } else {
                                                    PRINT_DEBUG("%s %s.\n", "ImGUI failed to load font", lf->lfFaceName);
                                                }
                                            }

                                        }

                                    } else {
                                        PRINT_DEBUG("%s %d.\n", "GetFontData() failed. Ret: ", fontDataRet);
                                    }

                                    if (font == NULL) {
                                        if (fontData != NULL) {
                                            memset(fontData, '\0', fontDataSize);
                                            free(fontData);
                                            fontData = NULL;
                                        }
                                    }
                                }
                            } else {
                                PRINT_DEBUG("%s %d.\n", "GetFontData() failed. Unable to get initial size of font data. Ret: ", fontDataSize);
                            }
                        } else {
                            PRINT_DEBUG("%s %s. otmfsType data: %d.\n", "Licensing failure. Cannot use font", lf->lfFaceName, metric->otmfsType);
                        }
                    } else {
                        PRINT_DEBUG("%s %s.\n",
                                    "Steam_Overlay::LoadWindowsFontFromMem GetOutlineTextMetrics() (fill font metric struct) failed for",
                                    lf->lfFaceName);
                    }

                    free(metric);
                    metric = NULL;
                }
            } else {
                PRINT_DEBUG("%s %s.\n",
                            "Steam_Overlay::LoadWindowsFontFromMem GetOutlineTextMetrics() (get struct size) failed for",
                            lf->lfFaceName);
            }
            SelectObject(CBSTR.hDevice, oldFont);
            DeleteObject(hFont);
        } else {
            PRINT_DEBUG("%s %s.\n",
                        "Steam_Overlay::LoadWindowsFontFromMem CreateFontIndirect() failed for",
                        lf->lfFaceName);
        }
    }
    return ret;
}

int CALLBACK cb_enumfonts(const LOGFONT *lf, const TEXTMETRIC *tm, DWORD fontType, LPARAM UNUSED)
{
    int ret = 1; // Continue iteration.
    cb_font_str * cbStr = &CBSTR;
    std::lock_guard<std::recursive_mutex> lock(cbStr->mutex);
    if (CBSTR.atlas != NULL && lf != NULL && fontType == TRUETYPE_FONTTYPE) {
        /*
            foundBits:
                Bit  1: Loaded ANSI font.
                Bit  2: Loaded Japanese font.
                Bit  4: Loaded Chinese font.
                Bit  8: Loaded Korean font.
                Bit 10: Loaded preferred ANSI font.
                Bit 20: Loaded preferred CJ font.
                Bit 40: Loaded preferred K font.
                Bit 80: Searched for preferred fonts.
         */
        if ((CBSTR.foundBits & 0x80) == 0) {
            // Load ANSI first, or the other fonts will define the glyphs for english text.
            if ((CBSTR.foundBits & 0x1) == 0) {
                // preferred ANSI font.
                if ((strncmp(lf->lfFaceName, "Microsoft Sans Serif", sizeof("Microsoft Sans Serif")) == 0) && ((CBSTR.foundBits & 0x10) == 0)) {
                    if (LoadWindowsFontFromMem(lf) == 1) {
                        CBSTR.foundBits = CBSTR.foundBits | 0x10 | 0x1;
                        ret = 0;
                        PRINT_DEBUG("%s %s.\n", "Loaded preferred font:", lf->lfFaceName);
                    }
                }
            } else {
                // preferred CJ font.
                if ((strncmp(lf->lfFaceName, "SimSun", sizeof("SimSun")) == 0) && ((CBSTR.foundBits & 0x20) == 0)) {
                    if (LoadWindowsFontFromMem(lf) == 1) {
                        CBSTR.foundBits = CBSTR.foundBits | 0x20 | 0x2 | 0x4;
                        PRINT_DEBUG("%s %s.\n", "Loaded preferred font:", lf->lfFaceName);
                    }
                }
                // preferred K font.
                if ((strncmp(lf->lfFaceName, "Malgun Gothic", sizeof("Malgun Gothic")) == 0) && ((CBSTR.foundBits & 0x40) == 0)) {
                    if (LoadWindowsFontFromMem(lf) == 1) {
                        CBSTR.foundBits = CBSTR.foundBits | 0x40 | 0x8;
                        PRINT_DEBUG("%s %s.\n", "Loaded preferred font:", lf->lfFaceName);
                    }
                }
            }
        } else {
            // Load ANSI first, or the other fonts will define the glyphs for english text.
            if ((CBSTR.foundBits & 0x1) == 0) {
                if (lf->lfCharSet == ANSI_CHARSET) {
                    if (LoadWindowsFontFromMem(lf) == 1) {
                        CBSTR.foundBits = CBSTR.foundBits | 0x1;
                        ret = 0;
                        PRINT_DEBUG("%s %s.\n", "Loaded ANSI_CHARSET:", lf->lfFaceName);
                    }
                }
            } else {
                if ((CBSTR.foundBits & 0x2) == 0) {
                    if (lf->lfCharSet == SHIFTJIS_CHARSET) {
                        if (LoadWindowsFontFromMem(lf) == 1) {
                            CBSTR.foundBits = CBSTR.foundBits | 0x2;
                            PRINT_DEBUG("%s %s.\n", "Loaded SHIFTJIS_CHARSET:", lf->lfFaceName);
                        }
                    }
                }
                if ((CBSTR.foundBits & 0x4) == 0) {
                    if (lf->lfCharSet == CHINESEBIG5_CHARSET) {
                        if (LoadWindowsFontFromMem(lf) == 1) {
                            CBSTR.foundBits = CBSTR.foundBits | 0x4;
                            PRINT_DEBUG("%s %s.\n", "Loaded CHINESEBIG5_CHARSET:", lf->lfFaceName);
                        }
                    }
                }
                if ((CBSTR.foundBits & 0x8) == 0) {
                    if (lf->lfCharSet == HANGUL_CHARSET) {
                        if (LoadWindowsFontFromMem(lf) == 1) {
                            CBSTR.foundBits = CBSTR.foundBits | 0x8;
                            PRINT_DEBUG("%s %s.\n", "Loaded HANGUL_CHARSET:", lf->lfFaceName);
                        }
                    }
                }
            }
        }
    }
    if ((CBSTR.foundBits & 0xF) == 0xF) {
        PRINT_DEBUG("%s\n", "Loaded all needed fonts.");
        ret = 0; // Stop iteration.
    }
    return ret;
}
#endif

void Steam_Overlay::CreateFonts()
{
    if (fonts_atlas) return;

    ImFontAtlas *Fonts = new ImFontAtlas();

    ImFontConfig fontcfg;

    float font_size = 16.0;
    fontcfg.OversampleH = fontcfg.OversampleV = 1;
    fontcfg.PixelSnapH = true;
    fontcfg.SizePixels = font_size;

    ImFontGlyphRangesBuilder font_builder;
    for (auto & x : achievements) {
        font_builder.AddText(x.title.c_str());
        font_builder.AddText(x.description.c_str());
    }

    font_builder.AddRanges(Fonts->GetGlyphRangesDefault());

    ImVector<ImWchar> ranges;
    font_builder.BuildRanges(&ranges);

    bool need_extra_fonts = false;
    for (auto &x : ranges) {
        if (x > 0xFF) {
            need_extra_fonts = true;
            break;
        }
    }

    fontcfg.GlyphRanges = ranges.Data;
    ImFont *font = NULL;

#if defined(__WINDOWS__)
    // Do this instead of calling Fonts->AddFontFromFileTTF(). As *some* Windows implementations don't ship anything in C:\Windows\Fonts.
    HDC oDC = GetDC(NULL);
    HWND oWND = WindowFromDC(oDC);
    int caps = GetDeviceCaps(oDC, RASTERCAPS);
    if (caps != 0) {
        int width = GetDeviceCaps(oDC, HORZRES);
        int height = GetDeviceCaps(oDC, VERTRES);
        HBITMAP hBitmap = CreateCompatibleBitmap(oDC, width, height);

        LOGFONT lf;
        memset(&lf, '\0', sizeof(LOGFONT));
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfHeight = font_size;
        lf.lfQuality = ANTIALIASED_QUALITY;
        HGDIOBJ hOldBitmap;
        {
            std::lock_guard<std::recursive_mutex> lock(CBSTR.mutex);
            CBSTR.font_size = font_size;
            CBSTR.fontcfg = fontcfg;
            CBSTR.atlas = Fonts;
            CBSTR.defaultFont = NULL;
            CBSTR.hDevice = CreateCompatibleDC(oDC);
            CBSTR.foundBits = 0;
            hOldBitmap = SelectObject(CBSTR.hDevice, hBitmap);
            DeleteObject(hOldBitmap);
        }

        PRINT_DEBUG("%s\n", "Atempting to load preferred ANSI font from Win32 API.");
        EnumFontFamiliesExA(CBSTR.hDevice, &lf, cb_enumfonts, 0, 0);
        bool resume_search = false;
        {
            std::lock_guard<std::recursive_mutex> lock(CBSTR.mutex);
            if ((CBSTR.foundBits & 0x1) != 0x1) {
                CBSTR.foundBits = CBSTR.foundBits | 0x80;
                resume_search = true;
            }
        }
        if (resume_search) {
            PRINT_DEBUG("%s\n", "Atempting to load generic ANSI font from Win32 API.");
            EnumFontFamiliesExA(CBSTR.hDevice, &lf, cb_enumfonts, 0, 0);
            {
                std::lock_guard<std::recursive_mutex> lock(CBSTR.mutex);
                if ((CBSTR.foundBits & 0x1) != 0x1) {
                    PRINT_DEBUG("%s\n", "Falling back to built in ImGUI ANSI font.");
                    CBSTR.defaultFont = Fonts->AddFontDefault(&fontcfg);
                    if (CBSTR.defaultFont == NULL) {
                        PRINT_DEBUG("%s\n", "Built in ImGUI ANSI font failed to load. Weird text will probably happen.");
                    }
                    CBSTR.foundBits = CBSTR.foundBits | 0x1;
                }
                CBSTR.foundBits = CBSTR.foundBits ^ 0x80;
            }
        }
        resume_search = false;
        {
            std::lock_guard<std::recursive_mutex> lock(CBSTR.mutex);
            if ((CBSTR.foundBits & 0xE) != 0xE) {
                resume_search = true;
            }
        }
        if (resume_search) {
            PRINT_DEBUG("%s\n", "Atempting to load preferred CJK fonts from Win32 API.");
            EnumFontFamiliesExA(CBSTR.hDevice, &lf, cb_enumfonts, 0, 0);
        }
        resume_search = false;
        {
            std::lock_guard<std::recursive_mutex> lock(CBSTR.mutex);
            CBSTR.foundBits = CBSTR.foundBits | 0x80;
            if ((CBSTR.foundBits & 0xF) != 0xF) {
                resume_search = true;
            }
        }
        if (resume_search) {
            PRINT_DEBUG("%s\n", "Loading generic CJK fonts from Win32 API.");
            EnumFontFamiliesExA(CBSTR.hDevice, &lf, cb_enumfonts, 0, 0);
        }
        {
            std::lock_guard<std::recursive_mutex> lock(CBSTR.mutex);
            if (CBSTR.defaultFont != NULL) {
                font = CBSTR.defaultFont;
            }
            if (need_extra_fonts == true) {
                if ((CBSTR.foundBits & 0xF) == 0xF) {
                    need_extra_fonts = false;
                }
            }

            DeleteDC(CBSTR.hDevice); // Order is important.
            DeleteObject(hBitmap);
        }
    }
    ReleaseDC(oWND, oDC);
#else
    font = Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/roboto/unhinted/RobotoCondensed-Regular.ttf", font_size, &fontcfg);
#endif

    if (!font) {
        font = Fonts->AddFontDefault(&fontcfg);
    }

    font_notif = font_default = font;

    if (need_extra_fonts) {
#if defined(__WINDOWS__)
        PRINT_DEBUG("%s\n", "Unable to load extra fonts! Some text may not be readable!");
#else
        PRINT_DEBUG("%s\n", "loading extra fonts.");
        fontcfg.MergeMode = true;
        Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf", font_size, &fontcfg);
#endif
    }

    Fonts->Build();
    fonts_atlas = (void *)Fonts;

    // ImGuiStyle& style = ImGui::GetStyle();
    // style.WindowRounding = 0.0; // Disable round window
    reset_LastError();
}

void Steam_Overlay::ReturnTemporaryImage(Profile_Image & imageData)
{
    if (imageData.raw_image != NULL) {
        delete imageData.raw_image;
        imageData.raw_image = NULL;
    }
    imageData.width = 0;
    imageData.height = 0;
    return;
}

Profile_Image Steam_Overlay::GetTemporaryImage(uint8 * imageData)
{
    Profile_Image ret;

    if (imageData != NULL) {
        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
        for (auto & x : temp_display_images) {
            if (x.first == imageData &&
                x.second.image_data.raw_image != NULL &&
                x.second.image_data.width > 0 &&
                x.second.image_data.height > 0) {
                size_t buffer_size = x.second.image_data.width * x.second.image_data.height * sizeof(uint32_t);
                ret.raw_image = new uint8[buffer_size];
                if (ret.raw_image != NULL) {
                    for (size_t y = 0; y < buffer_size; y++) {
                        uint8 a = x.second.image_data.raw_image[y];
                        ret.raw_image[y] = a;
                    }
                    ret.width = x.second.image_data.width;
                    ret.height = x.second.image_data.height;
                    x.second.last_display_time = std::chrono::steady_clock::now();
                }
                break;
            }
        }
    }

    return ret;
}

void Steam_Overlay::PruneTemporaryImages()
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    if (temp_display_images.size() > 0) {
        for (auto & x : temp_display_images) {
            if (x.second.last_display_time < (std::chrono::steady_clock::now() - std::chrono::minutes(1))) {
                if (x.second.image_data.image_resource.expired() == false) {
                    if (_renderer) {
                        _renderer->ReleaseImageResource(x.second.image_data.image_resource);
                        x.second.image_data.image_resource.reset();
                    }
                }
                if (x.second.image_data.raw_image != NULL) {
                    delete x.second.image_data.raw_image;
                    x.second.image_data.raw_image = NULL;
                }
                x.second.image_data.width = 0;
                x.second.image_data.height = 0;
            }
        }

        bool done = false;
        do {
            if (temp_display_images.size() > 0) {
                for (std::map<uint8*,Temporary_Image>::iterator x = temp_display_images.begin();
                     x != temp_display_images.end();
                     x++) {
                    std::map<uint8*,Temporary_Image>::iterator y = x;
                    y++;
                    if (y == temp_display_images.end()) {
                        done = true;
                    }
                    if ((x->second.image_data.image_resource.expired() == true) &&
                        x->second.image_data.raw_image == NULL &&
                        x->second.image_data.width <= 0 &&
                        x->second.image_data.height <= 0) {
                        temp_display_images.erase(x);
                        break;
                    }
                }
            } else {
                done = true;
            }
        } while (!done);
    }

    return;
}

void Steam_Overlay::DestroyTemporaryImageResources()
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    for (auto & x : temp_display_images) {
        if (x.second.image_data.image_resource.expired() == false) {
            if (_renderer) {
                _renderer->ReleaseImageResource(x.second.image_data.image_resource);
                x.second.image_data.image_resource.reset();
            }
        }
    }

    return;
}

void Steam_Overlay::DestroyTemporaryImageResource(uint8 * imageData)
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    auto x = temp_display_images.find(imageData);
    if (x != temp_display_images.end()) {
        if (x->second.image_data.image_resource.expired() == false) {
            if (_renderer) {
                _renderer->ReleaseImageResource(x->second.image_data.image_resource);
                x->second.image_data.image_resource.reset();
            }
        }
    }

    return;
}

void Steam_Overlay::DestroyTemporaryImages()
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    for (auto & x : temp_display_images) {
        if (x.second.image_data.raw_image != NULL) {
            delete x.second.image_data.raw_image;
            x.second.image_data.raw_image = NULL;
        }
        x.second.image_data.width = 0;
        x.second.image_data.height = 0;
    }

    return;
}

void Steam_Overlay::DestroyTemporaryImage(uint8 * imageData)
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    DestroyTemporaryImageResource(imageData);
    auto x = temp_display_images.find(imageData);
    if (x != temp_display_images.end()) {
        if (x->second.image_data.raw_image != NULL) {
            delete x->second.image_data.raw_image;
            x->second.image_data.raw_image = NULL;
        }
        x->second.image_data.width = 0;
        x->second.image_data.height = 0;
        temp_display_images.erase(x);
    }

    return;
}

void Steam_Overlay::DestroyProfileImageResources()
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    for (auto & x : profile_images) {
        if (x.second.small.image_resource.expired() == false) {
            if (_renderer) {
                DestroyProfileImageResource(x.first, k_EAvatarSize32x32, x.second);
            }
        }
        if (x.second.medium.image_resource.expired() == false) {
            if (_renderer) {
                DestroyProfileImageResource(x.first, k_EAvatarSize64x64, x.second);
            }
        }
        if (x.second.large.image_resource.expired() == false) {
            if (_renderer) {
                DestroyProfileImageResource(x.first, k_EAvatarSize184x184, x.second);
            }
        }
    }

    return;
}

void Steam_Overlay::DestroyAchievementImageResources()
{
    for (auto & x : achievements) {
        if (x.image_resource.expired() == false) {
            DestroyAchievementImageResource(x);
        }
    }

    return;
}

int Steam_Overlay::display_imgui_achievement(float xSize,
                                             float ySize,
                                             float image_color_multipler_r,
                                             float image_color_multipler_g,
                                             float image_color_multipler_b,
                                             float image_color_multipler_a,
                                             std::string achName,
                                             uint32_t loadType = 0x0)
{
    return Steam_Overlay::display_imgui_image(displayImageTypeAchievement,
                                              xSize,
                                              ySize,
                                              image_color_multipler_r,
                                              image_color_multipler_g,
                                              image_color_multipler_b,
                                              image_color_multipler_a,
                                              achName,
                                              k_steamIDNil,
                                              k_EAvatarSizeMAX,
                                              NULL,
                                              0x0,
                                              0x0,
                                              0x0,
                                              loadType);
}

int Steam_Overlay::display_imgui_avatar(float xSize,
                                        float ySize,
                                        float image_color_multipler_r,
                                        float image_color_multipler_g,
                                        float image_color_multipler_b,
                                        float image_color_multipler_a,
                                        CSteamID userID = k_steamIDNil,
                                        int eAvatarSize = k_EAvatarSizeMAX,
                                        uint32_t loadType = 0x0)
{
    return Steam_Overlay::display_imgui_image(displayImageTypeAvatar,
                                              xSize,
                                              ySize,
                                              image_color_multipler_r,
                                              image_color_multipler_g,
                                              image_color_multipler_b,
                                              image_color_multipler_a,
                                              "",
                                              userID,
                                              eAvatarSize,
                                              NULL,
                                              0x0,
                                              0x0,
                                              0x0,
                                              loadType);
}

int Steam_Overlay::display_imgui_custom_image(float xSize,
                                              float ySize,
                                              float image_color_multipler_r,
                                              float image_color_multipler_g,
                                              float image_color_multipler_b,
                                              float image_color_multipler_a,
                                              uint8 * imageData = NULL,
                                              uint32_t imageDataLength = 0x0,
                                              uint32_t imageDataWidth = 0x0,
                                              uint32_t imageDataHeight = 0x0)
{
    return Steam_Overlay::display_imgui_image(displayImageTypeCustom,
                                              xSize,
                                              ySize,
                                              image_color_multipler_r,
                                              image_color_multipler_g,
                                              image_color_multipler_b,
                                              image_color_multipler_a,
                                              "",
                                              k_steamIDNil,
                                              k_EAvatarSizeMAX,
                                              imageData,
                                              imageDataLength,
                                              imageDataWidth,
                                              imageDataHeight,
                                              0x0);
}

int Steam_Overlay::display_imgui_image(uint32_t displayImageType,
                                       float xSize,
                                       float ySize,
                                       float image_color_multipler_r,
                                       float image_color_multipler_g,
                                       float image_color_multipler_b,
                                       float image_color_multipler_a,
                                       std::string achName = "",
                                       CSteamID userID = k_steamIDNil,
                                       int eAvatarSize = k_EAvatarSizeMAX,
                                       uint8 * imageData = NULL,
                                       uint32_t imageDataLength = 0x0,
                                       uint32_t imageDataWidth = 0x0,
                                       uint32_t imageDataHeight = 0x0,
                                       uint32_t loadType = 0x0)
{
    int ret = 0;
    bool valid_args = false;
    ImVec2 image_size(xSize, ySize);
    ImVec4 image_color_multipler(image_color_multipler_r,
                                 image_color_multipler_g,
                                 image_color_multipler_b,
                                 image_color_multipler_a);

    switch (displayImageType) {
        case displayImageTypeAchievement:
            // Achievements
            if (achName != "") {
                valid_args = true;
                for (auto & x : achievements) {
                    if (x.name == achName) {
                        if (loadType == 0x0) {
                            if (x.raw_image == NULL) {
                                LoadAchievementImage(x);
                            }
                        }

                        if (x.image_resource.expired() == true) {
                            CreateAchievementImageResource(x);
                        }

                        if ((x.image_resource.expired() == false) &&
                            (x.raw_image_width > 0) &&
                            (x.raw_image_height > 0)) {
                            std::shared_ptr<uint64_t> s_ptr = x.image_resource.lock();
                            ImGui::Image((ImTextureID)(intptr_t)*s_ptr,
                                         image_size,
                                         ImVec2(0.0f, 0.0f),
                                         ImVec2(1.0f, 1.0f),
                                         image_color_multipler);
                            ret = 1;
                        }
                        break;
                    }
                }
            }
            break;
        case displayImageTypeAvatar:
            // User Avatars
            if (userID != k_steamIDNil) {
                bool found_profile_images = false;
                Profile_Image image;
                {
                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex); // Can't hold this when calling LoadProfileImage().
                    for (auto & x : profile_images) {
                        if (x.first.ConvertToUint64() == userID.ConvertToUint64()) {
                            if (eAvatarSize == k_EAvatarSize32x32) {
                                image = x.second.small;
                                valid_args = true;
                            } else {
                                if (eAvatarSize == k_EAvatarSize64x64) {
                                    image = x.second.medium;
                                    valid_args = true;
                                } else {
                                    if (eAvatarSize == k_EAvatarSize184x184) {
                                        image = x.second.large;
                                        valid_args = true;
                                    } else {
                                        valid_args = false;
                                    }
                                }
                            }
                            found_profile_images = true;
                            break;
                        }
                    }
                }

                if (found_profile_images == false) {
                    populate_initial_profile_images(userID);
                } else {
                    if (valid_args) {
                        bool reload = false;
                        if (image.raw_image == NULL ||
                                image.width == 0 ||
                                image.height == 0) {
                            if (loadType == 0x0) {
                                if (LoadProfileImage(userID, eAvatarSize) == true) {
                                    reload = true;
                                    PRINT_DEBUG("%s %d %s %" PRIu64 " %s\n",
                                                "Steam_Overlay::display_imgui_image Got avatar image size",
                                                eAvatarSize,
                                                "for user",
                                                userID.ConvertToUint64(),
                                                ". Load OK.");
                                } else {
                                    PRINT_DEBUG("%s %d %s %" PRIu64 " %s\n",
                                                "Steam_Overlay::display_imgui_image Unable to get avatar image size",
                                                eAvatarSize,
                                                "for user",
                                                userID.ConvertToUint64(),
                                                ". Load Failure.");
                                }
                            } else {
                                uint32_t type = 0x0;
                                switch (eAvatarSize) {
                                    case k_EAvatarSize32x32:
                                        type |= 0x1;
                                        break;
                                    case k_EAvatarSize64x64:
                                        type |= 0x2;
                                        break;
                                    case k_EAvatarSize184x184:
                                        type |= 0x4;
                                        break;
                                    default:
                                        type = 0x0;
                                        break;
                                };

                                if (type != 0x0) {
                                    PRINT_DEBUG("%s %" PRIu64 " %s %d.\n",
                                                "Steam_Overlay::display_imgui_image Queuing Lazy Load avatar image for",
                                                userID.ConvertToUint64(),
                                                "size",
                                                eAvatarSize);
                                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex); // Can't hold this when calling LoadProfileImage().
                                    auto ld = lazy_load_avatar_images.find(userID);
                                    if (ld == lazy_load_avatar_images.end()) {
                                        type |= lazy_load_avatar_images[userID] & 0x7;
                                        lazy_load_avatar_images[userID] = type;
                                    } else {
                                        lazy_load_avatar_images[userID] = type;
                                    }
                                }
                            }
                        }

                        // Re-get list in case the map buffer was reallocated.
                        if (reload) {
                            found_profile_images = false;
                            valid_args = false;
                            {
                                std::lock_guard<std::recursive_mutex> lock(overlay_mutex); // Can't hold this when calling LoadProfileImage().
                                for (auto & x : profile_images) {
                                    if (x.first.ConvertToUint64() == userID.ConvertToUint64()) {
                                        if (eAvatarSize == k_EAvatarSize32x32) {
                                            image = x.second.small;
                                            valid_args = true;
                                        } else {
                                            if (eAvatarSize == k_EAvatarSize64x64) {
                                                image = x.second.medium;
                                                valid_args = true;
                                            } else {
                                                if (eAvatarSize == k_EAvatarSize184x184) {
                                                    image = x.second.large;
                                                    valid_args = true;
                                                } else {
                                                    valid_args = false;
                                                }
                                            }
                                        }
                                        found_profile_images = true;
                                        break;
                                    }
                                }
                            }
                        }

                        reload = false;
                        if (found_profile_images && valid_args && image.raw_image != NULL && image.width > 0 && image.height > 0) {
                            //if (!image.image_resource) {
                            if (image.image_resource.expired() == true) {

                                if (CreateProfileImageResource(userID, eAvatarSize) == true) {
                                    reload = true;
                                    PRINT_DEBUG("%s %d %s %" PRIu64 " %s\n",
                                                "Steam_Overlay::display_imgui_image Got profile image size",
                                                eAvatarSize,
                                                "for user",
                                                userID.ConvertToUint64(),
                                                ". Resource OK.");
                                } else {
                                    PRINT_DEBUG("%s %d %s %" PRIu64 " %s\n",
                                                "Steam_Overlay::display_imgui_image Unable to get avatar image size",
                                                eAvatarSize,
                                                "for user",
                                                userID.ConvertToUint64(),
                                                ". Resource creation failure.");
                                }
                            }

                            // Re-get list in case the image resource was reallocated.
                            if (reload) {
                                found_profile_images = false;
                                valid_args = false;
                                {
                                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex); // Can't hold this when calling LoadProfileImage().
                                    for (auto & x : profile_images) {
                                        if (x.first.ConvertToUint64() == userID.ConvertToUint64()) {
                                            if (eAvatarSize == k_EAvatarSize32x32) {
                                                image = x.second.small;
                                                valid_args = true;
                                            } else {
                                                if (eAvatarSize == k_EAvatarSize64x64) {
                                                    image = x.second.medium;
                                                    valid_args = true;
                                                } else {
                                                    if (eAvatarSize == k_EAvatarSize184x184) {
                                                        image = x.second.large;
                                                        valid_args = true;
                                                    } else {
                                                        valid_args = false;
                                                    }
                                                }
                                            }
                                            found_profile_images = true;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (valid_args && found_profile_images && image.image_resource.expired() == false) {
                                std::shared_ptr<uint64_t> test2;
                                test2 = image.image_resource.lock();
                                if (test2) {
                                    ImGui::Image((ImTextureID)(intptr_t)*(test2),
                                                 image_size,
                                                 ImVec2(0.0f, 0.0f),
                                                 ImVec2(1.0f, 1.0f),
                                                 image_color_multipler);
                                    ret = 1;
                                }
                            } else {
                                PRINT_DEBUG("%s %d %s %" PRIu64 " %s\n",
                                            "Steam_Overlay::display_imgui_image Unable to get avatar image size",
                                            eAvatarSize,
                                            "for user",
                                            userID.ConvertToUint64(),
                                            ". Resource Invalid.");
                            }
                        } else {
                            if (loadType == 0x0) {
                                PRINT_DEBUG("%s %d %s %" PRIu64 " %s\n",
                                            "Steam_Overlay::display_imgui_image Unable to display avatar image size",
                                            eAvatarSize,
                                            "for user",
                                            userID.ConvertToUint64(),
                                            ". Could not find buffer after pixel data load.");
                            }
                        }
                    }
                }
            }
            break;
        case displayImageTypeCustom:
            // Custom image.
            if (imageData != NULL) {
                valid_args = true;
                {
                    Temporary_Image new_temp_image;
                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                    auto result = temp_display_images.find(imageData);
                    if (result != temp_display_images.end()) {
                        result->second.last_display_time = std::chrono::steady_clock::now();
                    } else {
                        if (imageDataLength > 0) {
                            uint8 * new_pixels = new uint8[(imageDataWidth * imageDataHeight * sizeof(uint32_t))];
                            if (new_pixels != NULL) {
                                for (size_t x = 0; x < (imageDataWidth * imageDataHeight * sizeof(uint32_t)) && x < imageDataLength; x++) {
                                    new_pixels[x] = imageData[x];
                                }
                                new_temp_image.image_data.raw_image = new_pixels;
                                new_temp_image.image_data.width = imageDataWidth;
                                new_temp_image.image_data.height = imageDataHeight;
                                new_temp_image.last_display_time = std::chrono::steady_clock::now();
                                temp_display_images[imageData] = new_temp_image;
                            } else {
                                valid_args = false;
                                PRINT_DEBUG("%s %p. %s.\n",
                                            "Steam_Overlay::display_imgui_image Unable to display custom image",
                                            imageData,
                                            "Could not allocate memory");
                            }
                        } else {
                            valid_args = false;
                            PRINT_DEBUG("%s %p. %s.\n",
                                        "Steam_Overlay::display_imgui_image Unable to display custom image",
                                        imageData,
                                        "Image length not given, and image is not registered in temporary mappings");
                        }
                    }
                }
                if (valid_args == true) {
                    {
                        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                        auto result = temp_display_images.find(imageData);
                        if (result != temp_display_images.end()) {
                            if (result->second.image_data.image_resource.expired() == true) {
                                if (_renderer &&
                                    result->second.image_data.raw_image != NULL &&
                                    result->second.image_data.width > 0 &&
                                    result->second.image_data.height > 0) {
                                    std::weak_ptr<uint64_t> test;
                                    test = _renderer->CreateImageResource(result->second.image_data.raw_image,
                                                                          result->second.image_data.width,
                                                                          result->second.image_data.height);
                                    std::shared_ptr<uint64_t> test2;
                                    test2 = test.lock();
                                    if (!test2) {
                                        PRINT_DEBUG("Steam_Overlay::display_imgui_image Unable to create resource for custom image %p.\n",
                                                    imageData);
                                        _renderer->ReleaseImageResource(test);
                                    } else {
                                        PRINT_DEBUG("Steam_Overlay::display_imgui_image created resource for custom image %p -> %" PRIu64 ".\n",
                                                    imageData,
                                                    *test2);
                                        result->second.image_data.image_resource = test;
                                    }
                                }
                            }
                        }
                    }
                    {
                        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                        auto result = temp_display_images.find(imageData);
                        if (result != temp_display_images.end() &&
                            result->second.image_data.raw_image != NULL &&
                            result->second.image_data.width > 0 &&
                            result->second.image_data.height > 0 &&
                            result->second.image_data.image_resource.expired() == false) {
                            std::shared_ptr<uint64_t> test2;
                            test2 = result->second.image_data.image_resource.lock();
                            if (test2) {
                                ImGui::Image((ImTextureID)(intptr_t)*(test2),
                                             image_size,
                                             ImVec2(0.0f, 0.0f),
                                             ImVec2(1.0f, 1.0f),
                                             image_color_multipler);
                                ret = 1;
                            }
                        }
                    }
                }
            }
            break;
        case displayImageTypeEND:
        default:
            PRINT_DEBUG("%s %d.\n",
                        "Steam_Overlay::display_imgui_image Unrecognized displayImageType: ",
                        displayImageType);
            break;
    };

    return ret;
}

// Try to make this function as short as possible or it might affect game's fps.
void Steam_Overlay::OverlayProc()
{
    if (!Ready())
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (have_notifications) {
        ImGui::PushFont(font_notif);
        BuildNotifications(io.DisplaySize.x, io.DisplaySize.y);
        ImGui::PopFont();
    }

    if (show_overlay)
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        // Set the overlay windows to the size of the game window
        ImGui::SetNextWindowPos({ 0,0 });
        ImGui::SetNextWindowSize({ static_cast<float>(io.DisplaySize.x),
                                   static_cast<float>(io.DisplaySize.y) });

        ImGui::SetNextWindowBgAlpha(0.50);

        ImGui::PushFont(font_default);

        bool show = true;

        if (ImGui::Begin("SteamOverlay", &show, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            bool user_image_displayed = false;
            CSteamID local_user_id = settings->get_local_steam_id();
            ImGuiStyle currentStyle = ImGui::GetStyle();

            user_image_displayed = (display_imgui_avatar(50.0f, 50.0f,
                                                         1.0f, 1.0f, 1.0f, 1.0f,
                                                         local_user_id,
                                                         k_EAvatarSize184x184) == 1);

            {
                std::lock_guard<std::recursive_mutex> lock(overlay_mutex);

                ImGui::LabelText("##label", "Username: %s(%llu) playing %u",
                    settings->get_local_name(),
                    local_user_id.ConvertToUint64(),
                    settings->get_local_game_id().AppID());
                ImGui::SameLine();

                ImGui::LabelText("##label", "Renderer: %s", (_renderer == nullptr ? "Unknown" : _renderer->GetLibraryName().c_str()));

                if (total_achievement_count > 0) {
                    ImGui::Text("Achievements earned: %d / %d", earned_achievement_count, total_achievement_count);
                    ImGui::SameLine();
                    ImGui::ProgressBar((earned_achievement_count / total_achievement_count), ImVec2((io.DisplaySize.x * 0.20f),0));
                } else {
                    ImGui::Text("Achievements not loaded. Unable to display statistics.");
                }

                ImGui::Spacing();
                if (total_achievement_count <= 0) {
                    ImGui::BeginDisabled();
                }
                if (ImGui::Button("Show Achievements")) {
                    show_achievements = true;
                }
                if (total_achievement_count <= 0) {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine();

                if (ImGui::Button("Settings")) {
                    show_settings = true;
                }

                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::LabelText("##label", "Friends");

                if (!friends.empty())
                {
                    if (ImGui::ListBoxHeader("##label", friends.size()))
                    {
                        std::for_each(friends.begin(), friends.end(), [this](std::pair<Friend const, friend_window_state> &i)
                        {
                            ImGui::PushID(i.second.id-base_friend_window_id+base_friend_item_id);

                            // Selectable "" + SameLine + Image + Text == all highlighted.
                            // See also: https://github.com/ocornut/imgui/issues/1658#issuecomment-373731255
                            ImGui::Selectable("", false, ImGuiSelectableFlags_AllowDoubleClick);
                            BuildContextMenu(i.first, i.second);
                            if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                            {
                                i.second.window_state |= window_state_show;
                            }
                            ImGui::PopID();

                            ImGui::SameLine();

                            if (display_imgui_avatar(32 * 0.4f, 32 * 0.4f,
                                                     1.0f, 1.0f, 1.0f, 1.0f,
                                                     CSteamID((uint64)i.first.id()),
                                                     k_EAvatarSize32x32,
                                                     0x1) != 1) {
                                ImGui::ColorButton("test", ImVec4(255, 0, 0, 255), 0, ImVec2(32 * 0.4f, 32 * 0.4f));
                                PRINT_DEBUG("%s %" PRIu64 ".\n",
                                            "SteamOverlay::OverlayProc Failed to display friend avatar for",
                                            (uint64)i.first.id());
                            }
                            ImGui::SameLine();

                            ImGui::Text(i.second.window_title.c_str());

                            BuildFriendWindow(i.first, i.second);
                        });
                        ImGui::ListBoxFooter();
                    }
                }

                if (show_achievements && achievements.size()) {
                    ImGui::SetNextWindowSizeConstraints(ImVec2(ImGui::GetFontSize() * 32, ImGui::GetFontSize() * 32), ImVec2(8192, 8192));
                    bool show = show_achievements;
                    if (ImGui::Begin("Achievement Window", &show)) {
                        ImGui::Text("List of achievements");
                        ImGui::BeginChild("Achievements");
                        float window_x_offset = (ImGui::GetWindowWidth() > currentStyle.ScrollbarSize) ? (ImGui::GetWindowWidth() - currentStyle.ScrollbarSize) : 0;
                        for (auto & x : achievements) {
                            bool achieved = x.achieved;
                            bool hidden = x.hidden && !achieved;

                            if (!hidden || show_achievement_hidden_unearned) {

                                ImGui::Separator();

                                // Anchor the image to the right side of the list.
                                ImVec2 target = ImGui::GetCursorPos();
                                target.x = window_x_offset;
                                if (target.x > (256 * 0.4f)) {
                                    target.x = target.x - (256 * 0.4f);
                                } else {
                                    target.x = 0;
                                }

                                ImGui::PushTextWrapPos(target.x);
                                ImGui::Text("%s", x.title.c_str());
                                if (hidden) {
                                    ImGui::Text("Hidden Achievement");
                                } else {
                                    ImGui::Text("%s", x.description.c_str());
                                }
                                ImGui::PopTextWrapPos();

                                if (achieved) {
                                    char buffer[80] = {};
                                    time_t unlock_time = (time_t)x.unlock_time;
                                    std::strftime(buffer, 80, "%Y-%m-%d at %H:%M:%S", std::localtime(&unlock_time));

                                    ImGui::TextColored(ImVec4(0, 255, 0, 255), "achieved on %s", buffer);
                                } else {
                                    ImGui::TextColored(ImVec4(255, 0, 0, 255), "not achieved");
                                }

                                // Set cursor for image output.
                                if (target.x != 0) {
                                    ImGui::SetCursorPos(target);
                                }

                                display_imgui_achievement(256 * 0.4f, 256 * 0.4f,
                                                          1.0f, 1.0f, 1.0f, 1.0f,
                                                          x.name);

                                ImGui::Separator();
                            }
                        }
                        ImGui::EndChild();
                    }
                    ImGui::End();
                    show_achievements = show;
                }

            }

            if (show_settings) {
                if (ImGui::Begin("Global Settings Window", &show_settings)) {
                    ImGui::Text("These are global emulator settings and will apply to all games.");

                    ImGui::Separator();

                    {
                        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);

                        ImGui::Text("Username:");
                        ImGui::SameLine();
                        ImGui::InputText("##username", username_text, sizeof(username_text), disable_forced ? ImGuiInputTextFlags_ReadOnly : 0);

                        ImGui::Separator();

                        ImGui::Text("Language:");

                        if (ImGui::ListBox("##language", &current_language, valid_languages, sizeof(valid_languages) / sizeof(char *), 7)) {

                        }

                        ImGui::Text("Selected Language: %s", valid_languages[current_language]);

                        ImGui::Separator();

                        ImGui::LabelText("##notificationpositions", "Notification Positions");

                        if (ImGui::BeginListBox("##notificationpositions", ImVec2(-0.1f, ImGui::GetTextLineHeightWithSpacing() * 4.25f + currentStyle.FramePadding.y * 2.0f))) {
                            for (size_t x = 0; x < (sizeof(valid_ui_notification_position_labels) / sizeof(char*)); x++) {
                                bool sel = false;
                                sel = ImGui::Selectable(valid_ui_notification_position_labels[x],
                                                        (current_ui_notification_position_selection == x),
                                                        ImGuiSelectableFlags_AllowDoubleClick);
                                if (sel) {
                                    PRINT_DEBUG("%s %s.\n",
                                                "SteamOverlay::OverlayProc click on notification positions item",
                                                valid_ui_notification_position_labels[x]);
                                    switch (x) {
                                        case 0:
                                            notif_position = k_EPositionTopLeft;
                                            current_ui_notification_position_selection = 0;
                                            break;
                                        case 1:
                                            notif_position = k_EPositionTopRight;
                                            current_ui_notification_position_selection = 1;
                                            break;
                                        case 2:
                                            notif_position = k_EPositionBottomLeft;
                                            current_ui_notification_position_selection = 2;
                                            break;
                                        case 3:
                                            notif_position = k_EPositionBottomRight;
                                            current_ui_notification_position_selection = 3;
                                            break;
                                        default:
                                            break;
                                    };
                                }
                            }

                            ImGui::EndListBox();
                        }

                        ImGui::Text("Selected notification position: %s", valid_ui_notification_position_labels[current_ui_notification_position_selection]);

                        ImGui::Separator();

                        ImGui::Checkbox("Show achievement descriptions on unlock", &show_achievement_desc_on_unlock);
                        ImGui::Checkbox("Show unearned hidden achievements", &show_achievement_hidden_unearned);

                        ImGui::Separator();

                        if (ImGui::Button("Set User Profile Image")) {
                            show_profile_image_select = true;
                        }

                        ImGui::Separator();

                        if (!disable_forced) {
                            ImGui::Text("You may have to restart the game for these to apply.");
                            if (ImGui::Button("Save")) {
                                save_settings = true;
                                show_settings = false;
                                show_profile_image_select = false;
                            }
                        } else {
                            ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                            ImGui::TextWrapped("Some steam_settings/force_*.txt files have been detected. Please delete them if you want this menu to work.");
                            ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                        }
                    }
                }

                ImGui::End();

                if (show_profile_image_select) {
                    bool shown_drive_list = false;
                    {
                        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);

                        cleared_new_profile_images_struct = false;
                        shown_drive_list = show_drive_list;

                        if (shown_drive_list) {
                            std::vector<std::string> temp = Local_Storage::get_drive_list();
                            for (auto x : temp) {
                                filesystem_list[x] = false;
                            }
                        } else {
                            if (current_path.length() <= 0) {
                                filesystem_list.clear();
                                if (future_path.length() > 0) {
                                    current_path = future_path;
                                    future_path.clear();
                                } else {
                                    current_path = Local_Storage::get_user_pictures_path();
                                }
                            }

                            if (filesystem_list.size() <= 0) {
                                std::vector<std::string> temp = Local_Storage::get_filenames_path(current_path);
                                for (auto x : temp) {
                                    filesystem_list[x] = false;
                                }
                                filesystem_list[".."] = false;
                            }
                        }
                    }

                    if (ImGui::Begin("Set User Profile Image", &show_profile_image_select, ImGuiWindowFlags_AlwaysAutoResize)) {

                        std::string status_message = "";
                        bool small_image_displayed = false;
                        bool medium_image_displayed = false;
                        bool large_image_displayed = false;
                        user_image_displayed = false;
                        bool found_profile_images = false;

                        ImGui::Text("Currently Used Profile Images:");
                        small_image_displayed = (display_imgui_avatar(50.0f, 50.0f,
                                                                      1.0f, 1.0f, 1.0f, 1.0f,
                                                                      local_user_id,
                                                                      k_EAvatarSize32x32) == 1);
                        if (small_image_displayed == false) {
                            ImGui::ColorButton("test", ImVec4(255, 0, 0, 255), 0, ImVec2(50.0f, 50.0f));
                        }
                        ImGui::SameLine();

                        medium_image_displayed = (display_imgui_avatar(70.0f, 70.0f,
                                                                       1.0f, 1.0f, 1.0f, 1.0f,
                                                                       local_user_id,
                                                                       k_EAvatarSize64x64) == 1);
                        if (medium_image_displayed == false) {
                            ImGui::ColorButton("test", ImVec4(255, 0, 0, 255), 0, ImVec2(70.0f, 70.0f));
                        }
                        ImGui::SameLine();

                        large_image_displayed = (display_imgui_avatar(90.0f, 90.0f,
                                                                      1.0f, 1.0f, 1.0f, 1.0f,
                                                                      local_user_id,
                                                                      k_EAvatarSize184x184) == 1);
                        if (large_image_displayed == false) {
                            ImGui::ColorButton("test", ImVec4(255, 0, 0, 255), 0, ImVec2(90.0f, 90.0f));
                        }
                        ImGui::SameLine();
                        user_image_displayed = (small_image_displayed && medium_image_displayed && large_image_displayed);
                        ImGui::Separator();

                        ImGui::TextWrapped("Select image size to configure:");
                        ImGui::SameLine();

                        {
                            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                            if (ImGui::RadioButton("32x32 (Small)", (radio_btn_new_profile_image_size[0] == true)) == true) {
                                radio_btn_new_profile_image_size[0] = true;
                                radio_btn_new_profile_image_size[1] = false;
                                radio_btn_new_profile_image_size[2] = false;
                            }
                            ImGui::SameLine();
                            if (ImGui::RadioButton("64x64 (Medium)", (radio_btn_new_profile_image_size[1] == true)) == true) {
                                radio_btn_new_profile_image_size[1] = true;
                                radio_btn_new_profile_image_size[0] = false;
                                radio_btn_new_profile_image_size[2] = false;
                            }
                            ImGui::SameLine();
                            if (ImGui::RadioButton("184x184 (Large)", (radio_btn_new_profile_image_size[2] == true)) == true) {
                                radio_btn_new_profile_image_size[2] = true;
                                radio_btn_new_profile_image_size[0] = false;
                                radio_btn_new_profile_image_size[1] = false;
                            }
                        }
                        ImGui::TextWrapped("Note: Selected image must have a resolution bigger than the previous option and less than or equal to the selected option.");
                        ImGui::Separator();

                        bool load_image_file = false;
                        ImGui::TextWrapped("Please select a JPG or PNG file");
                        {
                            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                            if (new_profile_image_path.length() > 0) {
                                load_image_file = true;
                            }
                            std::string path_message = "Current Path: " + current_path;
                            ImGui::TextWrapped(path_message.c_str());
                        }
                        if (ImGui::Button("Up")) {
                            std::string temp = Local_Storage::get_parent_directory(current_path);
                            PRINT_DEBUG("%s\n", "SteamOverlay::OverlayProc Image Select Up button.");
                            if (Local_Storage::is_directory(temp) == true) {
                                PRINT_DEBUG("%s %s %s.\n", "SteamOverlay::OverlayProc ", temp.c_str(), " is a directory");
                                std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                                current_path.clear();
                                future_path = temp;
                                show_drive_list = false;
                            } else {
                                PRINT_DEBUG("%s %s %s.\n", "SteamOverlay::OverlayProc ", temp.c_str(), " is NOT a directory");
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Home")) {
                            PRINT_DEBUG("%s.\n", "SteamOverlay::OverlayProc Image Select Home button");
                            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                            future_path = Local_Storage::get_user_pictures_path();
                            if (future_path.length() > 0) {
                                current_path.clear();
                                show_drive_list = false;
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Drives")) {
                            PRINT_DEBUG("%s.\n", "SteamOverlay::OverlayProc Image Select Drives button");
                            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                            show_drive_list = true;
                            current_path.clear();
                            future_path.clear();
                            filesystem_list.clear();
                        }
                        if (ImGui::BeginListBox("##FileSelect")) {
                            {
                                std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                                for (auto x : filesystem_list) {
                                    if (x.first.length() > 0) {
                                        x.second = ImGui::Selectable(x.first.c_str(), x.second, ImGuiSelectableFlags_AllowDoubleClick);

                                        if (x.second) {
                                            PRINT_DEBUG("%s %s.\n", "SteamOverlay::OverlayProc click on filesystem list item", x.first.c_str());
                                            if (shown_drive_list) {
                                                future_path = ((x.first.find_last_of((char)PATH_SEPARATOR) == (x.first.length())) ? (x.first) : (x.first + PATH_SEPARATOR));
                                                if (Local_Storage::is_directory(future_path) == true) {
                                                    current_path.clear();
                                                    show_drive_list = false;
                                                    PRINT_DEBUG("%s %s %" PRI_ZU " %" PRI_ZU ".\n", "SteamOverlay::OverlayProc Next directory will be", future_path.c_str(), x.first.find_last_of((char)PATH_SEPARATOR), x.first.length());
                                                } else {
                                                    future_path.clear();
                                                }
                                            } else {
                                                if (x.first == "..") {
                                                    future_path = Local_Storage::get_parent_directory(current_path);
                                                    PRINT_DEBUG("%s %s is %s.\n", "SteamOverlay::OverlayProc parent directory of", current_path.c_str(), future_path.c_str());
                                                    if (Local_Storage::is_directory(future_path) == true) {
                                                        PRINT_DEBUG("%s %s %s.\n", "SteamOverlay::OverlayProc ", future_path.c_str(), "is a directory");
                                                        current_path.clear();
                                                    } else {
                                                        PRINT_DEBUG("%s %s %s.\n", "SteamOverlay::OverlayProc ", future_path.c_str(), "is NOT a directory");
                                                        future_path.clear();
                                                    }
                                                } else {
                                                    std::string temp_path = ((x.first.find_last_of((char)PATH_SEPARATOR) == (x.first.length())) ? (current_path + x.first) : (current_path + PATH_SEPARATOR + x.first));
                                                    if (Local_Storage::is_directory(temp_path) == true) {
                                                        future_path = temp_path;
                                                        current_path.clear();
                                                        PRINT_DEBUG("%s %s.\n", "SteamOverlay::OverlayProc Next directory will be", future_path.c_str());
                                                    } else {
                                                        // Load file.
                                                        new_profile_image_path = temp_path;
                                                        status_message.clear();
                                                        tried_load_new_profile_image = false;
                                                        load_image_file = true;
                                                        PRINT_DEBUG("%s %s.\n", "SteamOverlay::OverlayProc Load file at", new_profile_image_path.c_str());
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            ImGui::EndListBox();

                            ImGui::Separator();
                        }

                        if (load_image_file == true) {
                            Profile_Image image;
                            bool loaded_new_profile_image = false;
                            int new_image_size = k_EAvatarSizeMAX;
                            int new_image_pix_size_max = 0;
                            int new_image_pix_size_min = 0;
                            uint32_t new_profile_image_width = 0;
                            uint32_t new_profile_image_height = 0;
                            std::string image_path = "";
                            {
                                std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                                if (radio_btn_new_profile_image_size[0]) {
                                    new_image_size = k_EAvatarSize32x32;
                                    new_image_pix_size_max = 32;
                                    new_image_pix_size_min = 0;
                                    image = new_profile_image_handles.small;
                                } else {
                                    if (radio_btn_new_profile_image_size[1]) {
                                        new_image_size = k_EAvatarSize64x64;
                                        new_image_pix_size_max = 64;
                                        new_image_pix_size_min = 32;
                                        image = new_profile_image_handles.medium;
                                    } else {
                                        if (radio_btn_new_profile_image_size[2]) {
                                            new_image_size = k_EAvatarSize184x184;
                                            new_image_pix_size_max = 184;
                                            new_image_pix_size_min = 64;
                                            image = new_profile_image_handles.large;
                                        } else {
                                            new_image_size = k_EAvatarSizeMAX;
                                            new_image_pix_size_max = 0;
                                            new_image_pix_size_min = 0;
                                            image = Profile_Image();
                                        }
                                    }
                                }
                                image_path = new_profile_image_path;
                                loaded_new_profile_image = tried_load_new_profile_image;
                            }

                            if (image_path.length() > 0 && new_image_size != k_EAvatarSizeMAX) {
                                if (loaded_new_profile_image == false) {
                                    PRINT_DEBUG("%s\n", "SteamOverlay::OverlayProc New image load stage 1.");
                                    {
                                        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                                        DestroyTemporaryImage(image.raw_image);
                                        image.width = 0;
                                        image.height = 0;
                                        if (radio_btn_new_profile_image_size[0]) {
                                            new_profile_image_handles.small = image;
                                        } else {
                                            if (radio_btn_new_profile_image_size[1]) {
                                                new_profile_image_handles.medium = image;
                                            } else {
                                                if (radio_btn_new_profile_image_size[2]) {
                                                    new_profile_image_handles.large = image;
                                                }
                                            }
                                        }
                                    }

                                    std::string new_profile_image_str = "";
                                    PRINT_DEBUG("%s %s.\n", "SteamOverlay::OverlayProc Loading new image pixel data from", image_path.c_str());
                                    new_profile_image_str = convert_vector_image_pixel_t_to_std_string(get_steam_client()->local_storage->load_image(image_path, &new_profile_image_width, &new_profile_image_height));
                                    if ((new_profile_image_str.length() > 0) &&
                                        (new_profile_image_width > new_image_pix_size_min) &&
                                            (new_profile_image_width <= new_image_pix_size_max) &&
                                        (new_profile_image_height > new_image_pix_size_min) &&
                                            (new_profile_image_height <= new_image_pix_size_max)) {
                                        uint8 * temp_raw_image = NULL;
                                        temp_raw_image = new uint8[(new_profile_image_width * new_profile_image_height * sizeof(uint32_t))];
                                        if (temp_raw_image != NULL) {
                                            size_t y = 0;
                                            for (auto & x : new_profile_image_str) {
                                                if (y < new_profile_image_str.length() && y < (new_profile_image_width * new_profile_image_height * sizeof(uint32_t))) {
                                                    temp_raw_image[y] = x;
                                                    y++;
                                                }
                                            }
                                            image.width = new_profile_image_width;
                                            image.height = new_profile_image_height;
                                            image.raw_image = temp_raw_image;

                                            status_message = "Loaded selected image.\n";
                                            ImGui::TextWrapped("Selected new avatar image:");
                                            if (display_imgui_custom_image(50.0f, 50.0f,
                                                                           1.0f, 1.0f, 1.0f, 1.0f,
                                                                           image.raw_image,
                                                                           (image.width * image.height * sizeof(uint32_t)),
                                                                           image.width,
                                                                           image.height) == 1) {
                                                std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                                                if (radio_btn_new_profile_image_size[0]) {
                                                    new_profile_image_handles.small = image;
                                                } else {
                                                    if (radio_btn_new_profile_image_size[1]) {
                                                        new_profile_image_handles.medium = image;
                                                    } else {
                                                        if (radio_btn_new_profile_image_size[2]) {
                                                            new_profile_image_handles.large = image;
                                                        }
                                                    }
                                                }
                                            }

                                            delete temp_raw_image;
                                            temp_raw_image = NULL;
                                        } else {
                                            PRINT_DEBUG("%s\n", "SteamOverlay::OverlayProc Unable to allocate memory for new image.");
                                            new_profile_image_str.clear();
                                            new_profile_image_width = 0;
                                            new_profile_image_height = 0;
                                        }
                                    } else {
                                        PRINT_DEBUG("%s\n", "SteamOverlay::OverlayProc Unable to load new image. Invalid size, or invalid image.");
                                        new_profile_image_str.clear();
                                        new_profile_image_width = 0;
                                        new_profile_image_height = 0;
                                        status_message = "Failed to load selected image. Invalid size, or invalid image.\n";
                                    }
                                    {
                                        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                                        tried_load_new_profile_image = true;
                                    }
                                } else {
                                    PRINT_DEBUG("%s\n", "SteamOverlay::OverlayProc New image load stage 2.");
                                    if (image.raw_image != NULL && image.width > 0 && image.height > 0) {
                                        status_message = "Loaded selected image.\n";
                                        ImGui::TextWrapped("Selected new avatar image:");
                                        display_imgui_custom_image(50.0f, 50.0f,
                                                                   1.0f, 1.0f, 1.0f, 1.0f,
                                                                   image.raw_image,
                                                                   0,
                                                                   image.width,
                                                                   image.height);
                                        if (ImGui::Button("Apply")) {
                                            Profile_Image temp_pi = GetTemporaryImage(image.raw_image);
                                            if (temp_pi.raw_image != NULL && temp_pi.width > 0 && temp_pi.height > 0) {
                                                Image_Data id;
                                                //id.data = (char*)temp_pi.raw_image; Not a deep copy....
                                                id.data.clear();
                                                for (size_t x = 0; x < (temp_pi.width * temp_pi.height * sizeof(uint32_t)); x++) {
                                                    char a = (char)(temp_pi.raw_image[x]);
                                                    id.data += a;
                                                }
                                                id.width = temp_pi.width;
                                                id.height = temp_pi.height;
                                                PRINT_DEBUG("%s %d %s %" PRIu64 ".\n",
                                                            "SteamOverlay::OverlayProc Attempting to set new avatar image size",
                                                            new_image_size,
                                                            "for profile",
                                                            local_user_id.ConvertToUint64());
                                                if (new_image_size == k_EAvatarSize32x32) {
                                                    std::lock_guard<std::recursive_mutex> glock(global_mutex);
                                                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                                                    if (settings->set_profile_image(k_EAvatarSize32x32, &id)) {
                                                        new_profile_image_path.clear();
                                                        save_small_profile_image = true;
                                                        show_profile_image_select = false;
                                                    } else {
                                                        status_message = "Failed to set active avatar image.\n";
                                                    }
                                                }
                                                if (new_image_size == k_EAvatarSize64x64) {
                                                    std::lock_guard<std::recursive_mutex> glock(global_mutex);
                                                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                                                    if (settings->set_profile_image(k_EAvatarSize64x64, &id)) {
                                                        new_profile_image_path.clear();
                                                        save_medium_profile_image = true;
                                                        show_profile_image_select = false;
                                                    } else {
                                                        status_message = "Failed to set active avatar image.\n";
                                                    }
                                                }
                                                if (new_image_size == k_EAvatarSize184x184) {
                                                    std::lock_guard<std::recursive_mutex> glock(global_mutex);
                                                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                                                    if (settings->set_profile_image(k_EAvatarSize184x184, &id)) {
                                                        new_profile_image_path.clear();
                                                        save_large_profile_image = true;
                                                        show_profile_image_select = false;
                                                    } else {
                                                        status_message = "Failed to set active avatar image.\n";
                                                    }
                                                }
                                                DestroyProfileImageResource(local_user_id, new_image_size);
                                                DestroyProfileImage(local_user_id, new_image_size);
                                                PRINT_DEBUG("%s\n", "Steam_Overlay::OverlayProc End apply new avatar image.");
                                            } else {
                                                PRINT_DEBUG("%s.\n",
                                                            "Steam_Overlay::OverlayProc Unable to set new avatar image. Image data expired");
                                                status_message = "Loaded image data expired. Please reselect the image file in the file chooser above.";
                                            }
                                            ReturnTemporaryImage(temp_pi);
                                        }
                                    } else {
                                        PRINT_DEBUG("%s\n", "SteamOverlay::OverlayProc Invalid image data after load cannot display.");
                                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                                        ImGui::TextWrapped("Could not load file as image. Image file must be either a PNG or JPG and must be less than or equal to selected resolution.");
                                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                                        status_message = "Selected file is not an image, or image has an invalid resolution.";
                                    }
                                }
                            }
                        }
                        if (ImGui::Button("Cancel")) {
                            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                            show_profile_image_select = false;
                        }
                        ImGui::Separator();

                        ImGui::TextWrapped("Status");
                        ImGui::TextWrapped(status_message.c_str());
                        ImGui::Separator();

                        {
                            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                            current_status_message = status_message;
                        }
                    }

                    ImGui::End();
                } else {
                    PRINT_DEBUG("%s\n", "SteamOverlay::OverlayProc Begin new avatar image select cleanup.");
                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                    new_profile_image_path.clear();
                    new_profile_image_handles.small.raw_image = NULL; // Handle
                    new_profile_image_handles.small.width = 0;
                    new_profile_image_handles.small.height = 0;
                    new_profile_image_handles.medium.raw_image = NULL; // Handle
                    new_profile_image_handles.medium.width = 0;
                    new_profile_image_handles.medium.height = 0;
                    new_profile_image_handles.large.raw_image = NULL; // Handle
                    new_profile_image_handles.large.width = 0;
                    new_profile_image_handles.large.height = 0;
                    current_status_message.clear();
                    cleared_new_profile_images_struct = true;
                    PRINT_DEBUG("%s\n", "SteamOverlay::OverlayProc End new avatar image select cleanup.");
                }
            }

            std::string url = show_url;
            if (url.size()) {
                bool show = true;
                if (ImGui::Begin(URL_WINDOW_NAME, &show)) {
                    ImGui::Text("The game tried to get the steam overlay to open this url:");
                    ImGui::Spacing();
                    ImGui::PushItemWidth(ImGui::CalcTextSize(url.c_str()).x + 20);
                    ImGui::InputText("##url_copy", (char *)url.data(), url.size(), ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopItemWidth();
                    ImGui::Spacing();
                    if (ImGui::Button("Close") || !show)
                        show_url = "";
                    // ImGui::SetWindowSize(ImVec2(ImGui::CalcTextSize(url.c_str()).x + 10, 0));
                }
                ImGui::End();
            }

            bool show_warning = local_save || warning_forced || appid == 0;
            if (show_warning) {
                ImGui::SetNextWindowSizeConstraints(ImVec2(ImGui::GetFontSize() * 32, ImGui::GetFontSize() * 32), ImVec2(8192, 8192));
                ImGui::SetNextWindowFocus();
                if (ImGui::Begin("WARNING", &show_warning)) {
                    if (appid == 0) {
                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                        ImGui::TextWrapped("AppID is 0, please create a steam_appid.txt with the right appid and restart the game.");
                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                    }
                    if (local_save) {
                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                        ImGui::TextWrapped("local_save.txt detected, the emu is saving locally to the game folder. Please delete it if you don't want this.");
                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                    }
                    if (warning_forced) {
                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                        ImGui::TextWrapped("Some steam_settings/force_*.txt files have been detected. You will not be able to save some settings.");
                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                    }
                }
                ImGui::End();
                if (!show_warning) {
                    local_save = warning_forced = false;
                }
            }
        }
        ImGui::End();

        ImGui::PopFont();

        if (!show)
            ShowOverlay(false);
    } else {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }
}

void Steam_Overlay::Callback(Common_Message *msg)
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    if (msg->has_steam_messages())
    {
        Friend frd;
        frd.set_id(msg->source_id());
        auto friend_info = friends.find(frd);
        if (friend_info != friends.end())
        {
            Steam_Messages const& steam_message = msg->steam_messages();
            // Change color to cyan for friend
            friend_info->second.chat_history.append(friend_info->first.name() + ": " + steam_message.message()).append("\n", 1);
            if (!(friend_info->second.window_state & window_state_show))
            {
                friend_info->second.window_state |= window_state_need_attention;
            }

            AddMessageNotification(friend_info->first.name() + " says: " + steam_message.message(),
                                   CSteamID((uint64)friend_info->first.id()));
            NotifyUser(friend_info->second);
        }
    }
}

void Steam_Overlay::RunCallbacks()
{
    if (!achievements.size()) {
        Steam_User_Stats* steamUserStats = get_steam_client()->steam_user_stats;
        total_achievement_count = steamUserStats->GetNumAchievements();
        if (total_achievement_count) {
            PRINT_DEBUG("POPULATE OVERLAY ACHIEVEMENTS\n");
            for (unsigned i = 0; i < total_achievement_count; ++i) {
                Overlay_Achievement ach;
                ach.name = steamUserStats->GetAchievementName(i);
                ach.title = steamUserStats->GetAchievementDisplayAttribute(ach.name.c_str(), "name");
                ach.description = steamUserStats->GetAchievementDisplayAttribute(ach.name.c_str(), "desc");
                const char *hidden = steamUserStats->GetAchievementDisplayAttribute(ach.name.c_str(), "hidden");

                if (strlen(hidden) && hidden[0] == '1') {
                    ach.hidden = true;
                } else {
                    ach.hidden = false;
                }

                bool achieved = false;
                uint32 unlock_time = 0;
                if (steamUserStats->GetAchievementAndUnlockTime(ach.name.c_str(), &achieved, &unlock_time)) {
                    ach.achieved = achieved;
                    ach.unlock_time = unlock_time;
                    if ((achieved == true) && (total_achievement_count > earned_achievement_count)) {
                        earned_achievement_count++;
                    }
                } else {
                    ach.achieved = false;
                    ach.unlock_time = 0;
                }

                ach.raw_image = nullptr;
                ach.raw_image_width = 0;
                ach.raw_image_height = 0;
                LoadAchievementImage(ach);

                achievements.push_back(ach);
            }

            PRINT_DEBUG("POPULATE OVERLAY ACHIEVEMENTS DONE\n");
        }
    }

    if (!Ready() && future_renderer.valid()) {
        if (future_renderer.wait_for(std::chrono::milliseconds{0}) ==  std::future_status::ready) {
            _renderer = future_renderer.get();
            PRINT_DEBUG("got renderer %p\n", _renderer);
            CreateFonts();
        }
    }

    if (!Ready() && _renderer) {
            _renderer->OverlayHookReady = std::bind(&Steam_Overlay::HookReady, this, std::placeholders::_1);
            _renderer->OverlayProc = std::bind(&Steam_Overlay::OverlayProc, this);
            auto callback = std::bind(&Steam_Overlay::OpenOverlayHook, this, std::placeholders::_1);
            PRINT_DEBUG("start renderer\n", _renderer);
            std::set<ingame_overlay::ToggleKey> keys = {ingame_overlay::ToggleKey::SHIFT, ingame_overlay::ToggleKey::TAB};
            _renderer->ImGuiFontAtlas = fonts_atlas;
            bool started = _renderer->StartHook(callback, keys);
            PRINT_DEBUG("tried to start renderer %u\n", started);
    }

    if (Ready() && _renderer) {
        bool done = false;
        std::map<CSteamID, uint32_t> temp_lazy;
        {
            std::lock_guard<std::recursive_mutex> lock(overlay_mutex); // Can't hold this when calling LoadProfileImage().
            temp_lazy = lazy_load_avatar_images;
        }
        if (temp_lazy.size() > 0) {
            for (auto x : temp_lazy) {
                uint32_t types = x.second & 0x7;
                uint32_t attempts = x.second & 0x70;
                if (x.first != k_steamIDNil && attempts < 0x70 && types > 0x0) {
                    switch (attempts) {
                        case 0x0:
                            attempts |= 0x10;
                            break;
                        case 0x10:
                            attempts |= 0x20;
                            break;
                        case 0x20:
                            attempts |= 0x40;
                            break;
                        default:
                            attempts = 0x70;
                            break;
                    };
                    uint32_t retry = 0x0;
                    PRINT_DEBUG("%s %" PRIu64 ".\n",
                                "Steam_Overlay::RunCallbacks Lazy loading Friend Avatars for",
                                x.first.ConvertToUint64());
                    if (x.second & 0x1) {
                        if (LoadProfileImage(x.first, k_EAvatarSize32x32) == false) {
                            retry |= 0x1;
                        }
                    }
                    if (x.second & 0x2) {
                        if (LoadProfileImage(x.first, k_EAvatarSize64x64) == false) {
                            retry |= 0x2;
                        }
                    }
                    if (x.second & 0x4) {
                        if (LoadProfileImage(x.first, k_EAvatarSize184x184) == false) {
                            retry |= 0x4;
                        }
                    }

                    {
                        std::lock_guard<std::recursive_mutex> lock(overlay_mutex); // Can't hold this when calling LoadProfileImage().
                        if ((lazy_load_avatar_images[x.first] & 0x7) != types) {
                            // New type requested during load.
                            retry = ((lazy_load_avatar_images[x.first] & 0x7) ^ types) | retry;
                            attempts = 0x0;
                        }
                        if (retry != 0x0) {
                           lazy_load_avatar_images[x.first] = retry | attempts;
                        } else {
                            auto found = lazy_load_avatar_images.find(x.first);
                            if (found != lazy_load_avatar_images.end()) {
                                lazy_load_avatar_images.erase(found);
                            }
                        }
                    }
                } else {
                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex); // Can't hold this when calling LoadProfileImage().
                    auto found = lazy_load_avatar_images.find(x.first);
                    if (found != lazy_load_avatar_images.end()) {
                        lazy_load_avatar_images.erase(found);
                    }
                }
            }
        }
    }

    if (!show_overlay) {
        PruneTemporaryImages();
    }

    if (overlay_state_changed)
    {
        GameOverlayActivated_t data = { 0 };
        data.m_bActive = show_overlay;
        data.m_bUserInitiated = true;
        data.m_nAppID = settings->get_local_game_id().AppID();
        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));
        overlay_state_changed = false;
    }

    Steam_Friends* steamFriends = get_steam_client()->steam_friends;
    Steam_Matchmaking* steamMatchmaking = get_steam_client()->steam_matchmaking;

    if (save_settings) {
        bool temp_show_achievement_desc_on_unlock = false;
        bool temp_show_achievement_hidden_unearned = false;
        char *language_text = NULL;
        char *notification_position_text = NULL;
        int temp_notif_position = 0;
        {
            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
            temp_show_achievement_desc_on_unlock = show_achievement_desc_on_unlock;
            temp_show_achievement_hidden_unearned = show_achievement_hidden_unearned;
            temp_notif_position = notif_position;
            language_text = valid_languages[current_language];
        }
        switch (temp_notif_position) {
            case k_EPositionTopLeft:
                notification_position_text = valid_ui_notification_position_labels[0];
                break;
            case k_EPositionTopRight:
                notification_position_text = valid_ui_notification_position_labels[1];
                break;
            case k_EPositionBottomLeft:
                notification_position_text = valid_ui_notification_position_labels[2];
                break;
            case k_EPositionBottomRight:
                notification_position_text = valid_ui_notification_position_labels[3];
                break;
            default:
                notification_position_text = NULL;
                break;
        };
        get_steam_client()->settings_client->set_local_name(username_text);
        get_steam_client()->settings_client->set_language(language_text);
        if (notification_position_text != NULL) {
            get_steam_client()->settings_client->set_ui_notification_position(notification_position_text);
        }
        get_steam_client()->settings_client->set_show_achievement_desc_on_unlock(temp_show_achievement_desc_on_unlock);
        get_steam_client()->settings_client->set_show_achievement_hidden_unearned(temp_show_achievement_hidden_unearned);
        get_steam_client()->settings_server->set_local_name(username_text);
        get_steam_client()->settings_server->set_language(language_text);
        if (notification_position_text != NULL) {
            get_steam_client()->settings_server->set_ui_notification_position(notification_position_text);
        }
        get_steam_client()->settings_server->set_show_achievement_desc_on_unlock(temp_show_achievement_desc_on_unlock);
        get_steam_client()->settings_server->set_show_achievement_hidden_unearned(temp_show_achievement_hidden_unearned);
        save_global_settings(get_steam_client()->local_storage, get_steam_client()->settings_client);
        bool save_small = false;
        bool save_medium = false;
        bool save_large = false;
        {
            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
            if (save_small_profile_image) save_small = true;
            if (save_medium_profile_image) save_medium = true;
            if (save_large_profile_image) save_large = true;
        }
        if (save_small || save_medium || save_large) {
            CSteamID local_steam_id = settings->get_local_steam_id();
            if (save_small) {
                if (LoadProfileImage(local_steam_id, k_EAvatarSize32x32) == true) {
                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                    get_steam_client()->local_storage->save_avatar_image(k_EAvatarSize32x32,
                                                                         profile_images[local_steam_id].small.width,
                                                                         profile_images[local_steam_id].small.height,
                                                                         profile_images[local_steam_id].small.raw_image);
                    save_small_profile_image = false;
                }
            }
            if (save_medium) {
                if (LoadProfileImage(local_steam_id, k_EAvatarSize64x64) == true) {
                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                    get_steam_client()->local_storage->save_avatar_image(k_EAvatarSize64x64,
                                                                         profile_images[local_steam_id].medium.width,
                                                                         profile_images[local_steam_id].medium.height,
                                                                         profile_images[local_steam_id].medium.raw_image);
                    save_medium_profile_image = false;
                }
            }
            if (save_large) {
                if (LoadProfileImage(local_steam_id, k_EAvatarSize184x184) == true) {
                    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
                    get_steam_client()->local_storage->save_avatar_image(k_EAvatarSize184x184,
                                                                         profile_images[local_steam_id].large.width,
                                                                         profile_images[local_steam_id].large.height,
                                                                         profile_images[local_steam_id].large.raw_image);
                    save_large_profile_image = false;
                }
            }
        }
        steamFriends->resend_friend_data();
        save_settings = false;
    }

    appid = settings->get_local_game_id().AppID();

    i_have_lobby = IHaveLobby();
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    std::for_each(friends.begin(), friends.end(), [this](std::pair<Friend const, friend_window_state> &i)
    {
        i.second.joinable = FriendJoinable(i);
    });

    while (!has_friend_action.empty())
    {
        auto friend_info = friends.find(has_friend_action.front());
        if (friend_info != friends.end())
        {
            uint64 friend_id = friend_info->first.id();
            // The user clicked on "Send"
            if (friend_info->second.window_state & window_state_send_message)
            {
                char* input = friend_info->second.chat_input;
                char* end_input = input + strlen(input);
                char* printable_char = std::find_if(input, end_input, [](char c) {
                    return std::isgraph(c);
                });
                // Check if the message contains something else than blanks
                if (printable_char != end_input)
                {
                    // Handle chat send
                    Common_Message msg;
                    Steam_Messages* steam_messages = new Steam_Messages;
                    steam_messages->set_type(Steam_Messages::FRIEND_CHAT);
                    steam_messages->set_message(friend_info->second.chat_input);
                    msg.set_allocated_steam_messages(steam_messages);
                    msg.set_source_id(settings->get_local_steam_id().ConvertToUint64());
                    msg.set_dest_id(friend_id);
                    network->sendTo(&msg, true);

                    friend_info->second.chat_history.append(get_steam_client()->settings_client->get_local_name()).append(": ").append(input).append("\n", 1);
                }
                *input = 0; // Reset the input field
                friend_info->second.window_state &= ~window_state_send_message;
            }
            // The user clicked on "Invite"
            if (friend_info->second.window_state & window_state_invite)
            {
                std::string connect = steamFriends->GetFriendRichPresence(settings->get_local_steam_id(), "connect");
                if (connect.length() > 0)
                    steamFriends->InviteUserToGame(friend_id, connect.c_str());
                else if (settings->get_lobby().IsValid())
                    steamMatchmaking->InviteUserToLobby(settings->get_lobby(), friend_id);

                friend_info->second.window_state &= ~window_state_invite;
            }
            // The user clicked on "Join"
            if (friend_info->second.window_state & window_state_join)
            {
                std::string connect = steamFriends->GetFriendRichPresence(friend_id, "connect");
                // The user got a lobby invite and accepted it
                if (friend_info->second.window_state & window_state_lobby_invite)
                {
                    GameLobbyJoinRequested_t data;
                    data.m_steamIDLobby.SetFromUint64(friend_info->second.lobbyId);
                    data.m_steamIDFriend.SetFromUint64(friend_id);
                    callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));

                    friend_info->second.window_state &= ~window_state_lobby_invite;
                } else {
                // The user got a rich presence invite and accepted it
                    if (friend_info->second.window_state & window_state_rich_invite)
                    {
                        GameRichPresenceJoinRequested_t data = {};
                        data.m_steamIDFriend.SetFromUint64(friend_id);
                        strncpy(data.m_rgchConnect, friend_info->second.connect, k_cchMaxRichPresenceValueLength - 1);
                        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));

                        friend_info->second.window_state &= ~window_state_rich_invite;
                    } else if (connect.length() > 0)
                    {
                        GameRichPresenceJoinRequested_t data = {};
                        data.m_steamIDFriend.SetFromUint64(friend_id);
                        strncpy(data.m_rgchConnect, connect.c_str(), k_cchMaxRichPresenceValueLength - 1);
                        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));
                    }

                    //Not sure about this but it fixes sonic racing transformed invites
                    FriendGameInfo_t friend_game_info = {};
                    steamFriends->GetFriendGamePlayed(friend_id, &friend_game_info);
                    uint64 lobby_id = friend_game_info.m_steamIDLobby.ConvertToUint64();
                    if (lobby_id) {
                        GameLobbyJoinRequested_t data;
                        data.m_steamIDLobby.SetFromUint64(lobby_id);
                        data.m_steamIDFriend.SetFromUint64(friend_id);
                        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));
                    }
                }

                friend_info->second.window_state &= ~window_state_join;
            }
        }
        has_friend_action.pop();
    }
}

bool Steam_Overlay::RegisteredInternalCallbacks()
{
    return (overlay_CCallback != NULL &&
            overlay_CCallback->is_ready() == true);
}

void Steam_Overlay::OnAvatarImageLoaded(AvatarImageLoaded_t *pParam)
{
    if (pParam != NULL) {
        if (pParam->m_steamID != k_steamIDNil) {
            PRINT_DEBUG("%s %" PRIu64 ". %s %d.\n",
                        "Steam_Overlay::OnAvatarImageLoaded Destroy avatar images for",
                        pParam->m_steamID.ConvertToUint64(),
                        "New reference",
                        pParam->m_iImage);
            DestroyProfileImageResource(pParam->m_steamID, k_EAvatarSize32x32);
            DestroyProfileImage(pParam->m_steamID, k_EAvatarSize32x32);
            DestroyProfileImageResource(pParam->m_steamID, k_EAvatarSize64x64);
            DestroyProfileImage(pParam->m_steamID, k_EAvatarSize64x64);
            DestroyProfileImageResource(pParam->m_steamID, k_EAvatarSize184x184);
            DestroyProfileImage(pParam->m_steamID, k_EAvatarSize184x184);
        }
    }
    return;
}

void Steam_Overlay_CCallback::OnAvatarImageLoaded(AvatarImageLoaded_t * pParam)
{
    auto client = try_get_steam_client();
    if (client != NULL && client->steam_overlay != NULL) {
        PRINT_DEBUG("%s 0x%p sent to overlay 0x%p.\n",
                    "Steam_Overlay_CCallback::OnAvatarImageLoaded",
                    pParam,
                    client->steam_overlay);
        client->steam_overlay->OnAvatarImageLoaded(pParam);
    }
    return;
}

#endif
