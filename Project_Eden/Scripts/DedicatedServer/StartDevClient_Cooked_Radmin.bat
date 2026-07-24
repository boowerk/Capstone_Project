@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "PACKAGED_CLIENT_EXE=%PROJECT_ROOT%Saved\DedicatedClient\Windows\Project_Eden.exe"
set "PACKAGED_CLIENT_EXE_ALT=%PROJECT_ROOT%Saved\DedicatedClient\Windows\Project_Eden\Binaries\Win64\Project_Eden.exe"
set "STAGED_CLIENT_EXE=%PROJECT_ROOT%Saved\StagedBuilds\Windows\Project_Eden.exe"
set "STAGED_CLIENT_EXE_ALT=%PROJECT_ROOT%Saved\StagedBuilds\Windows\Project_Eden\Binaries\Win64\Project_Eden.exe"
set "SERVER_PORT=7778"
set "CLIENT_WINDOW_ARGS=-windowed -ResX=960 -ResY=540 -WinX=60 -WinY=60"

if "%~1"=="" (
    echo Enter the host PC's Radmin VPN IP.
    echo Example: 26.123.45.67
    set /p "SERVER_IP=Server Radmin IP: "
) else (
    set "SERVER_IP=%~1"
    shift /1
)

if "%SERVER_IP%"=="" (
    echo Server Radmin IP is required.
    pause
    exit /b 1
)

if exist "%PACKAGED_CLIENT_EXE%" (
    set "CLIENT_EXE=%PACKAGED_CLIENT_EXE%"
) else if exist "%PACKAGED_CLIENT_EXE_ALT%" (
    set "CLIENT_EXE=%PACKAGED_CLIENT_EXE_ALT%"
) else if exist "%STAGED_CLIENT_EXE%" (
    set "CLIENT_EXE=%STAGED_CLIENT_EXE%"
) else if exist "%STAGED_CLIENT_EXE_ALT%" (
    set "CLIENT_EXE=%STAGED_CLIENT_EXE_ALT%"
) else (
    echo Packaged client executable was not found.
    echo Run BuildDevClient.bat and CookDevClient.bat first.
    pause
    exit /b 1
)

set "CLIENT_URL=%SERVER_IP%:%SERVER_PORT%"
set "EXTRA_ARGS="
:CollectExtraArgs
if "%~1"=="" goto ExtraArgsDone
set "EXTRA_ARGS=%EXTRA_ARGS% %~1"
shift /1
goto CollectExtraArgs

:ExtraArgsDone
echo Starting packaged Project Eden client: %CLIENT_URL%
echo Client exe: %CLIENT_EXE%
start "Project Eden Cooked Radmin Client" "%CLIENT_EXE%" %CLIENT_URL% -log %CLIENT_WINDOW_ARGS% %EXTRA_ARGS%
exit /b 0
