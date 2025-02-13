@echo off
cd /d "%~dp0"

SET OLD_DIR=%cd%

IF NOT "%1" == "" ( SET JOB_COUNT=%~1 )

IF NOT DEFINED BUILT_ALL_DEPS ( call generate_all_deps.bat )

IF EXIST build\debug\lobby_connect\x86\*.* ( DEL /F /S /Q build\debug\lobby_connect\x86\*.* )
IF EXIST build\debug\lobby_connect\x64\*.* ( DEL /F /S /Q build\debug\lobby_connect\x64\*.* )

IF EXIST debug\lobby_connect\*.* ( DEL /F /S /Q debug\lobby_connect\*.* )

setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
cd %OLD_DIR%\build\lobby_connect\debug\x86

cl /c @%CDS_DIR%\DEBUG.BLD @%CDS_DIR%\PROTOBUF_X86.BLD @%CDS_DIR%\LOBBY_CONNECT.BLD
IF EXIST %CDS_DIR%\DEBUG_LOBBY_CONNECT_X86.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_LOBBY_CONNECT_X86.LKS )
where "*.obj" > %CDS_DIR%\DEBUG_LOBBY_CONNECT_X86.LKS
echo /link /OUT:%OLD_DIR%\debug\lobby_connect\lobby_connect_x32.exe >> %CDS_DIR%\DEBUG_LOBBY_CONNECT_X86.LKS
echo /link /IMPLIB:%cd%\lobby_connect_x32.lib >> %CDS_DIR%\DEBUG_LOBBY_CONNECT_X86.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\lobby_connect\lobby_connect_x32.pdb >> %CDS_DIR%\DEBUG_LOBBY_CONNECT_X86.LKS )

cl @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\PROTOBUF_X86.BLD @%CDS_DIR%\PROTOBUF_X86.LKS @%CDS_DIR%\LOBBY_CONNECT.LKS @%CDS_DIR%\DEBUG_LOBBY_CONNECT_X86.LKS
cd %OLD_DIR%
endlocal

setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x64.bat
cd %OLD_DIR%\build\lobby_connect\debug\x64

cl /c @%CDS_DIR%\DEBUG.BLD @%CDS_DIR%\PROTOBUF_X64.BLD @%CDS_DIR%\LOBBY_CONNECT.BLD
IF EXIST %CDS_DIR%\DEBUG_LOBBY_CONNECT_X64.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_LOBBY_CONNECT_X64.LKS )
where "*.obj" > %CDS_DIR%\DEBUG_LOBBY_CONNECT_X64.LKS
echo /link /OUT:%OLD_DIR%\debug\lobby_connect\lobby_connect_x64.exe >> %CDS_DIR%\DEBUG_LOBBY_CONNECT_X64.LKS
echo /link /IMPLIB:%cd%\lobby_connect_x64.lib >> %CDS_DIR%\DEBUG_LOBBY_CONNECT_X64.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\lobby_connect\lobby_connect_x64.pdb >> %CDS_DIR%\DEBUG_LOBBY_CONNECT_X64.LKS )

cl @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\PROTOBUF_X64.BLD @%CDS_DIR%\PROTOBUF_X64.LKS @%CDS_DIR%\LOBBY_CONNECT.LKS @%CDS_DIR%\DEBUG_LOBBY_CONNECT_X64.LKS
cd %OLD_DIR%
endlocal
copy Readme_lobby_connect.txt debug\lobby_connect\Readme.txt
