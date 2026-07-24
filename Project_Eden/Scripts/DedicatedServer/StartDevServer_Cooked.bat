@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"
set "COOKED_SERVER_RESOLVER=%~dp0ResolveCookedServer.ps1"

rem Optional first argument: engine root folder that contains the Engine directory.
rem Example: StartDevServer_Cooked.bat "C:\Engine\Windows"
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

if not exist "%COOKED_SERVER_RESOLVER%" (
    echo Cooked server resolver not found: %COOKED_SERVER_RESOLVER%
    pause
    exit /b 1
)

for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%COOKED_SERVER_RESOLVER%" -ProjectRoot "%PROJECT_ROOT%"`) do set "SERVER_EXE=%%I"
if "%SERVER_EXE%"=="" (
    echo.
    echo No current cooked server package is safe to start.
    echo Run BuildDevServer.bat and CookDevServer.bat first.
    pause
    exit /b 1
)

set "PATH=%UE_SERVER_ROOT%\Engine\Binaries\Win64;%PATH%"
echo Starting Project_EdenServer with engine root: %UE_SERVER_ROOT%
echo Server exe: %SERVER_EXE%
echo Solo debug start: enabled
pushd "%PROJECT_ROOT%"
"%SERVER_EXE%" -log -port=7778 -AllowLobbyForceStart
popd
pause
