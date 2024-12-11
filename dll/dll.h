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

#include "steam_client.h"

#ifdef STEAMCLIENT_DLL
#define STEAMAPI_API static
#define STEAMCLIENT_API S_API_EXPORT
#else
#define STEAMAPI_API S_API_EXPORT
#define STEAMCLIENT_API static
#endif

#define GOLDBERG_CALLBACK_INTERNAL(parent, fname, cb_type) \
    struct GB_CCallbackInternal_ ## fname : private GB_CCallbackInterImp< sizeof(cb_type) > { \
        public: \
            ~GB_CCallbackInternal_ ## fname () { if (m_nCallbackFlags & k_ECallbackFlagsRegistered) { \
                                                    bool ready = false; \
                                                    do { \
                                                        if (global_mutex.try_lock() == true) { \
                                                            Steam_Client * client = get_steam_client(); \
                                                            if (client != NULL) { \
                                                                get_steam_client()->UnregisterCallback(this); \
                                                                ready = true; \
                                                            } \
                                                            global_mutex.unlock(); \
                                                        } \
                                                    } while (!ready); \
                                                 } }\
            GB_CCallbackInternal_ ## fname () {} \
            GB_CCallbackInternal_ ## fname ( const GB_CCallbackInternal_ ## fname & ) {} \
            GB_CCallbackInternal_ ## fname & operator=(const GB_CCallbackInternal_ ## fname &) { return *this; } \
        private: \
            virtual void Run(void *callback) { \
                if (!(m_nCallbackFlags & k_ECallbackFlagsRegistered)) { \
                   bool ready = false; \
                   do { \
                       if (global_mutex.try_lock() == true) { \
                           Steam_Client * client = get_steam_client(); \
                           if (client != NULL) { \
                               client->RegisterCallback(this, cb_type::k_iCallback); \
                               ready = true; \
                           } \
                           global_mutex.unlock(); \
                       } \
                   } while (!ready); \
                } \
                if (m_nCallbackFlags & k_ECallbackFlagsRegistered) { \
                    parent *obj = reinterpret_cast<parent*>(reinterpret_cast<char*>(this) - offsetof(parent, m_steamcallback_ ## fname)); \
                    obj->fname(reinterpret_cast<cb_type*>(callback)); \
                } \
        } \
    } m_steamcallback_ ## fname ; void fname( cb_type *callback )

template<int sizeof_cb_type>
class GB_CCallbackInterImp : protected CCallbackBase
{
    public:
        virtual ~GB_CCallbackInterImp() { if (m_nCallbackFlags & k_ECallbackFlagsRegistered) {
                                              bool ready = false;
                                              do {
                                                  if (global_mutex.try_lock() == true) {
                                                      Steam_Client * client = get_steam_client();
                                                      if (client != NULL) {
                                                          get_steam_client()->UnregisterCallback(this);
                                                          ready = true;
                                                      }
                                                      global_mutex.unlock();
                                                  }
                                              } while (!ready);
                                          } }
        void SetGameserverFlag() { m_nCallbackFlags |= k_ECallbackFlagsGameServer; }
    protected:
        friend class CCallbackMgr;
        virtual void Run(void *callback) = 0;
        virtual void Run( void *callback, bool io_failure, SteamAPICall_t api_fp) { Run(callback); }
        virtual int GetCallbackSizeBytes() { return sizeof_cb_type; }
};

Steam_Client *get_steam_client();
bool steamclient_has_ipv6_functions();
Steam_Client *try_get_steam_client();

HSteamUser flat_hsteamuser();
HSteamPipe flat_hsteampipe();
HSteamUser flat_gs_hsteamuser();
HSteamPipe flat_gs_hsteampipe();
