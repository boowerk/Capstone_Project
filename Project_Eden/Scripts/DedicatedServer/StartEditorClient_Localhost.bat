@echo off
setlocal

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"
set "CLIENT_URL=127.0.0.1:7778"
set "CLIENT_WINDOW_ARGS=-windowed -ResX=960 -ResY=540 -WinX=60 -WinY=60"

if "%UE_SERVER_ROOT%"=="" set "UE_SERVER_ROOT=C:\Engine_server\Windows"
set "EDITOR_EXE=%UE_SERVER_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%EDITOR_EXE%" (
    echo UnrealEditor.exe not found: %EDITOR_EXE%
    echo Set UE_SERVER_ROOT to your installed engine root and try again.
    pause
    exit /b 1
)

"%EDITOR_EXE%" "%UPROJECT%" %CLIENT_URL% -game -log %CLIENT_WINDOW_ARGS% %*
pause
