@echo off
cd /d "%~dp0"

IF NOT EXIST build ( mkdir build )
IF NOT EXIST build\release ( mkdir build\release )
IF NOT EXIST build\release\tools ( mkdir build\release\tools )
IF NOT EXIST build\release\tools\x86 ( mkdir build\release\tools\x86 )
IF EXIST build\release\tools\x86\*.* ( DEL /F /S /Q build\release\tools\x86\*.* )

IF NOT EXIST release\tools ( mkdir release\tools )
IF EXIST release\tools\*.* ( DEL /F /S /Q release\tools\*.* )

setlocal
call build_env_x86.bat
SET OLD_DIR=%cd%
cd build\release\tools\x86
cl %OLD_DIR%/generate_interfaces_file.cpp /EHsc /MP12 /Ox /link /debug:none /OUT:%OLD_DIR%\release\tools\generate_interfaces_file.exe
cd %OLD_DIR%
copy Readme_generate_interfaces.txt release\tools\Readme_generate_interfaces.txt
endlocal
