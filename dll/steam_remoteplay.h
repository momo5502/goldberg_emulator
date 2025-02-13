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

#include "base.h"

#ifndef INCLUDED_STEAM_REMOTEPLAY_H
#define INCLUDED_STEAM_REMOTEPLAY_H

class Steam_RemotePlay :
public ISteamRemotePlay
{
    class Settings *settings;
    class Networking *network;
    class SteamCallResults *callback_results;
    class SteamCallBacks *callbacks;
    class RunEveryRunCB *run_every_runcb;

public:
static void steam_callback(void *object, Common_Message *msg)
{
    PRINT_DEBUG("steam_remoteplay_callback\n");

    Steam_RemotePlay *steam_remoteplay = (Steam_RemotePlay *)object;
    steam_remoteplay->Callback(msg);
}

static void steam_run_every_runcb(void *object)
{
    PRINT_DEBUG("steam_remoteplay_run_every_runcb\n");

    Steam_RemotePlay *steam_remoteplay = (Steam_RemotePlay *)object;
    steam_remoteplay->RunCallbacks();
}

Steam_RemotePlay(class Settings *settings, class Networking *network, class SteamCallResults *callback_results, class SteamCallBacks *callbacks, class RunEveryRunCB *run_every_runcb)
{
    this->settings = settings;
    this->network = network;
    this->run_every_runcb = run_every_runcb;
    //this->network->setCallback(CALLBACK_ID_USER_STATUS, settings->get_local_steam_id(), &Steam_RemotePlay::steam_callback, this);
    this->run_every_runcb->add(&Steam_RemotePlay::steam_run_every_runcb, this);

    this->callback_results = callback_results;
    this->callbacks = callbacks;
}

~Steam_RemotePlay()
{
    //TODO rm network callbacks
    this->run_every_runcb->remove(&Steam_RemotePlay::steam_run_every_runcb, this);
}

// Get the number of currently connected Steam Remote Play sessions
uint32 GetSessionCount();

// Get the currently connected Steam Remote Play session ID at the specified index. Returns zero if index is out of bounds.
uint32 GetSessionID( int iSessionIndex );

// Get the SteamID of the connected user
CSteamID GetSessionSteamID( uint32 unSessionID );

// Get the name of the session client device
// This returns NULL if the sessionID is not valid
const char *GetSessionClientName( uint32 unSessionID );

// Get the form factor of the session client device
ESteamDeviceFormFactor GetSessionClientFormFactor( uint32 unSessionID );

// Get the resolution, in pixels, of the session client device
// This is set to 0x0 if the resolution is not available
bool BGetSessionClientResolution( uint32 unSessionID, int *pnResolutionX, int *pnResolutionY );

// Invite a friend to Remote Play Together
// This returns false if the invite can't be sent
bool BSendRemotePlayTogetherInvite( CSteamID steamIDFriend )
{
    PRINT_DEBUG("Steam_RemotePlay::BSendRemotePlayTogetherInvite\n");
    return false;
}

void RunCallbacks()
{
}

void Callback(Common_Message *msg)
{
    if (msg->has_low_level()) {
        if (msg->low_level().type() == Low_Level::CONNECT) {
            
        }

        if (msg->low_level().type() == Low_Level::DISCONNECT) {

        }
    }

    if (msg->has_networking_sockets()) {

    }
}

};

#endif // INCLUDED_STEAM_REMOTEPLAY_H
