@echo off
cd /d "%~dp0"

SET OLD_DIR=%cd%

IF NOT "%1" == "" ( SET JOB_COUNT=%~1 )

IF NOT DEFINED BUILT_ALL_DEPS ( call generate_all_deps.bat )

IF EXIST build\experimental\steamclient\debug\x86\*.* ( DEL /F /S /Q build\experimental\steamclient\debug\x86\*.* )
IF EXIST build\experimental\steamclient\debug\x64\*.* ( DEL /F /S /Q build\experimental\steamclient\debug\x64\*.* )

IF EXIST debug\experimental_steamclient\*.* ( DEL /F /S /Q debug\experimental_steamclient\*.* )

setlocal

IF DEFINED SKIP_X86 GOTO LK_X64

"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat

REM Non-STEAMCLIENT_DLL debug sc_deps.
cd "%OLD_DIR%\build\experimental_steamclient\debug\x86\sc_deps"
cl /c @%CDS_DIR%\DEBUG.BLD @%CDS_DIR%\PROTOBUF_X86.BLD @%CDS_DIR%\SC_DEPS.BLD
IF EXIST %CDS_DIR%\DEBUG_SC_DEPS_X86.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_SC_DEPS_X86.LKS )
where "*.obj" > %CDS_DIR%\DEBUG_SC_DEPS_X86.LKS

IF DEFINED SKIP_EXPERIMENTAL_BUILD GOTO LK_EXP_STEAMCLIENT_DLL_X86

REM Link Non-STEAMCLIENT_DLL debug steam_api.dll.
cd "%OLD_DIR%\build\experimental\debug\x86\"
IF EXIST %CDS_DIR%\DEBUG_STEAMAPI_NON_X86.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_STEAMAPI_NON_X86.LKS )
echo /link /OUT:%OLD_DIR%\debug\experimental\steam_api.dll > %CDS_DIR%\DEBUG_STEAMAPI_NON_X86.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\experimental\steam_api.pdb >> %CDS_DIR%\DEBUG_STEAMAPI_NON_X86.LKS )
cl /LD @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\PROTOBUF_X86.LKS @%CDS_DIR%\EXPERIMENTAL.BLD @%CDS_DIR%\EXPERIMENTAL.LKS @%CDS_DIR%\DEBUG_ALL_DEPS_X86.LKS @%CDS_DIR%\DEBUG_SC_DEPS_X86.LKS @%CDS_DIR%\DEBUG_STEAMAPI_NON_X86.LKS

:LK_EXP_STEAMCLIENT_DLL_X86

IF DEFINED SKIP_EXPERIMENTAL_STEAMCLIENT_BUILD GOTO LK_STEAMCLIENT_DLL_X86

REM Link STEAMCLIENT_DLL debug steamclient.dll
cd "%OLD_DIR%\build\experimental_steamclient\debug\x86"
IF EXIST %CDS_DIR%\DEBUG_STEAMCLIENT_X86.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_STEAMCLIENT_X86.LKS )
echo /link /OUT:%OLD_DIR%\debug\experimental_steamclient\steamclient.dll > %CDS_DIR%\DEBUG_STEAMCLIENT_X86.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\experimental_steamclient\steamclient.pdb >> %CDS_DIR%\DEBUG_STEAMCLIENT_X86.LKS )
cl /LD @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\PROTOBUF_X86.LKS @%CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS @%CDS_DIR%\DEBUG_ALL_DEPS_X86.LKS @%CDS_DIR%\DEBUG_SC_DEPS_X86.LKS @%CDS_DIR%\DEBUG_STEAMCLIENT_X86.LKS

:LK_STEAMCLIENT_DLL_X86

IF DEFINED SKIP_EXPERIMENTAL_BUILD GOTO LK_STEAMCLIENT_LOADER_X86

REM Link Non-STEAMCLIENT_DLL debug steamclient.dll.
cd "%OLD_DIR%\build\experimental\debug\x86\"
IF EXIST %CDS_DIR%\DEBUG_STEAMCLIENT_NON_X86.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_STEAMCLIENT_NON_X86.LKS )
echo /link /OUT:%OLD_DIR%\debug\experimental\steamclient.dll > %CDS_DIR%\DEBUG_STEAMCLIENT_NON_X86.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\experimental\steamclient.pdb >> %CDS_DIR%\DEBUG_STEAMCLIENT_NON_X86.LKS )
cl /LD @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\STEAMCLIENT.BLD @%CDS_DIR%\DEBUG_STEAMCLIENT_NON_X86.LKS

:LK_STEAMCLIENT_LOADER_X86

IF DEFINED SKIP_STEAMCLIENT_LOADER GOTO LK_X64

REM Build steamclient_loader debug x86.
cd "%OLD_DIR%\build\experimental_steamclient\steamclient_loader\debug\x86"
cl /c @%CDS_DIR%\DEBUG.BLD @%CDS_DIR%\STEAMCLIENT_LOADER.BLD
IF EXIST %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X86.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X86.LKS )
where "*.obj" > %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X86.LKS
echo /link /OUT:%OLD_DIR%\debug\experimental_steamclient\steamclient_loader_x32.exe >> %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X86.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\experimental_steamclient\steamclient_loader_x32.pdb >> %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X86.LKS )
cl @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\STEAMCLIENT_LOADER.LKS @%CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X86.LKS
cd %OLD_DIR%

