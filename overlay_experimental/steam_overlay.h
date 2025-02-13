#ifndef __INCLUDED_STEAM_OVERLAY_H__
#define __INCLUDED_STEAM_OVERLAY_H__

#include "../dll/base.h"
#include <map>
#include <queue>

static constexpr size_t max_chat_len = 768;

enum window_state
{
    window_state_none           = 0,
    window_state_show           = 1<<0,
    window_state_invite         = 1<<1,
    window_state_join           = 1<<2,
    window_state_lobby_invite   = 1<<3,
    window_state_rich_invite    = 1<<4,
    window_state_send_message   = 1<<5,
    window_state_need_attention = 1<<6,
};

struct friend_window_state
{
    int id;
    uint8 window_state;
    std::string window_title;
    union // The invitation (if any)
    {
        uint64 lobbyId;
        char connect[k_cchMaxRichPresenceValueLength];
    };
    std::string chat_history;
    char chat_input[max_chat_len];

    bool joinable;
};

struct Friend_Less
{
    bool operator()(const Friend& lhs, const Friend& rhs) const
    {
        return lhs.id() < rhs.id();
    }
};

enum notification_type
{
    notification_type_message = 0,
    notification_type_invite,
    notification_type_achievement,
};

struct Notification
{
    static constexpr float width = 0.25;
    static constexpr float height = 5.0;
    static constexpr std::chrono::milliseconds fade_in   = std::chrono::milliseconds(2000);
    static constexpr std::chrono::milliseconds fade_out  = std::chrono::milliseconds(2000);
    static constexpr std::chrono::milliseconds show_time = std::chrono::milliseconds(6000) + fade_in + fade_out;
    static constexpr std::chrono::milliseconds fade_out_start = show_time - fade_out;
    static constexpr float r = 0.16;
    static constexpr float g = 0.29;
    static constexpr float b = 0.48;
    static constexpr float max_alpha = 0.5f;

    int id;
    uint8 type;
    std::chrono::seconds start_time;
    std::string message;
    std::pair<const Friend, friend_window_state>* frd;
    std::string ach_name;
    CSteamID steam_id;
};

struct Overlay_Achievement
{
    std::string name;
    std::string title;
    std::string description;
    bool hidden;
    bool achieved;
    uint32 unlock_time;
    uint8 * raw_image;
    uint32 raw_image_width;
    uint32 raw_image_height;
    std::weak_ptr<uint64_t> image_resource;
};

struct Profile_Image_ID
{
    CSteamID id;
    int eAvatarSize;
};

bool operator<(const Profile_Image_ID &a, const Profile_Image_ID &b);

bool operator>(const Profile_Image_ID &a, const Profile_Image_ID &b);

bool operator<=(const Profile_Image_ID &a, const Profile_Image_ID &b);

bool operator>=(const Profile_Image_ID &a, const Profile_Image_ID &b);

bool operator==(const Profile_Image_ID &a, const Profile_Image_ID &b);

bool operator!=(const Profile_Image_ID &a, const Profile_Image_ID &b);

struct Profile_Image
{
    uint32 width;
    uint32 height;
    uint8 * raw_image;
    std::weak_ptr<uint64_t> image_resource;
};

#ifdef small
#undef small
#endif

struct Profile_Image_Set
{
    Profile_Image small;
    Profile_Image medium;
    Profile_Image large;
};

struct Temporary_Image
{
    Profile_Image image_data;
    std::chrono::steady_clock::time_point last_display_time;
};

enum DisplayImageType {
  displayImageTypeAchievement = 0,
  displayImageTypeAvatar = 1,
  displayImageTypeCustom = 2,
  displayImageTypeEND = 3
};

#ifdef EMU_OVERLAY
#include <future>
#include "Renderer_Hook.h"
class Steam_Overlay_CCallback;

class Steam_Overlay
{
    Settings* settings;
    SteamCallResults* callback_results;
    SteamCallBacks* callbacks;
    RunEveryRunCB* run_every_runcb;
    Networking* network;
    Steam_Overlay_CCallback* overlay_CCallback;

    // friend id, show client window (to chat and accept invite maybe)
    std::map<Friend, friend_window_state, Friend_Less> friends;

    bool setup_overlay_called;
    bool is_ready;
    bool show_overlay;
    ENotificationPosition notif_position;
    int h_inset, v_inset;
    std::string show_url;
    std::vector<Overlay_Achievement> achievements;
    bool show_achievements, show_settings, show_profile_image_select;
    void *fonts_atlas;

