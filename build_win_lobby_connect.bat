@echo off
cd /d "%~dp0"

IF NOT EXIST build ( mkdir build )
IF NOT EXIST build\release ( mkdir build\release )
IF NOT EXIST build\release\lobby_connect ( mkdir build\release\lobby_connect )
IF NOT EXIST build\release\lobby_connect\x86 ( mkdir build\release\lobby_connect\x86 )
IF EXIST build\release\lobby_connect\x86\*.* ( DEL /F /S /Q build\release\lobby_connect\x86\*.* )

IF NOT EXIST release\lobby_connect ( mkdir release\lobby_connect )
IF EXIST release\lobby_connect\*.* ( DEL /F /S /Q release\lobby_connect\*.* )

call build_set_protobuf_directories.bat
setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
SET OLD_DIR=%cd%
cd build\release\lobby_connect\x86
cl %OLD_DIR%/dll/rtlgenrandom.c %OLD_DIR%/dll/rtlgenrandom.def
cl /DNO_DISK_WRITES /DLOBBY_CONNECT /DEMU_RELEASE_BUILD /DNDEBUG /I%OLD_DIR%/%PROTOBUF_X86_DIRECTORY%\include\ %OLD_DIR%/lobby_connect.cpp %OLD_DIR%/dll/*.cpp %OLD_DIR%/dll/*.cc "%OLD_DIR%/%PROTOBUF_X86_LIBRARY%" Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Comdlg32.lib /EHsc /MP12 /Ox /link /debug:none /OUT:%OLD_DIR%\release\lobby_connect\lobby_connect.exe
cd %OLD_DIR%
copy Readme_lobby_connect.txt release\lobby_connect\Readme.txt
endlocal
