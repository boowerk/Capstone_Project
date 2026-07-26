@echo off
setlocal EnableExtensions
title Project Eden Dedicated Server - Local - Port 7778

for %%I in ("%~dp0..\..") do set "PROJECT_ROOT=%%~fI\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"
set "LOCAL_SERVER_CMD_EXE=%PROJECT_ROOT%Binaries\Win64\Project_EdenServer-Cmd.exe"
set "LOCAL_SERVER_EXE=%PROJECT_ROOT%Binaries\Win64\Project_EdenServer.exe"
set "PACKAGED_SERVER_CMD_EXE=%PROJECT_ROOT%Saved\DedicatedServer\WindowsServer\Project_EdenServer-Cmd.exe"
set "PACKAGED_SERVER_CMD_EXE_ALT=%PROJECT_ROOT%Saved\DedicatedServer\WindowsServer\Project_Eden\Binaries\Win64\Project_EdenServer-Cmd.exe"
set "PACKAGED_SERVER_EXE=%PROJECT_ROOT%Saved\DedicatedServer\WindowsServer\Project_EdenServer.exe"
set "PACKAGED_SERVER_EXE_ALT=%PROJECT_ROOT%Saved\DedicatedServer\WindowsServer\Project_Eden\Binaries\Win64\Project_EdenServer.exe"
set "STAGED_SERVER_CMD_EXE=%PROJECT_ROOT%Saved\StagedBuilds\WindowsServer\Project_EdenServer-Cmd.exe"
set "STAGED_SERVER_CMD_EXE_ALT=%PROJECT_ROOT%Saved\StagedBuilds\WindowsServer\Project_Eden\Binaries\Win64\Project_EdenServer-Cmd.exe"
set "STAGED_SERVER_EXE=%PROJECT_ROOT%Saved\StagedBuilds\WindowsServer\Project_EdenServer.exe"
set "STAGED_SERVER_EXE_ALT=%PROJECT_ROOT%Saved\StagedBuilds\WindowsServer\Project_Eden\Binaries\Win64\Project_EdenServer.exe"

rem Optional first argument: engine root folder that contains the Engine directory.
rem Example: StartDevServer_LocalExe.bat "C:\Engine\Windows"
if not "%~1"=="" (
    if exist "%~1\Engine\Build\BatchFiles\Build.bat" (
        set "UE_SERVER_ROOT=%~1"
        shift /1
    )
)

if "%UE_SERVER_ROOT%"=="" (
    for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ResolveEngineRoot.ps1" -UProject "%UPROJECT%" -Kind Build`) do set "UE_SERVER_ROOT=%%I"
)

if "%UE_SERVER_ROOT%"=="" (
    echo Engine root was not found from ProjectEden_Engine registry entry.
    echo Register the project engine association or run:
    echo %~nx0 "C:\Path\To\Your\EngineRoot"
    pause
    exit /b 1
)

if exist "%LOCAL_SERVER_CMD_EXE%" (
    set "SERVER_EXE=%LOCAL_SERVER_CMD_EXE%"
) else if exist "%LOCAL_SERVER_EXE%" (
    set "SERVER_EXE=%LOCAL_SERVER_EXE%"
) else if exist "%PACKAGED_SERVER_CMD_EXE%" (
    set "SERVER_EXE=%PACKAGED_SERVER_CMD_EXE%"
) else if exist "%PACKAGED_SERVER_CMD_EXE_ALT%" (
    set "SERVER_EXE=%PACKAGED_SERVER_CMD_EXE_ALT%"
) else if exist "%PACKAGED_SERVER_EXE%" (
    set "SERVER_EXE=%PACKAGED_SERVER_EXE%"
) else if exist "%PACKAGED_SERVER_EXE_ALT%" (
    set "SERVER_EXE=%PACKAGED_SERVER_EXE_ALT%"
) else if exist "%STAGED_SERVER_CMD_EXE%" (
    set "SERVER_EXE=%STAGED_SERVER_CMD_EXE%"
) else if exist "%STAGED_SERVER_CMD_EXE_ALT%" (
    set "SERVER_EXE=%STAGED_SERVER_CMD_EXE_ALT%"
) else if exist "%STAGED_SERVER_EXE%" (
    set "SERVER_EXE=%STAGED_SERVER_EXE%"
) else if exist "%STAGED_SERVER_EXE_ALT%" (
    set "SERVER_EXE=%STAGED_SERVER_EXE_ALT%"
) else (
    echo Server exe not found:
    echo %LOCAL_SERVER_CMD_EXE%
    echo %LOCAL_SERVER_EXE%
    echo %PACKAGED_SERVER_CMD_EXE%
    echo %PACKAGED_SERVER_CMD_EXE_ALT%
    echo %PACKAGED_SERVER_EXE%
    echo %PACKAGED_SERVER_EXE_ALT%
    echo %STAGED_SERVER_CMD_EXE%
    echo %STAGED_SERVER_CMD_EXE_ALT%
    echo %STAGED_SERVER_EXE%
    echo %STAGED_SERVER_EXE_ALT%
    echo.
    echo Run BuildDevServer.bat first for C++-only server testing.
    echo Run CookDevServer.bat if server needs newly cooked assets.
    pause
    exit /b 1
)

set "PATH=%UE_SERVER_ROOT%\Engine\Binaries\Win64;%PATH%"
set "SERVER_WINDOW_ARG="
if /I not "%SERVER_EXE:~-8%"=="-Cmd.exe" set "SERVER_WINDOW_ARG=-log"
set "SERVER_LOG=%PROJECT_ROOT%Saved\Cooked\WindowsServer\Project_Eden\Saved\Logs\Project_EdenServer.log"
echo(%SERVER_EXE%| findstr /I /C:"\Saved\DedicatedServer\WindowsServer\" >nul
if not errorlevel 1 set "SERVER_LOG=%PROJECT_ROOT%Saved\DedicatedServer\WindowsServer\Project_Eden\Saved\Logs\Project_EdenServer.log"
echo(%SERVER_EXE%| findstr /I /C:"\Saved\StagedBuilds\WindowsServer\" >nul
if not errorlevel 1 set "SERVER_LOG=%PROJECT_ROOT%Saved\StagedBuilds\WindowsServer\Project_Eden\Saved\Logs\Project_EdenServer.log"
for %%I in ("%SERVER_LOG%") do if not exist "%%~dpI" mkdir "%%~dpI" >nul 2>nul
cls
echo Starting Project_EdenServer with engine root: %UE_SERVER_ROOT%
echo Server exe: %SERVER_EXE%
echo Live log: %SERVER_LOG%
echo Close this window or press Ctrl+C to stop the server.
echo Local exe is preferred by this script. Cook assets only when content changed.
echo.
pushd "%PROJECT_ROOT%"
"%SERVER_EXE%" %SERVER_WINDOW_ARG% -LOG=Project_EdenServer.log -stdout -FullStdOutLogOutput -forcelogflush -port=7778
set "SERVER_EXIT_CODE=%ERRORLEVEL%"
popd
echo.
echo Project_EdenServer exited with code %SERVER_EXIT_CODE%.
echo Full log: %SERVER_LOG%
pause
exit /b %SERVER_EXIT_CODE%
