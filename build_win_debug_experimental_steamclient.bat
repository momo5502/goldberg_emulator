@echo off
cd /d "%~dp0"

IF NOT EXIST build ( mkdir build )
IF NOT EXIST build\experimental_steamclient ( mkdir build\experimental_steamclient )
IF NOT EXIST build\experimental_steamclient\debug ( mkdir build\experimental_steamclient\debug )
IF NOT EXIST build\experimental_steamclient\debug\x86 ( mkdir build\experimental_steamclient\debug\x86 )
IF NOT EXIST build\experimental_steamclient\debug\x64 ( mkdir build\experimental_steamclient\debug\x64 )
IF EXIST build\experimental_steamclient\debug\x86\*.* ( DEL /F /S /Q build\experimental_steamclient\debug\x86\*.* )
IF EXIST build\experimental_steamclient\debug\x64\*.* ( DEL /F /S /Q build\experimental_steamclient\debug\x64\*.* )

IF NOT EXIST debug ( mkdir debug )
IF NOT EXIST debug\experimental_steamclient ( mkdir debug\experimental_steamclient )
IF EXIST debug\experimental_steamclient\*.* ( DEL /F /S /Q debug\experimental_steamclient\*.* )

call build_set_protobuf_directories.bat

setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
SET OLD_DIR=%cd%
cd build\experimental_steamclient\debug\x86
cl %OLD_DIR%/dll/rtlgenrandom.c %OLD_DIR%/dll/rtlgenrandom.def
cl /LD /I%OLD_DIR%/ImGui /I%OLD_DIR%/%PROTOBUF_X86_DIRECTORY%\include\ /DSTEAMCLIENT_DLL /DCONTROLLER_SUPPORT /DEMU_EXPERIMENTAL_BUILD /DEMU_OVERLAY /I%OLD_DIR%/overlay_experimental %OLD_DIR%/dll/*.cpp %OLD_DIR%/dll/*.cc %OLD_DIR%/detours/*.cpp %OLD_DIR%/controller/gamepad.c %OLD_DIR%/ImGui/*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_dx*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_win32.cpp %OLD_DIR%/ImGui/backends/imgui_impl_vulkan.cpp %OLD_DIR%/ImGui/backends/imgui_impl_opengl3.cpp %OLD_DIR%/ImGui/backends/imgui_win_shader_blobs.cpp %OLD_DIR%/overlay_experimental/*.cpp %OLD_DIR%/overlay_experimental/windows/*.cpp %OLD_DIR%/overlay_experimental/System/*.cpp "%OLD_DIR%/%PROTOBUF_X86_LIBRARY%" opengl32.lib Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Winmm.lib /EHsc /MP12 /DDEBUG:FULL /Zi /Fd:%OLD_DIR%\debug\experimental_steamclient\steamclient.pdb /link /OUT:%OLD_DIR%\debug\experimental_steamclient\steamclient.dll
cl %OLD_DIR%/steamclient_loader/*.cpp advapi32.lib user32.lib /EHsc /MP12 /Ox /DDEBUG:FULL /Zi /Fd:%OLD_DIR%\debug\experimental_steamclient\steamclient_loader_x32.pdb /link /OUT:%OLD_DIR%\debug\experimental_steamclient\steamclient_loader_x32.exe
cd %OLD_DIR%
endlocal
setlocal
call build_env_x64.bat
SET OLD_DIR=%cd%
cd build\experimental_steamclient\debug\x64
cl %OLD_DIR%/dll/rtlgenrandom.c %OLD_DIR%/dll/rtlgenrandom.def
cl /LD /I%OLD_DIR%/ImGui /I%OLD_DIR%/%PROTOBUF_X64_DIRECTORY%\include\ /DSTEAMCLIENT_DLL /DCONTROLLER_SUPPORT /DEMU_EXPERIMENTAL_BUILD /DEMU_OVERLAY /I%OLD_DIR%/overlay_experimental %OLD_DIR%/dll/*.cpp %OLD_DIR%/dll/*.cc %OLD_DIR%/detours/*.cpp %OLD_DIR%/controller/gamepad.c %OLD_DIR%/ImGui/*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_dx*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_win32.cpp %OLD_DIR%/ImGui/backends/imgui_impl_vulkan.cpp %OLD_DIR%/ImGui/backends/imgui_impl_opengl3.cpp %OLD_DIR%/ImGui/backends/imgui_win_shader_blobs.cpp %OLD_DIR%/overlay_experimental/*.cpp %OLD_DIR%/overlay_experimental/windows/*.cpp %OLD_DIR%/overlay_experimental/System/*.cpp "%OLD_DIR%/%PROTOBUF_X64_LIBRARY%" opengl32.lib Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Winmm.lib /EHsc /MP12 /DDEBUG:FULL /Zi /Fd:%OLD_DIR%\debug\experimental_steamclient\steamclient64.pdb /link /OUT:%OLD_DIR%\debug\experimental_steamclient\steamclient64.dll
cl %OLD_DIR%/steamclient_loader/*.cpp advapi32.lib user32.lib /EHsc /MP12 /Ox /DDEBUG:FULL /Zi /Fd:%OLD_DIR%\debug\experimental_steamclient\steamclient_loader_x64.pdb /link /OUT:%OLD_DIR%\debug\experimental_steamclient\steamclient_loader_x64.exe
cd %OLD_DIR%
endlocal
