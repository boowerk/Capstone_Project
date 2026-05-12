@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"
set "SERVER_PORT=7778"
set "CLIENT_WINDOW_ARGS=-windowed -ResX=960 -ResY=540 -WinX=60 -WinY=60"

if "%~1"=="" (
    echo Enter the host PC's Radmin VPN IP.
    echo Example: 26.123.45.67
    echo.
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

rem Optional second argument: engine root folder that contains the Engine directory.
rem Example: StartEditorClient_Radmin.bat 26.123.45.67 "C:\Engine_server\Windows"
if not "%~1"=="" (
    if exist "%~1\Engine\Binaries\Win64\UnrealEditor.exe" (
        set "UE_SERVER_ROOT=%~1"
        shift /1
    )
)

if "%UE_SERVER_ROOT%"=="" call :TryEngineRoot "C:\Engine_server\Windows"
if "%UE_SERVER_ROOT%"=="" call :TryEngineRoot "C:\Program Files\Epic Games\UE_5.7"
if "%UE_SERVER_ROOT%"=="" call :TryEngineRoot "C:\Program Files\Epic Games\UE_5.6"
if "%UE_SERVER_ROOT%"=="" call :TryEngineRoot "D:\Engine_server\Windows"
if "%UE_SERVER_ROOT%"=="" call :TryEngineRoot "D:\Epic Games\UE_5.7"
if "%UE_SERVER_ROOT%"=="" call :TryEngineRoot "D:\Epic Games\UE_5.6"

if "%UE_SERVER_ROOT%"=="" (
    echo UnrealEditor.exe was not found in the common engine paths.
    echo Enter your engine root folder. It must contain: Engine\Binaries\Win64\UnrealEditor.exe
    echo.
    set /p "UE_SERVER_ROOT=Engine root: "
)

set "EDITOR_EXE=%UE_SERVER_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%EDITOR_EXE%" (
    echo UnrealEditor.exe not found: %EDITOR_EXE%
    echo.
    echo You can also run:
    echo %~nx0 %SERVER_IP% "C:\Path\To\Your\EngineRoot"
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

echo Starting Project Eden client: %CLIENT_URL%
echo Engine root: %UE_SERVER_ROOT%
start "Project Eden Radmin Client" "%EDITOR_EXE%" "%UPROJECT%" %CLIENT_URL% -game -log %CLIENT_WINDOW_ARGS% %EXTRA_ARGS%
exit /b 0

:TryEngineRoot
if exist "%~1\Engine\Binaries\Win64\UnrealEditor.exe" set "UE_SERVER_ROOT=%~1"
exit /b 0
