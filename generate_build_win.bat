@echo off
REM Should be called from the root of the repo.

REM Make build and output dirs.
IF NOT EXIST build ( mkdir build )
IF NOT EXIST build\cmds ( mkdir build\cmds )
IF NOT EXIST build\debug ( mkdir build\debug )
IF NOT EXIST build\release ( mkdir build\release )
IF NOT EXIST build\all_deps ( mkdir build\all_deps )
IF NOT EXIST build\all_deps\debug ( mkdir build\all_deps\debug )
IF NOT EXIST build\all_deps\release ( mkdir build\all_deps\release )
IF NOT EXIST build\experimental ( mkdir build\experimental )
IF NOT EXIST build\experimental\debug ( mkdir build\experimental\debug )
IF NOT EXIST build\experimental\release ( mkdir build\experimental\release )
IF NOT EXIST build\experimental_steamclient ( mkdir build\experimental_steamclient )
IF NOT EXIST build\experimental_steamclient\debug ( mkdir build\experimental_steamclient\debug )
IF NOT EXIST build\experimental_steamclient\release ( mkdir build\experimental_steamclient\release )
IF NOT EXIST build\experimental_steamclient\steamclient_loader ( mkdir build\experimental_steamclient\steamclient_loader )
IF NOT EXIST build\experimental_steamclient\steamclient_loader\debug ( mkdir build\experimental_steamclient\steamclient_loader\debug )
IF NOT EXIST build\experimental_steamclient\steamclient_loader\release ( mkdir build\experimental_steamclient\steamclient_loader\release )
IF NOT EXIST build\lobby_connect\debug ( mkdir build\lobby_connect\debug )
IF NOT EXIST build\lobby_connect\release ( mkdir build\lobby_connect\release )
IF NOT EXIST build\release\tools ( mkdir build\release\tools )
IF NOT EXIST build\release\tools\debug ( mkdir build\release\tools\debug )
IF NOT EXIST build\release\tools\release ( mkdir build\release\tools\release )

IF NOT EXIST build\debug\x86 ( mkdir build\debug\x86 )
IF NOT EXIST build\debug\x64 ( mkdir build\debug\x64 )
IF NOT EXIST build\release\x86 ( mkdir build\release\x86 )
IF NOT EXIST build\release\x64 ( mkdir build\release\x64 )

IF NOT EXIST build\all_deps\debug\x86 ( mkdir build\all_deps\debug\x86 )
IF NOT EXIST build\all_deps\debug\x64 ( mkdir build\all_deps\debug\x64 )
IF NOT EXIST build\all_deps\release\x86 ( mkdir build\all_deps\release\x86 )
IF NOT EXIST build\all_deps\release\x64 ( mkdir build\all_deps\release\x64 )

IF NOT EXIST build\experimental\debug\x86 ( mkdir build\experimental\debug\x86 )
IF NOT EXIST build\experimental\debug\x64 ( mkdir build\experimental\debug\x64 )
IF NOT EXIST build\experimental\release\x86 ( mkdir build\experimental\release\x86 )
IF NOT EXIST build\experimental\release\x64 ( mkdir build\experimental\release\x64 )

IF NOT EXIST build\experimental_steamclient\debug\x86 ( mkdir build\experimental_steamclient\debug\x86 )
IF NOT EXIST build\experimental_steamclient\debug\x64 ( mkdir build\experimental_steamclient\debug\x64 )
IF NOT EXIST build\experimental_steamclient\release\x86 ( mkdir build\experimental_steamclient\release\x86 )
IF NOT EXIST build\experimental_steamclient\release\x64 ( mkdir build\experimental_steamclient\release\x64 )
IF NOT EXIST build\experimental_steamclient\debug\x86\deps ( mkdir build\experimental_steamclient\debug\x86\deps )
IF NOT EXIST build\experimental_steamclient\debug\x64\deps ( mkdir build\experimental_steamclient\debug\x64\deps )
IF NOT EXIST build\experimental_steamclient\release\x86\deps ( mkdir build\experimental_steamclient\release\x86\deps )
IF NOT EXIST build\experimental_steamclient\release\x64\deps ( mkdir build\experimental_steamclient\release\x64\deps )
IF NOT EXIST build\experimental_steamclient\debug\x86\sc_deps ( mkdir build\experimental_steamclient\debug\x86\sc_deps )
IF NOT EXIST build\experimental_steamclient\debug\x64\sc_deps ( mkdir build\experimental_steamclient\debug\x64\sc_deps )
IF NOT EXIST build\experimental_steamclient\release\x86\sc_deps ( mkdir build\experimental_steamclient\release\x86\sc_deps )
IF NOT EXIST build\experimental_steamclient\release\x64\sc_deps ( mkdir build\experimental_steamclient\release\x64\sc_deps )

