@echo off
setlocal

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"

if "%UE_SERVER_ROOT%"=="" set "UE_SERVER_ROOT=C:\Engine_server\Windows"
set "BUILD_BAT=%UE_SERVER_ROOT%\Engine\Build\BatchFiles\Build.bat"

if not exist "%BUILD_BAT%" (
    echo Build.bat not found: %BUILD_BAT%
    echo Set UE_SERVER_ROOT to your installed engine root and try again.
    pause
    exit /b 1
)

call "%BUILD_BAT%" Project_EdenServer Win64 Development -Project="%UPROJECT%" -WaitMutex -FromMsBuild -architecture=x64
pause
