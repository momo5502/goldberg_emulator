@echo off
cd /d "%~dp0"

call generate_build_win.bat

IF EXIST build\all_deps\debug\x86\*.* ( DEL /F /S /Q build\all_deps\debug\x86\*.* )
IF EXIST build\all_deps\debug\x64\*.* ( DEL /F /S /Q build\all_deps\debug\x64\*.* )
IF EXIST build\all_deps\release\x86\*.* ( DEL /F /S /Q build\all_deps\release\x86\*.* )
IF EXIST build\all_deps\release\x64\*.* ( DEL /F /S /Q build\all_deps\release\x64\*.* )

call build_set_protobuf_directories.bat

setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
SET OLD_DIR=%cd%
cd "build\all_deps\debug\x86"
cl /c @%CDS_DIR%/DEBUG.BLD @%CDS_DIR%/PROTOBUF_X86.BLD @%CDS_DIR%\ALL_DEPS.BLD
IF EXIST %CDS_DIR%\DEBUG_ALL_DEPS_X86.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_ALL_DEPS_X86.LKS )
where "*.obj" > %CDS_DIR%\DEBUG_ALL_DEPS_X86.LKS
cd %OLD_DIR%
endlocal

setlocal
"%PROTOC_X64_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x64.bat
SET OLD_DIR=%cd%
cd "build\all_deps\debug\x64"
cl /c @%CDS_DIR%/DEBUG.BLD @%CDS_DIR%/PROTOBUF_X64.BLD @%CDS_DIR%\ALL_DEPS.BLD
IF EXIST %CDS_DIR%\DEBUG_ALL_DEPS_X64.LKS ( DEL /F /Q %CDS_DIR%\DEBUG_ALL_DEPS_X64.LKS )
where "*.obj" > %CDS_DIR%\DEBUG_ALL_DEPS_X64.LKS
cd %OLD_DIR%
endlocal

setlocal
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
SET OLD_DIR=%cd%
cd "build\all_deps\release\x86"
cl /c @%CDS_DIR%/RELEASE.BLD @%CDS_DIR%/PROTOBUF_X86.BLD @%CDS_DIR%\ALL_DEPS.BLD
IF EXIST %CDS_DIR%\RELEASE_ALL_DEPS_X86.LKS ( DEL /F /Q %CDS_DIR%\RELEASE_ALL_DEPS_X86.LKS )
where "*.obj" > %CDS_DIR%\RELEASE_ALL_DEPS_X86.LKS
cd %OLD_DIR%
endlocal

setlocal
"%PROTOC_X64_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x64.bat
SET OLD_DIR=%cd%
cd "build\all_deps\release\x64"
cl /c @%CDS_DIR%/RELEASE.BLD @%CDS_DIR%/PROTOBUF_X64.BLD @%CDS_DIR%\ALL_DEPS.BLD
IF EXIST %CDS_DIR%\RELEASE_ALL_DEPS_X64.LKS ( DEL /F /Q %CDS_DIR%\RELEASE_ALL_DEPS_X64.LKS )
where "*.obj" > %CDS_DIR%\RELEASE_ALL_DEPS_X64.LKS
cd %OLD_DIR%
endlocal

SET BUILT_ALL_DEPS=1
