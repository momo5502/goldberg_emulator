@echo off
cd /d "%~dp0"

IF NOT EXIST build ( mkdir build )
IF NOT EXIST build\experimental ( mkdir build\experimental )
IF NOT EXIST build\experimental\debug ( mkdir build\experimental\debug )
IF NOT EXIST build\experimental\debug\x86 ( mkdir build\experimental\debug\x86 )
IF NOT EXIST build\experimental\debug\x64 ( mkdir build\experimental\debug\x64 )
IF EXIST build\experimental\debug\x86\*.* ( DEL /F /S /Q build\experimental\debug\x86\*.* )
IF EXIST build\experimental\debug\x64\*.* ( DEL /F /S /Q build\experimental\debug\x64\*.* )

IF NOT EXIST debug ( mkdir debug )
IF NOT EXIST debug\experimental ( mkdir debug\experimental )
IF EXIST debug\experimental\*.* ( DEL /F /S /Q debug\experimental\*.* )

call build_set_protobuf_directories.bat

setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
SET OLD_DIR=%cd%
cd build\experimental\debug\x86
cl %OLD_DIR%/dll/rtlgenrandom.c %OLD_DIR%/dll/rtlgenrandom.def
cl /LD /I%OLD_DIR%/ImGui /I%OLD_DIR%/%PROTOBUF_X86_DIRECTORY%\include\ /DEMU_EXPERIMENTAL_BUILD /DCONTROLLER_SUPPORT /DEMU_OVERLAY /I%OLD_DIR%/overlay_experimental %OLD_DIR%/dll/*.cpp %OLD_DIR%/dll/*.cc %OLD_DIR%/detours/*.cpp %OLD_DIR%/controller/gamepad.c %OLD_DIR%/ImGui/*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_dx*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_win32.cpp %OLD_DIR%/ImGui/backends/imgui_impl_vulkan.cpp %OLD_DIR%/ImGui/backends/imgui_impl_opengl3.cpp %OLD_DIR%/ImGui/backends/imgui_win_shader_blobs.cpp %OLD_DIR%/overlay_experimental/*.cpp %OLD_DIR%/overlay_experimental/windows/*.cpp %OLD_DIR%/overlay_experimental/System/*.cpp "%OLD_DIR%/%PROTOBUF_X86_LIBRARY%" opengl32.lib Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Winmm.lib /EHsc /MP12 /DDEBUG:FULL /Zi /Fd:%OLD_DIR%\debug\experimental\steam_api.pdb /link /OUT:%OLD_DIR%\debug\experimental\steam_api.dll
cl /LD %OLD_DIR%/steamclient.cpp /EHsc /MP12 /DDEBUG:FULL /Zi /Fd:%OLD_DIR%\debug\experimental\steamclient.pdb /link /OUT:%OLD_DIR%\debug\experimental\steamclient.dll
cd %OLD_DIR%
endlocal

setlocal
"%PROTOC_X64_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x64.bat
SET OLD_DIR=%cd%
cd build\experimental\debug\x64
cl %OLD_DIR%/dll/rtlgenrandom.c %OLD_DIR%/dll/rtlgenrandom.def
cl /LD /I%OLD_DIR%/ImGui /I%OLD_DIR%/%PROTOBUF_X64_DIRECTORY%\include\ /DEMU_EXPERIMENTAL_BUILD /DCONTROLLER_SUPPORT /DEMU_OVERLAY /I%OLD_DIR%/overlay_experimental %OLD_DIR%/dll/*.cpp %OLD_DIR%/dll/*.cc %OLD_DIR%/detours/*.cpp %OLD_DIR%/controller/gamepad.c %OLD_DIR%/ImGui/*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_dx*.cpp %OLD_DIR%/ImGui/backends/imgui_impl_win32.cpp %OLD_DIR%/ImGui/backends/imgui_impl_vulkan.cpp %OLD_DIR%/ImGui/backends/imgui_impl_opengl3.cpp %OLD_DIR%/ImGui/backends/imgui_win_shader_blobs.cpp %OLD_DIR%/overlay_experimental/*.cpp %OLD_DIR%/overlay_experimental/windows/*.cpp %OLD_DIR%/overlay_experimental/System/*.cpp "%OLD_DIR%/%PROTOBUF_X64_LIBRARY%" opengl32.lib Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Winmm.lib /EHsc /MP12 /DDEBUG:FULL /Zi /Fd:%OLD_DIR%\debug\experimental\steam_api64.pdb /link /OUT:%OLD_DIR%\debug\experimental\steam_api64.dll
cl /LD %OLD_DIR%/steamclient.cpp /EHsc /MP12 /DDEBUG:FULL /Zi /Fd:%OLD_DIR%\debug\experimental\steamclient64.pdb /link /OUT:%OLD_DIR%\debug\experimental\steamclient64.dll
cd %OLD_DIR%
endlocal
