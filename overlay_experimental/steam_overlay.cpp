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

#define URL_WINDOW_NAME "URL Window"

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

#ifdef __WINDOWS__
#include "windows/Windows_Hook.h"
#endif

#include "notification.h"

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

            if (ach.raw_image != NULL) {
                delete ach.raw_image;
                ach.raw_image = NULL;
                ach.raw_image_width = 0;
                ach.raw_image_height = 0;
            }
            DestroyAchievementImageResource(ach);

            uint8 * raw_image = new uint8[(width * height * sizeof(uint32))];
            if ((raw_image != NULL) &&
                (steamUtils->GetImageRGBA(image_handle,
                                          raw_image,
                                          (width * height * sizeof(uint32))) == true)) {
                                              PRINT_DEBUG("LoadAchievementImage() %d -> %p.\n", image_handle, raw_image);
                ach.raw_image = raw_image;
                ach.raw_image_width = width;
                ach.raw_image_height = height;
            } else {
                delete ach.raw_image;
                PRINT_DEBUG("Image for achievement %s could not get pixel data.\n", ach.name.c_str());
            }
        } else {
            PRINT_DEBUG("Image for achievement %s has an invalid size.\n", ach.name.c_str());
        }
    } else {
        PRINT_DEBUG("Image for achievement %s is not loaded.\n", ach.name.c_str());
    }
}

