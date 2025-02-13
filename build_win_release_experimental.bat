@echo off
cd /d "%~dp0"

IF NOT "%1" == "" ( SET JOB_COUNT=%~1 )

SET SKIP_EXPERIMENTAL_STEAMCLIENT_BUILD=1
SET SKIP_STEAMCLIENT_LOADER=1

call build_win_release_experimental_steamclient.bat