IF NOT EXIST build\experimental_steamclient\steamclient_loader\debug\x86 ( mkdir build\experimental_steamclient\steamclient_loader\debug\x86 )
IF NOT EXIST build\experimental_steamclient\steamclient_loader\debug\x64 ( mkdir build\experimental_steamclient\steamclient_loader\debug\x64 )
IF NOT EXIST build\experimental_steamclient\steamclient_loader\release\x86 ( mkdir build\experimental_steamclient\steamclient_loader\release\x86 )
IF NOT EXIST build\experimental_steamclient\steamclient_loader\release\x64 ( mkdir build\experimental_steamclient\steamclient_loader\release\x64 )

IF NOT EXIST build\lobby_connect\debug\x86 ( mkdir build\lobby_connect\debug\x86 )
IF NOT EXIST build\lobby_connect\debug\x64 ( mkdir build\lobby_connect\debug\x64 )
IF NOT EXIST build\lobby_connect\release\x86 ( mkdir build\lobby_connect\release\x86 )
IF NOT EXIST build\lobby_connect\release\x64 ( mkdir build\lobby_connect\release\x64 )
IF NOT EXIST build\release\tools\debug\x86 ( mkdir build\release\tools\debug\x86 )
IF NOT EXIST build\release\tools\debug\x64 ( mkdir build\release\tools\debug\x64 )
IF NOT EXIST build\release\tools\release\x86 ( mkdir build\release\tools\release\x86 )
IF NOT EXIST build\release\tools\release\x64 ( mkdir build\release\tools\release\x64 )

IF NOT EXIST debug ( mkdir debug )
IF NOT EXIST debug\experimental ( mkdir debug\experimental )
IF NOT EXIST debug\experimental_steamclient ( mkdir debug\experimental_steamclient )
IF NOT EXIST debug\lobby_connect ( mkdir debug\lobby_connect )
IF NOT EXIST debug\tools ( mkdir debug\tools )

IF NOT EXIST release ( mkdir release )
IF NOT EXIST release\experimental ( mkdir release\experimental )
IF NOT EXIST release\experimental_steamclient ( mkdir release\experimental_steamclient )
IF NOT EXIST release\lobby_connect ( mkdir release\lobby_connect )
IF NOT EXIST release\tools ( mkdir release\tools )

SET CDS_DIR=%cd%\build\cmds

REM
REM Arguments.
REM

REM normal_args.
IF EXIST %CDS_DIR%\NORMAL_ARGS.ARG ( DEL /F /S /Q %CDS_DIR%\NORMAL_ARGS.ARG )
echo /EHsc > %CDS_DIR%\NORMAL_ARGS.ARG
echo /Ox >> %CDS_DIR%\NORMAL_ARGS.ARG

REM JOB ARGS.
IF "%JOB_COUNT%" == "" ( echo /MP1 >> %CDS_DIR%\NORMAL_ARGS.ARG )
IF NOT "%JOB_COUNT%" == "" ( echo /MP%JOB_COUNT% >> %CDS_DIR%\NORMAL_ARGS.ARG )

REM Debug args.
IF EXIST %CDS_DIR%\DEBUG.BLD ( DEL /F /S /Q %CDS_DIR%\DEBUG.BLD )
IF EXIST %CDS_DIR%\DEBUG.LKS ( DEL /F /S /Q %CDS_DIR%\DEBUG.LKS )

REM Create empty file. (No BUILD time args currently.)
type NUL >> %CDS_DIR%\DEBUG.BLD

