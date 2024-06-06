@echo off
cd /d "%~dp0"
IF NOT EXIST build ( mkdir build )
IF NOT EXIST build\experimental ( mkdir build\experimental )
IF NOT EXIST build\experimental\steamclient ( mkdir build\experimental\steamclient )
IF NOT EXIST build\experimental\steamclient\release ( mkdir build\experimental\steamclient\release )
IF NOT EXIST build\experimental\steamclient\release\x86 ( mkdir build\experimental\steamclient\release\x86 )
IF NOT EXIST build\experimental\steamclient\release\x64 ( mkdir build\experimental\steamclient\release\x64 )
IF EXIST build\experimental\steamclient\release\x86\*.* ( DEL /F /S /Q build\experimental\steamclient\release\x86\*.* )
IF EXIST build\experimental\steamclient\release\x64\*.* ( DEL /F /S /Q build\experimental\steamclient\release\x64\*.* )

IF NOT EXIST release\experimental_steamclient ( mkdir release\experimental_steamclient )
IF EXIST release\experimental_steamclient\*.* ( DEL /F /S /Q release\experimental_steamclient\*.* )

call build_set_protobuf_directories.bat

setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
SET OLD_DIR=%cd%
cd "build\experimental\steamclient\release\x86"
cl %OLD_DIR%/dll/rtlgenrandom.c %OLD_DIR%/dll/rtlgenrandom.def
cl /LD /DEMU_RELEASE_BUILD /DEMU_EXPERIMENTAL_BUILD /DSTEAMCLIENT_DLL /DCONTROLLER_SUPPORT /DEMU_OVERLAY /I%OLD_DIR%/ImGui /DNDEBUG /I%OLD_DIR%/%PROTOBUF_X86_DIRECTORY%\include\ /I%OLD_DIR%/overlay_experimental %OLD_DIR%/dll/*.cpp %OLD_DIR%/dll/*.cc %OLD_DIR%/detours/*.cpp %OLD_DIR%/controller/gamepad.c %OLD_DIR%/ImGui/*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_dx*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_win32.cpp %OLD_DIR%/ImGui/backends/imgui_impl_vulkan.cpp %OLD_DIR%/ImGui/backends/imgui_impl_opengl3.cpp %OLD_DIR%/ImGui/backends/imgui_win_shader_blobs.cpp %OLD_DIR%/overlay_experimental/*.cpp %OLD_DIR%/overlay_experimental/windows/*.cpp %OLD_DIR%/overlay_experimental/System/*.cpp "%OLD_DIR%/%PROTOBUF_X86_LIBRARY%" opengl32.lib Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Winmm.lib /EHsc /MP12 /Ox /link /debug:none /OUT:%OLD_DIR%\release\experimental_steamclient\steamclient.dll
cl %OLD_DIR%/steamclient_loader/*.cpp advapi32.lib user32.lib /EHsc /MP12 /Ox /link /debug:none /OUT:%OLD_DIR%\release\experimental_steamclient\steamclient_loader_x32.exe
cd %OLD_DIR%
copy steamclient_loader\ColdClientLoader.ini release\experimental_steamclient\
endlocal
setlocal
"%PROTOC_X64_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x64.bat
SET OLD_DIR=%cd%
cd "build\experimental\steamclient\release\x64"
cl %OLD_DIR%/dll/rtlgenrandom.c %OLD_DIR%/dll/rtlgenrandom.def
cl /LD /DEMU_RELEASE_BUILD /DEMU_EXPERIMENTAL_BUILD /DSTEAMCLIENT_DLL /DCONTROLLER_SUPPORT /DEMU_OVERLAY /I%OLD_DIR%/ImGui /DNDEBUG /I%OLD_DIR%/%PROTOBUF_X64_DIRECTORY%\include\ /I%OLD_DIR%/overlay_experimental %OLD_DIR%/dll/*.cpp %OLD_DIR%/dll/*.cc %OLD_DIR%/detours/*.cpp %OLD_DIR%/controller/gamepad.c %OLD_DIR%/ImGui/*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_dx*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_win32.cpp %OLD_DIR%/ImGui/backends/imgui_impl_vulkan.cpp %OLD_DIR%/ImGui/backends/imgui_impl_opengl3.cpp %OLD_DIR%/ImGui/backends/imgui_win_shader_blobs.cpp %OLD_DIR%/overlay_experimental/*.cpp %OLD_DIR%/overlay_experimental/windows/*.cpp %OLD_DIR%/overlay_experimental/System/*.cpp "%OLD_DIR%/%PROTOBUF_X64_LIBRARY%" opengl32.lib Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Winmm.lib /EHsc /MP12 /Ox /link /debug:none /OUT:%OLD_DIR%\release\experimental_steamclient\steamclient64.dll
cl %OLD_DIR%/steamclient_loader/*.cpp advapi32.lib user32.lib /EHsc /MP12 /Ox /link /debug:none /OUT:%OLD_DIR%\release\experimental_steamclient\steamclient_loader_x64.exe
cd %OLD_DIR%
copy Readme_experimental_steamclient.txt release\experimental_steamclient\Readme.txt
endlocal