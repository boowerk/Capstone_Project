@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"
set "PACKAGED_SERVER_EXE=%PROJECT_ROOT%Saved\DedicatedServer\WindowsServer\Project_EdenServer.exe"
set "PACKAGED_SERVER_EXE_ALT=%PROJECT_ROOT%Saved\DedicatedServer\WindowsServer\Project_Eden\Binaries\Win64\Project_EdenServer.exe"
set "STAGED_SERVER_EXE=%PROJECT_ROOT%Saved\StagedBuilds\WindowsServer\Project_EdenServer.exe"
set "STAGED_SERVER_EXE_ALT=%PROJECT_ROOT%Saved\StagedBuilds\WindowsServer\Project_Eden\Binaries\Win64\Project_EdenServer.exe"
set "LOCAL_SERVER_EXE=%PROJECT_ROOT%Binaries\Win64\Project_EdenServer.exe"

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

if exist "%PACKAGED_SERVER_EXE%" (
    set "SERVER_EXE=%PACKAGED_SERVER_EXE%"
) else if exist "%PACKAGED_SERVER_EXE_ALT%" (
    set "SERVER_EXE=%PACKAGED_SERVER_EXE_ALT%"
) else if exist "%STAGED_SERVER_EXE%" (
    set "SERVER_EXE=%STAGED_SERVER_EXE%"
) else if exist "%STAGED_SERVER_EXE_ALT%" (
    set "SERVER_EXE=%STAGED_SERVER_EXE_ALT%"
) else (
    set "SERVER_EXE=%LOCAL_SERVER_EXE%"
    echo Packaged server exe was not found:
    echo %PACKAGED_SERVER_EXE%
    echo %PACKAGED_SERVER_EXE_ALT%
    echo %STAGED_SERVER_EXE%
    echo %STAGED_SERVER_EXE_ALT%
    echo.
    echo Falling back to local build exe. If it crashes while loading AssetRegistry,
    echo run CookDevServer.bat first and start this script again.
    echo.
)

if not exist "%SERVER_EXE%" (
    echo Server exe not found: %SERVER_EXE%
    echo Run CookDevServer.bat first with this engine root:
    echo %UE_SERVER_ROOT%
    pause
    exit /b 1
)

set "PATH=%UE_SERVER_ROOT%\Engine\Binaries\Win64;%PATH%"
echo Starting Project_EdenServer with engine root: %UE_SERVER_ROOT%
echo Server exe: %SERVER_EXE%
pushd "%PROJECT_ROOT%"
"%SERVER_EXE%" -log -port=7778
popd
pause