:LK_X64

endlocal

setlocal

IF DEFINED SKIP_X64 GOTO LK_END

"%PROTOC_X64_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x64.bat

REM Non-STEAMCLIENT_DLL debug sc_deps.
cd "%OLD_DIR%\build\experimental_steamclient\debug\x64\sc_deps"
cl /c @%CDS_DIR%\DEBUG.BLD @%CDS_DIR%\PROTOBUF_X64.BLD @%CDS_DIR%\SC_DEPS.BLD
IF EXIST %CDS_DIR%\DEBUG_SC_DEPS_X64.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_SC_DEPS_X64.LKS )
where "*.obj" > %CDS_DIR%\DEBUG_SC_DEPS_X64.LKS

IF DEFINED SKIP_EXPERIMENTAL_BUILD GOTO LK_EXP_STEAMCLIENT_DLL_X64

REM Link Non-STEAMCLIENT_DLL debug steam_api64.dll.
cd "%OLD_DIR%\build\experimental\debug\x64\"
IF EXIST %CDS_DIR%\DEBUG_STEAMAPI_NON_X64.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_STEAMAPI_NON_X64.LKS )
echo /link /OUT:%OLD_DIR%\debug\experimental\steam_api64.dll > %CDS_DIR%\DEBUG_STEAMAPI_NON_X64.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\experimental\steam_api64.pdb >> %CDS_DIR%\DEBUG_STEAMAPI_NON_X64.LKS )
cl /LD @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\PROTOBUF_X64.LKS @%CDS_DIR%\EXPERIMENTAL.BLD @%CDS_DIR%\EXPERIMENTAL.LKS @%CDS_DIR%\DEBUG_ALL_DEPS_X64.LKS @%CDS_DIR%\DEBUG_SC_DEPS_X64.LKS @%CDS_DIR%\DEBUG_STEAMAPI_NON_X64.LKS

:LK_EXP_STEAMCLIENT_DLL_X64

IF DEFINED SKIP_EXPERIMENTAL_STEAMCLIENT_BUILD GOTO LK_STEAMCLIENT_DLL_X64

REM Link STEAMCLIENT_DLL debug steamclient64.dll
cd "%OLD_DIR%\build\experimental_steamclient\debug\x64"
IF EXIST %CDS_DIR%\DEBUG_STEAMCLIENT_X64.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_STEAMCLIENT_X64.LKS )
echo /link /OUT:%OLD_DIR%\debug\experimental_steamclient\steamclient64.dll > %CDS_DIR%\DEBUG_STEAMCLIENT_X64.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\experimental_steamclient\steamclient64.pdb >> %CDS_DIR%\DEBUG_STEAMCLIENT_X64.LKS )
cl /LD @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\PROTOBUF_X64.LKS @%CDS_DIR%\EXPERIMENTAL_STEAMCLIENT.LKS @%CDS_DIR%\DEBUG_ALL_DEPS_X64.LKS @%CDS_DIR%\DEBUG_SC_DEPS_X64.LKS @%CDS_DIR%\DEBUG_STEAMCLIENT_X64.LKS

:LK_STEAMCLIENT_DLL_X64

IF DEFINED SKIP_EXPERIMENTAL_BUILD GOTO LK_STEAMCLIENT_LOADER_X64

REM Link Non-STEAMCLIENT_DLL debug steamclient64.dll.
cd "%OLD_DIR%\build\experimental\debug\x64\"
IF EXIST %CDS_DIR%\DEBUG_STEAMCLIENT_NON_X64.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_STEAMCLIENT_NON_X64.LKS )
echo /link /OUT:%OLD_DIR%\debug\experimental\steamclient64.dll > %CDS_DIR%\DEBUG_STEAMCLIENT_NON_X64.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\experimental\steamclient64.pdb >> %CDS_DIR%\DEBUG_STEAMCLIENT_NON_X64.LKS )
cl /LD @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\STEAMCLIENT.BLD @%CDS_DIR%\DEBUG_STEAMCLIENT_NON_X64.LKS

:LK_STEAMCLIENT_LOADER_X64

IF DEFINED SKIP_STEAMCLIENT_LOADER GOTO LK_END

REM Build steamclient_loader debug x64.
cd "%OLD_DIR%\build\experimental_steamclient\steamclient_loader\debug\x64"
cl /c @%CDS_DIR%\DEBUG.BLD @%CDS_DIR%\STEAMCLIENT_LOADER.BLD
IF EXIST %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X64.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X64.LKS )
where "*.obj" > %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X64.LKS
echo /link /OUT:%OLD_DIR%\debug\experimental_steamclient\steamclient_loader_x64.exe >> %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X64.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\debug\experimental_steamclient\steamclient_loader_x64.pdb >> %CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X64.LKS )
cl @%CDS_DIR%\DEBUG.LKS @%CDS_DIR%\STEAMCLIENT_LOADER.LKS @%CDS_DIR%\DEBUG_STEAMCLIENT_LOADER_X64.LKS
cd %OLD_DIR%

:LK_END

endlocal

copy Readme_experimental.txt debug\experimental\Readme.txt
copy steamclient_loader\ColdClientLoader.ini debug\experimental_steamclient\
copy Readme_experimental_steamclient.txt debug\experimental_steamclient\Readme.txt
