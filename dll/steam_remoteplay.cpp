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

#include "steam_remoteplay.h"
#include <string.h>
#include <limits.h>

typedef struct remote_play_session_info_T {
    RemotePlaySessionID_t session_id;
    CSteamID connected_user;
    const char * client_name;
    ESteamDeviceFormFactor client_form_factor;
    int client_resolution_x;
    int client_resolution_y;
} remote_play_session_info;
//TODO: NOT thread safe!!!
static std::vector<remote_play_session_info> remote_play_sessions;

int create_remote_play_session_info( RemotePlaySessionID_t session_id, CSteamID connected_user, const char * client_name, ESteamDeviceFormFactor client_form_factor, int client_resolution_x, int client_resolution_y ) {
    remote_play_session_info session_info;
    size_t buffer_length = 0;
    char * buffer = NULL;

    if ((remote_play_sessions.size() < UINT_MAX) && (client_name != NULL)) {
        session_info.session_id = session_id;
        session_info.connected_user = connected_user;
        session_info.client_form_factor = client_form_factor;
        session_info.client_resolution_x = client_resolution_x;
        session_info.client_resolution_y = client_resolution_y;

        buffer_length = strlen( client_name );
        if (buffer_length > 0) {
            buffer = new char[buffer_length + 1];
            if (buffer != NULL) {
                memcpy(buffer, client_name, buffer_length);
                session_info.client_name = buffer;
                remote_play_sessions.push_back( (const remote_play_session_info)session_info );
                return 0;
            }
        }
    }
    return -1;
}

int destroy_remote_play_session_info( size_t index ) {
    if (remote_play_sessions.size() < index) {
        delete remote_play_sessions[index].client_name;
        remote_play_sessions.erase(remote_play_sessions.begin() + index);
        return 0;
    }
    return -1;
}

uint32 get_number_of_remote_play_sessions() {
    return (uint32)remote_play_sessions.size();
}

int get_remote_play_session_id( size_t index, RemotePlaySessionID_t * session_id ) {
    if ((session_id != NULL) && (index >= 0) && (remote_play_sessions.size() < index)) {
        *session_id = remote_play_sessions[index].session_id;
        return 0;
    }
    return -1;
}

int get_remote_play_session_index( RemotePlaySessionID_t session_id, size_t * index ) {
    size_t count = 0;

    if ((index != NULL) && (remote_play_sessions.size() > 0)) {
        for (std::vector<remote_play_session_info>::iterator iter = remote_play_sessions.begin(); iter != remote_play_sessions.end(); iter++) {
            if (iter->session_id == session_id) {
                *index = count;
                return 0;
            }
            count++;
        }
    }
    return -1;
}

int get_remote_play_session_connected_user( size_t index, CSteamID * connected_user ) {
    if ((connected_user != NULL) && (index >= 0) && (remote_play_sessions.size() < index)) {
        *connected_user = remote_play_sessions[index].connected_user;
        return 0;
    }
    return -1;
}

int get_remote_play_session_client_name( size_t index, const char ** client_name ) {
    if ((client_name != NULL) && (index >= 0) && (remote_play_sessions.size() < index)) {
        *client_name = remote_play_sessions[index].client_name;
        return 0;
    }
    return -1;
}

int get_remote_play_session_client_form_factor( size_t index, ESteamDeviceFormFactor * client_form_factor ) {
    if ((client_form_factor != NULL) && (index >= 0) && (remote_play_sessions.size() < index)) {
        *client_form_factor = remote_play_sessions[index].client_form_factor;
        return 0;
    }
    return -1;
}

int get_remote_play_session_client_resolutions( size_t index, int * client_resolution_x, int * client_resolution_y ) {
    if ((client_resolution_x != NULL) && (client_resolution_y != NULL) && (index >= 0) && (remote_play_sessions.size() < index)) {
        *client_resolution_x = remote_play_sessions[index].client_resolution_x;
        *client_resolution_y = remote_play_sessions[index].client_resolution_y;
        return 0;
    }
    return -1;
}

uint32 Steam_RemotePlay::GetSessionCount()
{
    PRINT_DEBUG("Steam_RemotePlay::GetSessionCount\n");
    return get_number_of_remote_play_sessions();
}

uint32 Steam_RemotePlay::GetSessionID( int iSessionIndex )
{
    RemotePlaySessionID_t session_id;

    PRINT_DEBUG("Steam_RemotePlay::GetSessionID\n");
    return ((get_remote_play_session_id( iSessionIndex, &session_id ) == 0) ? (session_id) : (0));
}

CSteamID Steam_RemotePlay::GetSessionSteamID( uint32 unSessionID )
{
    CSteamID steam_id = k_steamIDNil; 
    size_t index = 0;

    PRINT_DEBUG("Steam_RemotePlay::GetSessionSteamID\n");
    if (get_remote_play_session_index( unSessionID, &index ) == 0) {
        if (get_remote_play_session_connected_user( index, &steam_id ) == 0) {
            return steam_id;
        }
    }
    return k_steamIDNil;
}

const char * Steam_RemotePlay::GetSessionClientName( uint32 unSessionID )
{
    const char * client_name = NULL;
    size_t index = 0;

    PRINT_DEBUG("Steam_RemotePlay::GetSessionClientName\n");
    if (get_remote_play_session_index( unSessionID, &index ) == 0) {
        if (get_remote_play_session_client_name( index, &client_name ) == 0) {
            return client_name;
        }
    }
    return NULL;
}

ESteamDeviceFormFactor Steam_RemotePlay::GetSessionClientFormFactor( uint32 unSessionID )
{
    ESteamDeviceFormFactor form_factor = k_ESteamDeviceFormFactorUnknown;
    size_t index = 0;

    PRINT_DEBUG("Steam_RemotePlay::GetSessionClientFormFactor\n");
    if (get_remote_play_session_index( unSessionID, &index ) == 0) {
        if (get_remote_play_session_client_form_factor( index, &form_factor ) == 0) {
            return form_factor;
        }
    }
    return k_ESteamDeviceFormFactorUnknown;
}

bool Steam_RemotePlay::BGetSessionClientResolution( uint32 unSessionID, int *pnResolutionX, int *pnResolutionY )
{
    int x = 0;
    int y = 0;
    size_t index = 0;

    PRINT_DEBUG("Steam_RemotePlay::BGetSessionClientResolution\n");
    if ((pnResolutionX != NULL) && (pnResolutionY != NULL)) {
        if (get_remote_play_session_index( unSessionID, &index ) == 0) {
            if (get_remote_play_session_client_resolutions( index, &x, &y ) == 0) {
                *pnResolutionX = x;
                *pnResolutionY = y;
                return true;
            }
        }
    }
    return false;
}

