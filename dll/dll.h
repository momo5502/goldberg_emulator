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
            GB_CCallbackInternal_ ## fname () { \
                PRINT_DEBUG("GB_CCallbackInternal_%s::%s %s 0x%p.\n", \
                            #fname, \
                            #fname, \
                            "Default constructor", \
                            this); \
                this->reg_thread_has_run = false; \
                std::thread th = std::thread(GB_CCallbackInterImp::_register, this, cb_type::k_iCallback); \
                th.detach(); \
            } \
            GB_CCallbackInternal_ ## fname ( const GB_CCallbackInternal_ ## fname & a ) { \
                PRINT_DEBUG("GB_CCallbackInternal_%s::%s %s 0x%p.\n", \
                            #fname, \
                            #fname, \
                            "Copy constructor", \
                            this); \
                this->reg_thread_has_run = false; \
                std::thread th = std::thread(GB_CCallbackInterImp::_register, this, cb_type::k_iCallback); \
                th.detach(); \
            } \
            GB_CCallbackInternal_ ## fname & operator=(const GB_CCallbackInternal_ ## fname &) { \
                PRINT_DEBUG("GB_CCallbackInternal_%s::%s %s 0x%p.\n", \
                            #fname, \
                            #fname, \
                            "Assignment = operator", \
                            this); \
                return *this; \
            } \
            virtual void Run(void *callback) { \
                PRINT_DEBUG("GB_CCallbackInternal_%s::Run 0x%p Callback argument: 0x%p.\n", \
                            #fname, \
                            this, \
                            callback); \
                if (m_nCallbackFlags & k_ECallbackFlagsRegistered) { \
                    parent *obj = reinterpret_cast<parent*>(reinterpret_cast<char*>(this) - offsetof(parent, m_steamcallback_ ## fname)); \
                    obj->fname(reinterpret_cast<cb_type*>(callback)); \
                } \
            } \
        private: \
    } m_steamcallback_ ## fname ; void fname(cb_type *callback )

template<int sizeof_cb_type>
class GB_CCallbackInterImp : protected CCallbackBase
{
    public:
        virtual ~GB_CCallbackInterImp() {
            _unregister(this);
            return;
        }
        void SetGameserverFlag() {
            m_nCallbackFlags |= k_ECallbackFlagsGameServer;
            return;
        }
    protected:
        friend class CCallbackMgr;
        std::atomic<bool> reg_thread_has_run;
        virtual void Run(void *callback) = 0;
        virtual void Run(void *callback, bool io_failure, SteamAPICall_t api_fp) {
            Run(callback);
            return;
        }
        virtual int GetCallbackSizeBytes() {
            return sizeof_cb_type;
        }
        static void _register(void * arg, int callback) {
            GB_CCallbackInterImp * gb = (GB_CCallbackInterImp *)arg;
            if (gb != NULL) {
                PRINT_DEBUG("GB_CCallbackInterImp::_register Begin registration thread for 0x%p callback %d.\n",
                            gb,
                            callback);
                if (!(gb->m_nCallbackFlags & k_ECallbackFlagsRegistered)) {
                    bool ready = false;
                    do {
                        if (global_mutex.try_lock() == true) {
                            Steam_Client * client = try_get_steam_client();
                            if (client != NULL) {
                                client->RegisterCallback(gb, callback);
                                ready = true;
                                gb->reg_thread_has_run = true;
                                PRINT_DEBUG("GB_CCallbackInterImp::_register Registration complete for 0x%p callback %d.\n",
                                            gb,
                                            callback);
                            }
                            global_mutex.unlock();
                        }
                    } while (!ready);
                }
                PRINT_DEBUG("GB_CCallbackInterImp::_register Exiting registration thread for 0x%p callback %d.\n",
                            gb,
                            callback);
            }
            return;
        }
        static void _unregister(void * arg) {
            GB_CCallbackInterImp * gb = (GB_CCallbackInterImp *)arg;
            if (gb != NULL) {
                PRINT_DEBUG("GB_CCallbackInterImp::_unregister Begin deregistration thread for 0x%p.\n",
                            gb);
                bool can_dereg = false;
                do {
                    can_dereg = gb->reg_thread_has_run;
                } while (!can_dereg);
                if (gb->m_nCallbackFlags & k_ECallbackFlagsRegistered) {
                    bool ready = false;
                    do {
                        if (global_mutex.try_lock() == true) {
                            Steam_Client * client = try_get_steam_client();
                            if (client != NULL) {
                                client->UnregisterCallback(gb);
                                ready = true;
                                PRINT_DEBUG("GB_CCallbackInterImp::_unregister Deregistration complete for 0x%p.\n",
                                            gb);
                            }
                            global_mutex.unlock();
                        }
                    } while (!ready);
                    PRINT_DEBUG("GB_CCallbackInterImp::_unregister Exiting deregistration thread for 0x%p.\n",
                                gb);
                }
            }
            return;
        }
};

Steam_Client *get_steam_client();
bool steamclient_has_ipv6_functions();
Steam_Client *try_get_steam_client();

HSteamUser flat_hsteamuser();
HSteamPipe flat_hsteampipe();
HSteamUser flat_gs_hsteamuser();
HSteamPipe flat_gs_hsteampipe();
