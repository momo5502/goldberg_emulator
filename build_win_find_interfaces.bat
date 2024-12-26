@echo off
cd /d "%~dp0"

SET OLD_DIR=%cd%

IF NOT "%1" == "" ( SET JOB_COUNT=%~1 )

IF NOT DEFINED BUILT_ALL_DEPS ( call generate_all_deps.bat )

IF EXIST build\tools\release\x86\*.* ( DEL /F /S /Q build\tools\release\x86\*.* )
IF EXIST build\tools\release\x64\*.* ( DEL /F /S /Q build\tools\release\x64\*.* )

IF EXIST release\tools\*.* ( DEL /F /S /Q release\tools\*.* )

setlocal
call build_env_x86.bat
cd %OLD_DIR%\build\tools\release\x86
cl /c @%CDS_DIR%\RELEASE.BLD @%CDS_DIR%\GENERATE_INTERFACES_FILE.BLD
IF EXIST %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X86.LKS ( DEL /F /Q %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X86.LKS )
where "generate_interfaces_file.obj" > %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X86.LKS
echo /link /OUT:%OLD_DIR%\release\tools\generate_interfaces_file_x32.exe >> %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X86.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\release\tools\generate_interfaces_file_x32.pdb >> %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X86.LKS )
cl @%CDS_DIR%\RELEASE.LKS @%CDS_DIR%\GENERATE_INTERFACES_FILE.LKS @%CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X86.LKS
cd %OLD_DIR%
endlocal

setlocal
call build_env_x64.bat
cd %OLD_DIR%\build\tools\release\x64
cl /c @%CDS_DIR%\RELEASE.BLD @%CDS_DIR%\GENERATE_INTERFACES_FILE.BLD
IF EXIST %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X64.LKS ( DEL /F /Q %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X64.LKS )
where "generate_interfaces_file.obj" > %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X64.LKS
echo /link /OUT:%OLD_DIR%\release\tools\generate_interfaces_file_x64.exe >> %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X64.LKS
IF NOT EXIST %OLD_DIR%\CI_BUILD.TAG ( echo /link /PDB:%OLD_DIR%\release\tools\generate_interfaces_file_x64.pdb >> %CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X64.LKS )
cl @%CDS_DIR%\RELEASE.LKS @%CDS_DIR%\GENERATE_INTERFACES_FILE.LKS @%CDS_DIR%\RELEASE_GENERATE_INTERFACES_FILE_X64.LKS
cd %OLD_DIR%
endlocal

copy Readme_generate_interfaces.txt release\tools\Readme_generate_interfaces.txt
