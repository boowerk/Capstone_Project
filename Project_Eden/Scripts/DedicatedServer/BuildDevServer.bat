@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"

rem Optional first argument: engine root folder that contains the Engine directory.
rem Example: BuildDevServer.bat "C:\Engine_server\Windows"
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
    echo Build.bat was not found in the common engine paths.
    echo Enter your engine root folder. It must contain: Engine\Build\BatchFiles\Build.bat
    echo.
    set /p "UE_SERVER_ROOT=Engine root: "
)

set "BUILD_BAT=%UE_SERVER_ROOT%\Engine\Build\BatchFiles\Build.bat"
if not exist "%BUILD_BAT%" (
    echo Build.bat not found: %BUILD_BAT%
    echo.
    echo You can also run:
    echo %~nx0 "C:\Path\To\Your\EngineRoot"
    pause
    exit /b 1
)

rem SHIFT does not change %%* in a batch file. Collect the remaining arguments
rem explicitly so an engine-root argument is never forwarded to UnrealBuildTool.
set "EXTRA_ARGS="
:CollectExtraArgs
if "%~1"=="" goto ExtraArgsDone
set "EXTRA_ARGS=%EXTRA_ARGS% %1"
shift /1
goto CollectExtraArgs

:ExtraArgsDone
echo Building Project_EdenServer with engine root: %UE_SERVER_ROOT%
call "%BUILD_BAT%" Project_EdenServer Win64 Development -Project="%UPROJECT%" -WaitMutex -FromMsBuild -architecture=x64 %EXTRA_ARGS%
set "BUILD_EXIT_CODE=%ERRORLEVEL%"
if not "%BUILD_EXIT_CODE%"=="0" (
    echo Server build failed with exit code %BUILD_EXIT_CODE%.
)
pause
exit /b %BUILD_EXIT_CODE%
