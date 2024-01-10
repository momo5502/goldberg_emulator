@echo off
cd /d "%~dp0"
mkdir debug\lobby_connect
del /Q debug\lobby_connect\*
call build_set_protobuf_directories.bat
setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
cl dll/rtlgenrandom.c dll/rtlgenrandom.def
cl /DNO_DISK_WRITES /DLOBBY_CONNECT /DEMU_RELEASE_BUILD /I%PROTOBUF_X86_DIRECTORY%\include\ lobby_connect.cpp dll/*.cpp dll/*.cc "%PROTOBUF_X86_LIBRARY%" Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Comdlg32.lib /EHsc /MP12 /Ox /link /OUT:debug\lobby_connect\lobby_connect.exe
copy Readme_lobby_connect.txt debug\lobby_connect\Readme.txt
endlocal