REM DISABLE the PDB builds if we are running on the CI. (It can't build them currently.)
IF EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /DEBUG:NONE > %CDS_DIR%\DEBUG.LKS )
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /DEBUG:FULL /OPT:REF /OPT:ICF > %CDS_DIR%\DEBUG.LKS )

REM Release args.
IF EXIST %CDS_DIR%\RELEASE.ARG ( DEL /F /S /Q %CDS_DIR%\RELEASE.ARG )
IF EXIST %CDS_DIR%\RELEASE.LKS ( DEL /F /S /Q %CDS_DIR%\RELEASE.LKS )
REM Release mode Flags.
echo /DEMU_RELEASE_BUILD > %CDS_DIR%\RELEASE.ARG
echo /DNDEBUG >> %CDS_DIR%\RELEASE.ARG
copy %CDS_DIR%\RELEASE.ARG %CDS_DIR%\RELEASE.BLD
type %CDS_DIR%\RELEASE.ARG > %CDS_DIR%\RELEASE.LKS

REM DISABLE the PDB builds if we are running on the CI. (It can't build them currently.)
IF EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /DEBUG:NONE >> %CDS_DIR%\RELEASE.LKS )
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /DEBUG:FULL /OPT:REF /OPT:ICF >> %CDS_DIR%\RELEASE.LKS )

REM BASE DLL Flags.
IF EXIST %CDS_DIR%\DLL_MAIN_CPP.ARG ( DEL /F /S /Q %CDS_DIR%\DLL_MAIN_CPP.ARG )

REM EXPERIMENTAL Flags.
IF EXIST %CDS_DIR%\EXPERIMENTAL.ARG ( DEL /F /S /Q %CDS_DIR%\EXPERIMENTAL.ARG )
echo /DEMU_EXPERIMENTAL_BUILD > %CDS_DIR%\EXPERIMENTAL.ARG
echo /DCONTROLLER_SUPPORT >> %CDS_DIR%\EXPERIMENTAL.ARG
echo /DEMU_OVERLAY >> %CDS_DIR%\EXPERIMENTAL.ARG

REM EXPERIMENTAL_STEAMCLIENT Flags.
IF EXIST %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.ARG ( DEL /F /S /Q %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.ARG )
echo /DSTEAMCLIENT_DLL > %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.ARG

REM lobby_connect Flags.
IF EXIST %CDS_DIR%\LOBBY_CONNECT.ARG ( DEL /F /S /Q %CDS_DIR%\LOBBY_CONNECT.ARG )
echo /DNO_DISK_WRITES > %CDS_DIR%\LOBBY_CONNECT.ARG
echo /DLOBBY_CONNECT >> %CDS_DIR%\LOBBY_CONNECT.ARG

REM
REM Includes.
REM

REM protobuf.
call build_set_protobuf_directories.bat
IF EXIST %CDS_DIR%\PROTOBUF_X86.ICD ( DEL /F /S /Q %CDS_DIR%\PROTOBUF_X86.ICD )
IF EXIST %CDS_DIR%\PROTOBUF_X64.ICD ( DEL /F /S /Q %CDS_DIR%\PROTOBUF_X64.ICD )
setlocal
SET TEST_A=%cd%
cd %PROTOBUF_X86_DIRECTORY%
SET TEST_B=%cd%\include
cd %TEST_A%
echo /I%TEST_B% > %CDS_DIR%\PROTOBUF_X86.ICD
endlocal
setlocal
SET TEST_A=%cd%
cd %PROTOBUF_X64_DIRECTORY%
SET TEST_B=%cd%\include
cd %TEST_A%
echo /I%TEST_B% > %CDS_DIR%\PROTOBUF_X64.ICD
endlocal

REM OVERLAY_EXPERIMENTAL.
IF EXIST %CDS_DIR%\OVERLAY_EXPERIMENTAL.ICD ( DEL /F /S /Q %CDS_DIR%\OVERLAY_EXPERIMENTAL.ICD )
echo /I%cd%\overlay_experimental > %CDS_DIR%\OVERLAY_EXPERIMENTAL.ICD

