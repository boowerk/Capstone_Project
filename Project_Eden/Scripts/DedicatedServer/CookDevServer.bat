@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"
set "ARCHIVE_DIR=%PROJECT_ROOT%Saved\DedicatedServer"

rem Optional first argument: engine root folder that contains the Engine directory.
rem Example: CookDevServer.bat "C:\Engine\Windows"
if not "%~1"=="" (
    if exist "%~1\Engine\Build\BatchFiles\RunUAT.bat" (
        set "UE_SERVER_ROOT=%~1"
        shift /1
    )
)

if "%UE_SERVER_ROOT%"=="" (
    for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ResolveEngineRoot.ps1" -UProject "%UPROJECT%" -Kind Build`) do set "UE_SERVER_ROOT=%%I"
)

if "%UE_SERVER_ROOT%"=="" (
    echo RunUAT.bat was not found from the ProjectEden_Engine registry entry.
    echo Register the project engine association or run:
    echo %~nx0 "C:\Path\To\Your\EngineRoot"
    pause
    exit /b 1
)

set "RUN_UAT=%UE_SERVER_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
if not exist "%RUN_UAT%" (
    echo RunUAT.bat not found: %RUN_UAT%
    pause
    exit /b 1
)

echo Cooking and staging Project_EdenServer with engine root: %UE_SERVER_ROOT%
echo Output: %ARCHIVE_DIR%
echo This script uses the already-built Project_EdenServer.exe.
echo If server code changed, run BuildDevServer.bat before this script.
call "%RUN_UAT%" BuildCookRun -project="%UPROJECT%" -noP4 -server -noclient -serverplatform=Win64 -serverconfig=Development -nocompile -nocompileeditor -cook -stage -package -pak -archive -archivedirectory="%ARCHIVE_DIR%" -map=/Game/Maps/MainMap/LobbyMap+/Game/Maps/DemoMap/ServerTest+/Game/Maps/DemoMap/ServerEmptyTest -utf8output %*
pause
exit /b %ERRORLEVEL%
