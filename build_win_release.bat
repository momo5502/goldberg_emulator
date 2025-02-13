@echo off
cd /d "%~dp0"

SET OLD_DIR=%cd%

IF NOT "%1" == "" ( SET JOB_COUNT=%~1 )

IF NOT DEFINED BUILT_ALL_DEPS ( call generate_all_deps.bat )

IF EXIST build\release\x86\*.* ( DEL /F /S /Q build\release\x86\*.* )
IF EXIST build\release\x64\*.* ( DEL /F /S /Q build\release\x64\*.* )

IF EXIST release\steam_settings.EXAMPLE ( DEL /F /S /Q release\steam_settings.EXAMPLE )
IF EXIST release\*.dll ( DEL /F /Q release\*.dll )
IF EXIST release\*.txt ( DEL /F /Q release\*.txt )

setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
cd %OLD_DIR%\build\release\x86

cl /c @%CDS_DIR%\RELEASE.BLD @%CDS_DIR%\PROTOBUF_X86.BLD @%CDS_DIR%\DLL_MAIN_CPP.BLD
IF EXIST %CDS_DIR%\RELEASE_BASE_DLL_X86.LKS ( DEL /F /Q %CDS_DIR%\RELEASE_BASE_DLL_X86.LKS )
where "*.obj" > %CDS_DIR%\RELEASE_BASE_DLL_X86.LKS
echo /link /OUT:%OLD_DIR%\release\steam_api.dll >> %CDS_DIR%\RELEASE_BASE_DLL_X86.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\release\steam_api.pdb >> %CDS_DIR%\RELEASE_BASE_DLL_X86.LKS )

cl /LD @%CDS_DIR%/RELEASE.LKS @%CDS_DIR%/PROTOBUF_X86.LKS @%CDS_DIR%/DLL_MAIN_CPP.LKS @%CDS_DIR%\RELEASE_BASE_DLL_X86.LKS
cd %OLD_DIR%
endlocal

setlocal
"%PROTOC_X64_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x64.bat
cd %OLD_DIR%\build\release\x64

cl /c @%CDS_DIR%/RELEASE.BLD @%CDS_DIR%/PROTOBUF_X64.BLD @%CDS_DIR%/DLL_MAIN_CPP.BLD
IF EXIST %CDS_DIR%\RELEASE_BASE_DLL_X64.LKS ( DEL /F /Q %CDS_DIR%\RELEASE_BASE_DLL_X64.LKS )
where "*.obj" > %CDS_DIR%\RELEASE_BASE_DLL_X64.LKS
echo /link /OUT:%OLD_DIR%\release\steam_api64.dll >> %CDS_DIR%\RELEASE_BASE_DLL_X64.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\release\steam_api64.pdb >> %CDS_DIR%\RELEASE_BASE_DLL_X64.LKS )

cl /LD @%CDS_DIR%/RELEASE.LKS @%CDS_DIR%/PROTOBUF_X64.LKS @%CDS_DIR%/DLL_MAIN_CPP.LKS @%CDS_DIR%\RELEASE_BASE_DLL_X64.LKS
cd %OLD_DIR%

endlocal
copy Readme_release.txt release\Readme.txt
xcopy /s files_example\* release\
call build_win_release_experimental_steamclient.bat
call build_win_lobby_connect.bat
call build_win_find_interfaces.bat
