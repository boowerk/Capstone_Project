@echo off
setlocal

set "PROJECT_ROOT=%~dp0..\..\"
set "SERVER_EXE=%PROJECT_ROOT%Binaries\Win64\Project_EdenServer.exe"

if not exist "%SERVER_EXE%" (
    echo Server exe not found: %SERVER_EXE%
    echo Run BuildDevServer.bat first.
    pause
    exit /b 1
)

pushd "%PROJECT_ROOT%"
"%SERVER_EXE%" -log -port=7778
popd
pause