REM IMGUI.
IF EXIST %CDS_DIR%\IMGUI.ICD ( DEL /F /S /Q %CDS_DIR%\IMGUI.ICD )
echo /I%cd%\ImGui > %CDS_DIR%\IMGUI.ICD

REM
REM Link Libraries.
REM

REM protobuf.
IF EXIST %CDS_DIR%\PROTOBUF_X86.OS ( DEL /F /S /Q %CDS_DIR%\PROTOBUF_X86.OS )
IF EXIST %CDS_DIR%\PROTOBUF_X64.OS ( DEL /F /S /Q %CDS_DIR%\PROTOBUF_X64.OS )
dir /b /s %PROTOBUF_X86_LIBRARY% > %CDS_DIR%\PROTOBUF_X86.OS
dir /b /s %PROTOBUF_X64_LIBRARY% > %CDS_DIR%\PROTOBUF_X64.OS

REM BASE DLL.
IF EXIST %CDS_DIR%\DLL_MAIN_CPP.OS ( DEL /F /S /Q %CDS_DIR%\DLL_MAIN_CPP.OS )
echo dbghelp.lib > %CDS_DIR%\EXPERIMENTAL.OS
echo Iphlpapi.lib >> %CDS_DIR%\DLL_MAIN_CPP.OS
echo Ws2_32.lib >> %CDS_DIR%\DLL_MAIN_CPP.OS
echo Shell32.lib >> %CDS_DIR%\DLL_MAIN_CPP.OS
echo advapi32.lib >> %CDS_DIR%\DLL_MAIN_CPP.OS

REM EXPERIMENTAL.
IF EXIST %CDS_DIR%\EXPERIMENTAL.OS ( DEL /F /S /Q %CDS_DIR%\EXPERIMENTAL.OS )
echo Faultrep.lib > %CDS_DIR%\EXPERIMENTAL.OS
echo opengl32.lib >> %CDS_DIR%\EXPERIMENTAL.OS
echo Winmm.lib >> %CDS_DIR%\EXPERIMENTAL.OS

REM steamclient_loader.
IF EXIST %CDS_DIR%\STEAMCLIENT_LOADER.OS ( DEL /F /S /Q %CDS_DIR%\STEAMCLIENT_LOADER.OS )
echo advapi32.lib > %CDS_DIR%\STEAMCLIENT_LOADER.OS
echo user32.lib >> %CDS_DIR%\STEAMCLIENT_LOADER.OS

REM lobby_connect.
IF EXIST %CDS_DIR%\LOBBY_CONNECT.OS ( DEL /F /S /Q %CDS_DIR%\LOBBY_CONNECT.OS )
echo Comdlg32.lib > %CDS_DIR%\LOBBY_CONNECT.OS

REM
REM Files.
REM

REM Protobuf.
REM Needs to be compiled here (really just needs to exist), as we include it below.
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto

REM OVERLAY_EXPERIMENTAL.
IF EXIST %CDS_DIR%\OVERLAY_EXPERIMENTAL.FLS ( DEL /F /S /Q %CDS_DIR%\OVERLAY_EXPERIMENTAL.FLS )
IF EXIST %CDS_DIR%\OVERLAY_EXPERIMENTAL_SYSTEM.FLS ( DEL /F /S /Q %CDS_DIR%\OVERLAY_EXPERIMENTAL_SYSTEM.FLS )
where "%cd%\overlay_experimental\:*.cpp" > %CDS_DIR%\OVERLAY_EXPERIMENTAL.FLS
where "%cd%\overlay_experimental\windows\:*.cpp" >> %CDS_DIR%\OVERLAY_EXPERIMENTAL.FLS
where "%cd%\overlay_experimental\System\:*.cpp" >> %CDS_DIR%\OVERLAY_EXPERIMENTAL_SYSTEM.FLS

