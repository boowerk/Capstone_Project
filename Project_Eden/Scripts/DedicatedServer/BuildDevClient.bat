@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"

rem Optional first argument: engine root folder that contains the Engine directory.
rem Example: BuildDevClient.bat "C:\Engine_server\Windows"
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
    echo Build.bat was not found from the ProjectEden_Engine registry entry.
    echo Register the project engine association or run:
    echo %~nx0 "C:\Path\To\Your\EngineRoot"
    pause
    exit /b 1
)

set "BUILD_BAT=%UE_SERVER_ROOT%\Engine\Build\BatchFiles\Build.bat"
if not exist "%BUILD_BAT%" (
    echo Build.bat not found: %BUILD_BAT%
    pause
    exit /b 1
)

set "PCGEX_GAME_MANIFEST=%UE_SERVER_ROOT%\Engine\Plugins\Marketplace\PCGExtendedToolkit\Intermediate\Build\Win64\x64\UnrealGame\Development\PCGExCore\PCGExCore.precompiled"
set "PROJECT_PCGEX=%PROJECT_ROOT%Plugins\PCGExtendedToolkit\PCGExtendedToolkit.uplugin"
if not exist "%PCGEX_GAME_MANIFEST%" if not exist "%PROJECT_PCGEX%" (
    echo Installed PCGExtendedToolkit has no UnrealGame manifest.
    echo Preparing the ignored project-local source copy.
    call "%~dp0PreparePCGExClientBuild.bat" "%UE_SERVER_ROOT%"
    if errorlevel 1 exit /b 1
)

set "EXTRA_ARGS="
:CollectExtraArgs
if "%~1"=="" goto ExtraArgsDone
set "EXTRA_ARGS=%EXTRA_ARGS% %~1"
shift /1
goto CollectExtraArgs

:ExtraArgsDone
echo Building Project_Eden game client with engine root: %UE_SERVER_ROOT%
call "%BUILD_BAT%" Project_Eden Win64 Development -Project="%UPROJECT%" -WaitMutex -FromMsBuild -architecture=x64 %EXTRA_ARGS%
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" (
    echo Client build failed with exit code %RESULT%.
    pause
    exit /b %RESULT%
)

rem Cook runs through UnrealEditor-Cmd. When the installed PCGEx plugin lacks
rem Game manifests and a project-local source copy is used, its Editor modules
rem must also be built so the cook host can load the same plugin.
if exist "%PROJECT_PCGEX%" (
    echo Building Project_EdenEditor cook host for project-local PCGExtendedToolkit.
    call "%BUILD_BAT%" Project_EdenEditor Win64 Development -Project="%UPROJECT%" -WaitMutex -FromMsBuild -architecture=x64 %EXTRA_ARGS%
    set "RESULT=%ERRORLEVEL%"
    if not "%RESULT%"=="0" (
        echo Client cook-host Editor build failed with exit code %RESULT%.
        pause
        exit /b %RESULT%
    )
)

echo Client build succeeded.
pause
exit /b 0
