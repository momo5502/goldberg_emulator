@echo off
cd /d "%~dp0"

IF NOT EXIST build ( mkdir build )
IF NOT EXIST build\release ( mkdir build\release )
IF NOT EXIST build\release\x86 ( mkdir build\release\x86 )
IF NOT EXIST build\release\x64 ( mkdir build\release\x64 )
IF EXIST build\release\x86\*.* ( DEL /F /S /Q build\release\x86\*.* )
IF EXIST build\release\x64\*.* ( DEL /F /S /Q build\release\x64\*.* )

IF NOT EXIST release ( mkdir release )
IF EXIST release\steam_settings.EXAMPLE ( DEL /F /S /Q release\steam_settings.EXAMPLE )
IF EXIST release\*.dll ( DEL /F /Q release\*.dll )
IF EXIST release\*.txt ( DEL /F /Q release\*.txt )

call build_set_protobuf_directories.bat

SET OLD_DIR=%cd%

setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
cd build\release\x86
cl %OLD_DIR%/dll/rtlgenrandom.c %OLD_DIR%/dll/rtlgenrandom.def
cl /LD /DEMU_RELEASE_BUILD /DNDEBUG /I%OLD_DIR%/%PROTOBUF_X86_DIRECTORY%\include\ %OLD_DIR%/dll/*.cpp %OLD_DIR%/dll/*.cc "%OLD_DIR%/%PROTOBUF_X86_LIBRARY%" Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib /EHsc /MP12 /Ox /link /debug:none /OUT:%OLD_DIR%\release\steam_api.dll
cd %OLD_DIR%
endlocal

setlocal
"%PROTOC_X64_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x64.bat
cd build\release\x64
cl %OLD_DIR%/dll/rtlgenrandom.c %OLD_DIR%/dll/rtlgenrandom.def
cl /LD /DEMU_RELEASE_BUILD /DNDEBUG /I%OLD_DIR%/%PROTOBUF_X64_DIRECTORY%\include\ %OLD_DIR%/dll/*.cpp %OLD_DIR%/dll/*.cc "%OLD_DIR%/%PROTOBUF_X64_LIBRARY%" Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib /EHsc /MP12 /Ox /link /debug:none /OUT:%OLD_DIR%\release\steam_api64.dll
cd %OLD_DIR%
endlocal
copy Readme_release.txt release\Readme.txt
xcopy /s files_example\* release\
call build_win_release_experimental.bat
call build_win_release_experimental_steamclient.bat
call build_win_lobby_connect.bat
call build_win_find_interfaces.bat