REM IMGUI.
IF EXIST %CDS_DIR%\IMGUI.FLS ( DEL /F /S /Q %CDS_DIR%\IMGUI.FLS )
where "%cd%\ImGui\:*.cpp" > %CDS_DIR%\IMGUI.FLS
where "%cd%\ImGui\backends\:imgui_impl_dx*.cpp" >> %CDS_DIR%\IMGUI.FLS
where "%cd%\ImGui\backends\:imgui_impl_win32.cpp" >> %CDS_DIR%\IMGUI.FLS
where "%cd%\ImGui\backends\:imgui_impl_vulkan.cpp" >> %CDS_DIR%\IMGUI.FLS
where "%cd%\ImGui\backends\:imgui_impl_opengl3.cpp" >> %CDS_DIR%\IMGUI.FLS
where "%cd%\ImGui\backends\:imgui_win_shader_blobs.cpp" >> %CDS_DIR%\IMGUI.FLS

REM DETOURS.
IF EXIST %CDS_DIR%\DETOURS.FLS ( DEL /F /S /Q %CDS_DIR%\DETOURS.FLS )
where "%cd%\detours\:*.cpp" > %CDS_DIR%\DETOURS.FLS

REM CONTROLLER.
IF EXIST %CDS_DIR%\CONTROLLER.FLS ( DEL /F /S /Q CONTROLLER.FLS )
where "%cd%\controller\:gamepad.c" > %CDS_DIR%\CONTROLLER.FLS

REM sc_different_deps.
IF EXIST %CDS_DIR%\SC_DIFFERENT_DEPS.FLS ( DEL /F /S /Q %CDS_DIR%\SC_DIFFERENT_DEPS.FLS )
where "%cd%\dll\:flat.cpp" > %CDS_DIR%\SC_DIFFERENT_DEPS.FLS
where "%cd%\dll\:dll.cpp" >> %CDS_DIR%\SC_DIFFERENT_DEPS.FLS

REM BASE DLL.
IF EXIST %CDS_DIR%\DLL_MAIN_CPP.FLS ( DEL /F /S /Q %CDS_DIR%\DLL_MAIN_CPP.FLS )
move %cd%\dll\flat.cpp %cd%\dll\flat.cpp.tmp
move %cd%\dll\dll.cpp %cd%\dll\dll.cpp.tmp
where "%cd%\dll\:*.cpp" > %CDS_DIR%\DLL_MAIN_CPP.FLS
move %cd%\dll\flat.cpp.tmp %cd%\dll\flat.cpp
move %cd%\dll\dll.cpp.tmp %cd%\dll\dll.cpp
where "%cd%\dll\:*.cc" >> %CDS_DIR%\DLL_MAIN_CPP.FLS

REM SteamClient.
IF EXIST %CDS_DIR%\STEAMCLIENT.FLS ( DEL /F /S /Q %CDS_DIR%\STEAMCLIENT.FLS )
where "%cd%\:steamclient.cpp" > %CDS_DIR%\STEAMCLIENT.FLS

REM steamclient_loader.
IF EXIST %CDS_DIR%\STEAMCLIENT_LOADER.FLS ( DEL /F /S /Q %CDS_DIR%\STEAMCLIENT_LOADER.FLS )
where "%cd%\steamclient_loader\:*.cpp" > %CDS_DIR%\STEAMCLIENT_LOADER.FLS

REM lobby_connect.
IF EXIST %CDS_DIR%\LOBBY_CONNECT.FLS ( DEL /F /S /Q %CDS_DIR%\LOBBY_CONNECT.FLS )
where "%cd%\:lobby_connect.cpp" > %CDS_DIR%\LOBBY_CONNECT.FLS

REM generate_interfaces_file.
IF EXIST %CDS_DIR%\GENERATE_INTERFACES_FILE.FLS ( DEL /F /S /Q %CDS_DIR%\GENERATE_INTERFACES_FILE.FLS )
where "%cd%\:generate_interfaces_file.cpp" > %CDS_DIR%\GENERATE_INTERFACES_FILE.FLS

REM
REM Build and link cmd script files.
REM