void Steam_Overlay::CreateAchievementImageResource(Overlay_Achievement & ach)
{
    PRINT_DEBUG("CreateAchievementImageResource() %s. %d x %d -> %p\n", ach.name.c_str(), ach.raw_image_width, ach.raw_image_height, ach.raw_image);

    if (_renderer) {
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

    if (_renderer && ach.image_resource.expired() == false) {
        _renderer->ReleaseImageResource(ach.image_resource);
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
    notif_position(ENotificationPosition::k_EPositionBottomLeft),
    h_inset(0),
    v_inset(0),
    overlay_state_changed(false),
    i_have_lobby(false),
    show_achievements(false),
    show_settings(false),
    _renderer(nullptr),
    fonts_atlas(nullptr),
    earned_achievement_count(0)
{
    strncpy(username_text, settings->get_local_name(), sizeof(username_text));

    show_achievement_desc_on_unlock = settings->get_show_achievement_desc_on_unlock();
    show_achievement_hidden_unearned = settings->get_show_achievement_hidden_unearned();

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

    current_language = 0;
    const char *language = settings->get_language();

    int i = 0;
    for (auto l : valid_languages) {
        if (strcmp(l, language) == 0) {
            current_language = i;
            break;
        }

        ++i;
    }

    run_every_runcb->add(&Steam_Overlay::steam_overlay_run_every_runcb, this);
    this->network->setCallback(CALLBACK_ID_STEAM_MESSAGES, settings->get_local_steam_id(), &Steam_Overlay::steam_overlay_callback, this);
}

Steam_Overlay::~Steam_Overlay()
{
    run_every_runcb->remove(&Steam_Overlay::steam_overlay_run_every_runcb, this);
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
    notif_position = eNotificationPosition;
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
        if (future_renderer.wait_for(std::chrono::milliseconds{500}) ==  std::future_status::ready) {
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

void Steam_Overlay::AddMessageNotification(std::string const& message)
{
    std::lock_guard<std::recursive_mutex> lock(notifications_mutex);
    int id = find_free_notification_id(notifications);
    if (id != 0)
    {
        Notification notif;
        notif.id = id;
        notif.type = notification_type_message;
        notif.message = message;
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
        ImGuiStyle &style = ImGui::GetStyle();
        wnd_width -= ImGui::CalcTextSize("Send").x + style.FramePadding.x * 2 + style.ItemSpacing.x + 1;

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

            ImGui::SetNextWindowPos(ImVec2((float)width - width * Notification::width, Notification::height * font_size * i ));
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
            ImVec2 text_offset(image_offset.x, image_offset.y);
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
                                ImVec2 image_scale(image_max_resolution.x * 0.4f,
                                                    image_max_resolution.y * 0.4f);

                                // Fix text offset.
                                text_offset.x = image_offset.x + currentStyle.ItemSpacing.x + image_scale.x;

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
                    ImGui::SetCursorPos(text_offset);
                    ImGui::TextWrapped("%s", it->message.c_str()); break;
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
                OUTLINETEXTMETRIC * metric = (OUTLINETEXTMETRIC*)calloc(metsize, 1);
                if (metric != NULL) {
                    if (GetOutlineTextMetrics(CBSTR.hDevice, metsize, metric) != 0) {
                        if ((metric->otmfsType & 0x1) == 0) {
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
                            PRINT_DEBUG("%s %s.\n", "Licensing failure. Cannot use font", lf->lfFaceName);
                        }
                    }

                    free(metric);
                    metric = NULL;
                }
            }
            SelectObject(CBSTR.hDevice, oldFont);
            DeleteObject(hFont);
        }
    }
    return ret;
}

int CALLBACK cb_enumfonts(const LOGFONT *lf, const TEXTMETRIC *tm, DWORD fontType, LPARAM UNUSED)
{
    int ret = 1; // Continue iteration.
    cb_font_str * cbStr = &CBSTR;
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
        memset(&CBSTR, '\0', sizeof(cb_font_str));
        CBSTR.font_size = font_size;
        CBSTR.fontcfg = fontcfg;
        CBSTR.atlas = Fonts;
        CBSTR.hDevice = CreateCompatibleDC(oDC);
        HGDIOBJ hOldBitmap = SelectObject(CBSTR.hDevice, hBitmap);
        DeleteObject(hOldBitmap);

        PRINT_DEBUG("%s\n", "Atempting to load preferred ANSI font from Win32 API.");
        EnumFontFamiliesExA(CBSTR.hDevice, &lf, cb_enumfonts, 0, 0);
        if ((CBSTR.foundBits & 0x1) != 0x1) {
            CBSTR.foundBits = CBSTR.foundBits | 0x80;
            PRINT_DEBUG("%s\n", "Atempting to load generic ANSI font from Win32 API.");
            EnumFontFamiliesExA(CBSTR.hDevice, &lf, cb_enumfonts, 0, 0);
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
        if ((CBSTR.foundBits & 0xE) != 0xE) {
            PRINT_DEBUG("%s\n", "Atempting to load preferred CJK fonts from Win32 API.");
            EnumFontFamiliesExA(CBSTR.hDevice, &lf, cb_enumfonts, 0, 0);
        }
        CBSTR.foundBits = CBSTR.foundBits | 0x80;
        if ((CBSTR.foundBits & 0xF) != 0xF) {
            PRINT_DEBUG("%s\n", "Loading generic CJK fonts from Win32 API.");
            EnumFontFamiliesExA(CBSTR.hDevice, &lf, cb_enumfonts, 0, 0);
        }
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

void Steam_Overlay::DestroyAchievementImageResources()
{
    for (auto & x : achievements) {
        if (x.image_resource.expired()) {
            DestroyAchievementImageResource(x);
        }
    }

    return;
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
            ImGui::LabelText("##label", "Username: %s(%llu) playing %u",
                settings->get_local_name(),
                settings->get_local_steam_id().ConvertToUint64(),
                settings->get_local_game_id().AppID());
            ImGui::SameLine();

            ImGui::LabelText("##label", "Renderer: %s", (_renderer == nullptr ? "Unknown" : _renderer->GetLibraryName().c_str()));

            ImGui::Text("Achievements earned: %d / %d", earned_achievement_count, total_achievement_count);
            ImGui::SameLine();
            ImGui::ProgressBar((earned_achievement_count / total_achievement_count), ImVec2((io.DisplaySize.x * 0.20f),0));

            ImGui::Spacing();
            if (ImGui::Button("Show Achievements")) {
                show_achievements = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Settings")) {
                show_settings = true;
            }

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::LabelText("##label", "Friends");

            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
            if (!friends.empty())
            {
                if (ImGui::ListBoxHeader("##label", friends.size()))
                {
                    std::for_each(friends.begin(), friends.end(), [this](std::pair<Friend const, friend_window_state> &i)
                    {
                        ImGui::PushID(i.second.id-base_friend_window_id+base_friend_item_id);

                        ImGui::Selectable(i.second.window_title.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);
                        BuildContextMenu(i.first, i.second);
                        if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                        {
                            i.second.window_state |= window_state_show;
                        }
                        ImGui::PopID();

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
                    ImGuiStyle currentStyle = ImGui::GetStyle();
                    float window_x_offset = (ImGui::GetWindowWidth() > currentStyle.ScrollbarSize) ? (ImGui::GetWindowWidth() - currentStyle.ScrollbarSize) : 0;
                    for (auto & x : achievements) {
                        bool achieved = x.achieved;
                        bool hidden = x.hidden && !achieved;

                        if (!hidden || show_achievement_hidden_unearned) {

                            if (x.raw_image == NULL) {
                                LoadAchievementImage(x);
                            }

                            if (x.image_resource.expired() == true) {
                                CreateAchievementImageResource(x);
                            }

                            ImGui::Separator();

                            // Anchor the image to the right side of the list.
                            ImVec2 target = ImGui::GetCursorPos();
                            target.x = window_x_offset;
                            if (target.x > (x.raw_image_width * 0.4f)) {
                                target.x = target.x - (x.raw_image_width * 0.4f);
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

                            if ((x.image_resource.expired() == false) &&
                                (x.raw_image_width > 0) &&
                                (x.raw_image_height > 0)) {
                                std::shared_ptr<uint64_t> s_ptr = x.image_resource.lock();
                                ImGui::Image((ImTextureID)(intptr_t)*s_ptr, ImVec2(x.raw_image_width * 0.4f, x.raw_image_height * 0.4f));
                            }

                            ImGui::Separator();
                        }
                    }
                    ImGui::EndChild();
                }
                ImGui::End();
                show_achievements = show;
            }

            if (show_settings) {
                if (ImGui::Begin("Global Settings Window", &show_settings)) {
                    ImGui::Text("These are global emulator settings and will apply to all games.");

                    ImGui::Separator();

                    ImGui::Text("Username:");
                    ImGui::SameLine();
                    ImGui::InputText("##username", username_text, sizeof(username_text), disable_forced ? ImGuiInputTextFlags_ReadOnly : 0);

                    ImGui::Separator();

                    ImGui::Text("Language:");

                    if (ImGui::ListBox("##language", &current_language, valid_languages, sizeof(valid_languages) / sizeof(char *), 7)) {

                    }

                    ImGui::Text("Selected Language: %s", valid_languages[current_language]);

                    ImGui::Separator();

                    ImGui::Checkbox("Show achievement descriptions on unlock", &show_achievement_desc_on_unlock);
                    ImGui::Checkbox("Show unearned hidden achievements", &show_achievement_hidden_unearned);

                    ImGui::Separator();

                    if (!disable_forced) {
                        ImGui::Text("You may have to restart the game for these to apply.");
                        if (ImGui::Button("Save")) {
                            save_settings = true;
                            show_settings = false;
                        }
                    } else {
                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                        ImGui::TextWrapped("Some steam_settings/force_*.txt files have been detected. Please delete them if you want this menu to work.");
                        ImGui::TextColored(ImVec4(255, 0, 0, 255), "WARNING WARNING WARNING");
                    }
                }

                ImGui::End();
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

            AddMessageNotification(friend_info->first.name() + " says: " + steam_message.message());
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

    if (overlay_state_changed)
    {
        GameOverlayActivated_t data = { 0 };
        data.m_bActive = show_overlay;
        data.m_bUserInitiated = true;
        data.m_nAppID = settings->get_local_game_id().AppID();
        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));
        DestroyAchievementImageResources(); // Don't recreate them here. OpenGL won't display them if we do.
        overlay_state_changed = false;
    }

    Steam_Friends* steamFriends = get_steam_client()->steam_friends;
    Steam_Matchmaking* steamMatchmaking = get_steam_client()->steam_matchmaking;

    if (save_settings) {
        char *language_text = valid_languages[current_language];
        get_steam_client()->settings_client->set_local_name(username_text);
        get_steam_client()->settings_client->set_language(language_text);
        get_steam_client()->settings_client->set_show_achievement_desc_on_unlock(show_achievement_desc_on_unlock);
        get_steam_client()->settings_client->set_show_achievement_hidden_unearned(show_achievement_hidden_unearned);
        get_steam_client()->settings_server->set_local_name(username_text);
        get_steam_client()->settings_server->set_language(language_text);
        get_steam_client()->settings_server->set_show_achievement_desc_on_unlock(show_achievement_desc_on_unlock);
        get_steam_client()->settings_server->set_show_achievement_hidden_unearned(show_achievement_hidden_unearned);
        save_global_settings(get_steam_client()->local_storage, get_steam_client()->settings_client);
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

#endif