    bool disable_forced, local_save, warning_forced, show_achievement_desc_on_unlock, show_achievement_hidden_unearned;
    uint32_t appid, total_achievement_count, earned_achievement_count;

    char username_text[256];
    std::atomic_bool save_settings, save_small_profile_image, save_medium_profile_image, save_large_profile_image;

    // Set Avatar Image filesystem chooser
    bool show_drive_list;
    std::map<std::string, bool> filesystem_list;
    std::string current_path;
    std::string future_path;

    // Avatar Images
    std::map<CSteamID, Profile_Image_Set> profile_images;
    std::map<CSteamID, uint32_t>lazy_load_avatar_images;

    std::string new_profile_image_path;
    std::string current_status_message;
    bool tried_load_new_profile_image, cleared_new_profile_images_struct;
    Profile_Image_Set new_profile_image_handles;
    bool radio_btn_new_profile_image_size[3];

    int current_language;
    int current_ui_notification_position_selection;

    std::string warning_message;

    // Callback infos
    std::queue<Friend> has_friend_action;
    std::vector<Notification> notifications;
    std::recursive_mutex notifications_mutex;
    std::atomic<bool> have_notifications;

    bool overlay_state_changed;

    std::recursive_mutex overlay_mutex;
    std::atomic<bool> i_have_lobby;
    std::future<ingame_overlay::Renderer_Hook*> future_renderer;
    ingame_overlay::Renderer_Hook* _renderer;

    Steam_Overlay(Steam_Overlay const&) = delete;
    Steam_Overlay(Steam_Overlay&&) = delete;
    Steam_Overlay& operator=(Steam_Overlay const&) = delete;
    Steam_Overlay& operator=(Steam_Overlay&&) = delete;

    static void steam_overlay_run_every_runcb(void* object);
    static void steam_overlay_callback(void* object, Common_Message* msg);

    void Callback(Common_Message* msg);
    void RunCallbacks();

    bool FriendJoinable(std::pair<const Friend, friend_window_state> &f);
    bool IHaveLobby();

    void NotifyUser(friend_window_state& friend_state);

    // Right click on friend
    void BuildContextMenu(Friend const& frd, friend_window_state &state);
    // Double click on friend
    void BuildFriendWindow(Friend const& frd, friend_window_state &state);
    // Notifications like achievements, chat and invitations
    void BuildNotifications(int width, int height);

    // ImGui image.
    std::map<uint8*,Temporary_Image> temp_display_images;

    int display_imgui_achievement(float xSize,
                                  float ySize,
                                  float image_color_multipler_r,
                                  float image_color_multipler_g,
                                  float image_color_multipler_b,
                                  float image_color_multipler_a,
                                  std::string achName,
                                  uint32_t loadType);

    int display_imgui_avatar(float xSize,
                             float ySize,
                             float image_color_multipler_r,
                             float image_color_multipler_g,
                             float image_color_multipler_b,
                             float image_color_multipler_a,
                             CSteamID userID,
                             int eAvatarSize,
                             uint32_t loadType);

    int display_imgui_custom_image(float xSize,
                                   float ySize,
                                   float image_color_multipler_r,
                                   float image_color_multipler_g,
                                   float image_color_multipler_b,
                                   float image_color_multipler_a,
                                   uint8 * imageData,
                                   uint32_t imageDataLength,
                                   uint32_t imageDataWidth,
                                   uint32_t imageDataHeight);

    int display_imgui_image(uint32_t displayImageType,
                            float xSize,
                            float ySize,
                            float image_color_multipler_r,
                            float image_color_multipler_g,
                            float image_color_multipler_b,
                            float image_color_multipler_a,
                            std::string achName,
                            CSteamID userID,
                            int eAvatarSize,
                            uint8 * imageData,
                            uint32_t imageDataLength,
                            uint32_t imageDataWidth,
                            uint32_t imageDataHeight,
                            uint32_t loadType);

    void LoadAchievementImage(Overlay_Achievement & ach);
    void CreateAchievementImageResource(Overlay_Achievement & ach);
    void DestroyAchievementImageResource(Overlay_Achievement & ach);
    void DestroyAchievementImageResources();