REM protobuf.
IF EXIST %CDS_DIR%\PROTOBUF_X86.BLD ( DEL /F /S /Q %CDS_DIR%\PROTOBUF_X86.BLD )
IF EXIST %CDS_DIR%\PROTOBUF_X86.LKS ( DEL /F /S /Q %CDS_DIR%\PROTOBUF_X86.LKS )
type %CDS_DIR%\PROTOBUF_X86.ICD > %CDS_DIR%\PROTOBUF_X86.BLD
type %CDS_DIR%\PROTOBUF_X86.BLD > %CDS_DIR%\PROTOBUF_X86.LKS
type %CDS_DIR%\PROTOBUF_X86.OS >> %CDS_DIR%\PROTOBUF_X86.LKS

IF EXIST %CDS_DIR%\PROTOBUF_X64.BLD ( DEL /F /S /Q %CDS_DIR%\PROTOBUF_X64.BLD )
IF EXIST %CDS_DIR%\PROTOBUF_X64.LKS ( DEL /F /S /Q %CDS_DIR%\PROTOBUF_X64.LKS )
type %CDS_DIR%\PROTOBUF_X64.ICD > %CDS_DIR%\PROTOBUF_X64.BLD
type %CDS_DIR%\PROTOBUF_X64.BLD > %CDS_DIR%\PROTOBUF_X64.LKS
type %CDS_DIR%\PROTOBUF_X64.OS >> %CDS_DIR%\PROTOBUF_X64.LKS

REM SC_DEPS
IF EXIST %CDS_DIR%\SC_DEPS.BLD ( DEL /F /S /Q %CDS_DIR%\SC_DEPS.BLD )
type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\SC_DEPS.BLD
type %CDS_DIR%\EXPERIMENTAL.ARG >> %CDS_DIR%\SC_DEPS.BLD
type %CDS_DIR%\OVERLAY_EXPERIMENTAL.ICD >> %CDS_DIR%\SC_DEPS.BLD
type %CDS_DIR%\IMGUI.ICD >> %CDS_DIR%\SC_DEPS.BLD
type %CDS_DIR%\DLL_MAIN_CPP.FLS >> %CDS_DIR%\SC_DEPS.BLD
type %CDS_DIR%\OVERLAY_EXPERIMENTAL.FLS >> %CDS_DIR%\SC_DEPS.BLD

REM DEPS
IF EXIST %CDS_DIR%\DEPS.BLD ( DEL /F /S /Q %CDS_DIR%\DEPS.BLD )
type %CDS_DIR%\DETOURS.FLS >> %CDS_DIR%\DEPS.BLD
type %CDS_DIR%\CONTROLLER.FLS >> %CDS_DIR%\DEPS.BLD
type %CDS_DIR%\IMGUI.FLS >> %CDS_DIR%\DEPS.BLD
type %CDS_DIR%\OVERLAY_EXPERIMENTAL_SYSTEM.FLS >> %CDS_DIR%\DEPS.BLD

REM all_deps.
IF EXIST %CDS_DIR%\ALL_DEPS.BLD ( DEL /F /S /Q %CDS_DIR%\ALL_DEPS.BLD )
type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\ALL_DEPS.BLD
type %CDS_DIR%\OVERLAY_EXPERIMENTAL.ICD >> %CDS_DIR%\ALL_DEPS.BLD
type %CDS_DIR%\IMGUI.ICD >> %CDS_DIR%\ALL_DEPS.BLD
type %CDS_DIR%\DETOURS.FLS >> %CDS_DIR%\ALL_DEPS.BLD
type %CDS_DIR%\CONTROLLER.FLS >> %CDS_DIR%\ALL_DEPS.BLD
type %CDS_DIR%\IMGUI.FLS >> %CDS_DIR%\ALL_DEPS.BLD
type %CDS_DIR%\OVERLAY_EXPERIMENTAL_SYSTEM.FLS >> %CDS_DIR%\ALL_DEPS.BLD

REM BASE DLL.
IF EXIST %CDS_DIR%\DLL_MAIN_CPP.BLD ( DEL /F /S /Q %CDS_DIR%\DLL_MAIN_CPP.BLD )
IF EXIST %CDS_DIR%\DLL_MAIN_CPP.LKS ( DEL /F /S /Q %CDS_DIR%\DLL_MAIN_CPP.LKS )
type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\DLL_MAIN_CPP.BLD
type %CDS_DIR%\DLL_MAIN_CPP.FLS >> %CDS_DIR%\DLL_MAIN_CPP.BLD
type %CDS_DIR%\SC_DIFFERENT_DEPS.FLS >> %CDS_DIR%\DLL_MAIN_CPP.BLD
type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\DLL_MAIN_CPP.LKS
type %CDS_DIR%\DLL_MAIN_CPP.OS >> %CDS_DIR%\DLL_MAIN_CPP.LKS

REM EXPERIMENTAL.
IF EXIST %CDS_DIR%\EXPERIMENTAL.BLD ( DEL /F /S /Q %CDS_DIR%\EXPERIMENTAL.BLD )
IF EXIST %CDS_DIR%\EXPERIMENTAL.LKS ( DEL /F /S /Q %CDS_DIR%\EXPERIMENTAL.LKS )

REM Note the order and repeats. cl will complain if this gets messed up.
REM OLD SCRIPT.
REM type NORMAL_ARGS.ARG > EXPERIMENTAL.BLD
REM type EXPERIMENTAL.ARG >> EXPERIMENTAL.BLD
REM type EXPERIMENTAL.ICD >> EXPERIMENTAL.BLD
REM type DLL_MAIN_CPP.FLS >> EXPERIMENTAL.BLD
REM type SC_DIFFERENT_DEPS.FLS >> EXPERIMENTAL.BLD
REM type OVERLAY_EXPERIMENTAL.FLS >> EXPERIMENTAL.BLD
REM type OVERLAY_EXPERIMENTAL_SYSTEM.FLS >> EXPERIMENTAL.BLD
REM type DETOURS.FLS >> EXPERIMENTAL.BLD
REM type CONTROLLER.FLS >> EXPERIMENTAL.BLD
REM type IMGUI.FLS >> EXPERIMENTAL.BLD
REM type NORMAL_ARGS.ARG > EXPERIMENTAL.LKS
REM type EXPERIMENTAL.ARG >> EXPERIMENTAL.LKS
REM type EXPERIMENTAL.ICD >> EXPERIMENTAL.LKS
REM type DLL_MAIN_CPP.OS >> EXPERIMENTAL.LKS
REM type EXPERIMENTAL.OS >> EXPERIMENTAL.LKS
REM NEW Combined experimental && experimental_steamclient SCRIPT.
type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\EXPERIMENTAL.BLD
type %CDS_DIR%\EXPERIMENTAL.ARG >> %CDS_DIR%\EXPERIMENTAL.BLD
type %CDS_DIR%\OVERLAY_EXPERIMENTAL.ICD >> %CDS_DIR%\EXPERIMENTAL.BLD
type %CDS_DIR%\IMGUI.ICD >> %CDS_DIR%\EXPERIMENTAL.BLD
type %CDS_DIR%\SC_DIFFERENT_DEPS.FLS >> %CDS_DIR%\EXPERIMENTAL.BLD

type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\EXPERIMENTAL.LKS
type %CDS_DIR%\EXPERIMENTAL.ARG >> %CDS_DIR%\EXPERIMENTAL.LKS
type %CDS_DIR%\OVERLAY_EXPERIMENTAL.ICD >> %CDS_DIR%\EXPERIMENTAL.LKS
type %CDS_DIR%\IMGUI.ICD >> %CDS_DIR%\EXPERIMENTAL.LKS
type %CDS_DIR%\DLL_MAIN_CPP.OS >> %CDS_DIR%\EXPERIMENTAL.LKS
type %CDS_DIR%\EXPERIMENTAL.OS >> %CDS_DIR%\EXPERIMENTAL.LKS

REM SteamClient.
IF EXIST %CDS_DIR%\STEAMCLIENT.BLD ( DEL /F /S /Q %CDS_DIR%\STEAMCLIENT.BLD )
type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\STEAMCLIENT.BLD
type %CDS_DIR%\EXPERIMENTAL.ARG >> %CDS_DIR%\STEAMCLIENT.BLD
type %CDS_DIR%\OVERLAY_EXPERIMENTAL.ICD >> %CDS_DIR%\STEAMCLIENT.BLD
type %CDS_DIR%\IMGUI.ICD >> %CDS_DIR%\STEAMCLIENT.BLD
type %CDS_DIR%\STEAMCLIENT.FLS >> %CDS_DIR%\STEAMCLIENT.BLD