    // Profile images
    void populate_initial_profile_images(CSteamID id);
    bool LoadProfileImage(const CSteamID & id, const int eAvatarSize);
    bool LoadProfileImage(const CSteamID & id, const int eAvatarSize, Profile_Image_Set & images);
    bool CreateProfileImageResource(const CSteamID & id, const int eAvatarSize);
    bool CreateProfileImageResource(const CSteamID & id, const int eAvatarSize, Profile_Image_Set & images);
    void DestroyProfileImage(const CSteamID & id, const int eAvatarSize);
    void DestroyProfileImage(const CSteamID & id, const int eAvatarSize, Profile_Image_Set & images);
    void DestroyProfileImageResource(const CSteamID & id, const int eAvatarSize);
    void DestroyProfileImageResource(const CSteamID & id, const int eAvatarSize, Profile_Image_Set & images);
    void DestroyProfileImageResources();

    // Temporary images
    void ReturnTemporaryImage(Profile_Image & imageData);
    Profile_Image GetTemporaryImage(uint8 * imageData);
    void DestroyTemporaryImageResource(uint8 * imageData);
    void DestroyTemporaryImageResources();
    void DestroyTemporaryImage(uint8 * imageData);
    void DestroyTemporaryImages();
    void PruneTemporaryImages();

public:
    Steam_Overlay(Settings* settings, SteamCallResults* callback_results, SteamCallBacks* callbacks, RunEveryRunCB* run_every_runcb, Networking *network);

    ~Steam_Overlay();

    bool Ready() const;

    bool NeedPresent() const;

    void SetNotificationPosition(ENotificationPosition eNotificationPosition);

    void SetNotificationInset(int nHorizontalInset, int nVerticalInset);
    void SetupOverlay();
    void UnSetupOverlay();
    bool RegisteredInternalCallbacks();

    void HookReady(bool ready);

    void CreateFonts();
    void OverlayProc();

    void OpenOverlayInvite(CSteamID lobbyId);
    void OpenOverlay(const char* pchDialog);
    void OpenOverlayWebpage(const char* pchURL);

    bool ShowOverlay() const;
    void ShowOverlay(bool state);
    bool OpenOverlayHook(bool toggle);

    void SetLobbyInvite(Friend friendId, uint64 lobbyId);
    void SetRichInvite(Friend friendId, const char* connect_str);

    void FriendConnect(Friend _friend);
    void FriendDisconnect(Friend _friend);

    void AddMessageNotification(std::string const& message, CSteamID frd);
    void AddAchievementNotification(nlohmann::json const& ach);
    void AddInviteNotification(std::pair<const Friend, friend_window_state> &wnd_state);
    void OnAvatarImageLoaded(AvatarImageLoaded_t *pParam);
};

#else

class Steam_Overlay
{
public:
    Steam_Overlay(Settings* settings, SteamCallResults* callback_results, SteamCallBacks* callbacks, RunEveryRunCB* run_every_runcb, Networking* network) {}
    ~Steam_Overlay() {}

    bool Ready() const { return false; }

    bool NeedPresent() const { return false; }

    void SetNotificationPosition(ENotificationPosition eNotificationPosition) {}

    void SetNotificationInset(int nHorizontalInset, int nVerticalInset) {}
    void SetupOverlay() {}
    void UnSetupOverlay() {}
    bool RegisteredInternalCallbacks() const { return true; }

    void HookReady(bool ready) {}

    void CreateFonts() {}
    void OverlayProc() {}

    void OpenOverlayInvite(CSteamID lobbyId) {}
    void OpenOverlay(const char* pchDialog) {}
    void OpenOverlayWebpage(const char* pchURL) {}

    bool ShowOverlay() const {}
    void ShowOverlay(bool state) {}
    bool OpenOverlayHook(bool toggle) {}

    void SetLobbyInvite(Friend friendId, uint64 lobbyId) {}
    void SetRichInvite(Friend friendId, const char* connect_str) {}

    void FriendConnect(Friend _friend) {}
    void FriendDisconnect(Friend _friend) {}

    void AddMessageNotification(std::string const& message, CSteamID frd) {}
    void AddAchievementNotification(nlohmann::json const& ach) {}
    void AddInviteNotification(std::pair<const Friend, friend_window_state> &wnd_state) {}
    void OnAvatarImageLoaded(AvatarImageLoaded_t *pParam) {}
};

#endif

#endif//__INCLUDED_STEAM_OVERLAY_H__