REM EXPERIMENTAL_STEAMCLIENT.
IF EXIST %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.BLD ( DEL /F /S /Q %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.BLD )
IF EXIST %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS ( DEL /F /S /Q %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS )

REM Note the order and repeats. cl will complain if this gets messed up.




REM FULL
type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS
type %CDS_DIR%\EXPERIMENTAL.ARG >> %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS
type %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.ARG >> %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS
type %CDS_DIR%\SC_DIFFERENT_DEPS.FLS >> %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS
type %CDS_DIR%\DLL_MAIN_CPP.OS >> %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS
type %CDS_DIR%\EXPERIMENTAL.OS >> %CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS

REM steamclient_loader.
IF EXIST %CDS_DIR%\STEAMCLIENT_LOADER.BLD ( DEL /F /S /Q %CDS_DIR%\STEAMCLIENT_LOADER.BLD )
IF EXIST %CDS_DIR%\STEAMCLIENT_LOADER.LKS ( DEL /F /S /Q %CDS_DIR%\STEAMCLIENT_LOADER.LKS )

type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\STEAMCLIENT_LOADER.BLD
type %CDS_DIR%\STEAMCLIENT_LOADER.FLS >> %CDS_DIR%\STEAMCLIENT_LOADER.BLD

type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\STEAMCLIENT_LOADER.LKS
type %CDS_DIR%\STEAMCLIENT_LOADER.OS >> %CDS_DIR%\STEAMCLIENT_LOADER.LKS

REM lobby_connect.
IF EXIST %CDS_DIR%\LOBBY_CONNECT.BLD ( DEL /F /S /Q %CDS_DIR%\LOBBY_CONNECT.BLD )
IF EXIST %CDS_DIR%\LOBBY_CONNECT.LKS ( DEL /F /S /Q %CDS_DIR%\LOBBY_CONNECT.LKS )
type %CDS_DIR%\LOBBY_CONNECT.ARG > %CDS_DIR%\LOBBY_CONNECT.BLD
type %CDS_DIR%\NORMAL_ARGS.ARG >> %CDS_DIR%\LOBBY_CONNECT.BLD
type %CDS_DIR%\LOBBY_CONNECT.FLS >> %CDS_DIR%\LOBBY_CONNECT.BLD
type %CDS_DIR%\DLL_MAIN_CPP.FLS >> %CDS_DIR%\LOBBY_CONNECT.BLD
type %CDS_DIR%\SC_DIFFERENT_DEPS.FLS >> %CDS_DIR%\LOBBY_CONNECT.BLD

type %CDS_DIR%\LOBBY_CONNECT.ARG > %CDS_DIR%\LOBBY_CONNECT.LKS
type %CDS_DIR%\NORMAL_ARGS.ARG >> %CDS_DIR%\LOBBY_CONNECT.LKS
type %CDS_DIR%\DLL_MAIN_CPP.OS >> %CDS_DIR%\LOBBY_CONNECT.LKS
type %CDS_DIR%\LOBBY_CONNECT.OS >> %CDS_DIR%\LOBBY_CONNECT.LKS

REM GENERATE_INTERFACES_FILE
IF EXIST %CDS_DIR%\GENERATE_INTERFACES_FILE.BLD ( DEL /F /S /Q %CDS_DIR%\GENERATE_INTERFACES_FILE.BLD )
IF EXIST %CDS_DIR%\GENERATE_INTERFACES_FILE.LKS ( DEL /F /S /Q %CDS_DIR%\GENERATE_INTERFACES_FILE.LKS )
type %CDS_DIR%\NORMAL_ARGS.ARG >> %CDS_DIR%\GENERATE_INTERFACES_FILE.BLD
type %CDS_DIR%\GENERATE_INTERFACES_FILE.FLS >> %CDS_DIR%\GENERATE_INTERFACES_FILE.BLD

type %CDS_DIR%\NORMAL_ARGS.ARG > %CDS_DIR%\GENERATE_INTERFACES_FILE.LKS
