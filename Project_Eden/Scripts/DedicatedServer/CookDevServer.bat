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
rem 로비가 동적으로 Landscape로 ServerTravel하므로 쿠커가 찾을 수 있도록 목적지 맵을 명시합니다.
set "COOK_MAPS=/Game/Maps/MainMap/LobbyMap+/Game/Maps/MainMap/L_LandscapeMap+/Game/WorldLayout/L_Village_00+/Game/WorldLayout/L_Village_01+/Game/WorldLayout/L_Village_02+/Game/WorldLayout/L_Village_03+/Game/Maps/DemoMap/ServerTest+/Game/Maps/DemoMap/ServerEmptyTest"
rem SHIFT does not change %%* in a batch file. Collect the remaining arguments
rem explicitly so an engine-root argument is never forwarded to RunUAT.
set "EXTRA_ARGS="
:CollectExtraArgs
if "%~1"=="" goto ExtraArgsDone
set "EXTRA_ARGS=%EXTRA_ARGS% %1"
shift /1
goto CollectExtraArgs

:ExtraArgsDone
echo Cook maps: %COOK_MAPS%
call "%RUN_UAT%" BuildCookRun -project="%UPROJECT%" -noP4 -server -noclient -serverplatform=Win64 -serverconfig=Development -nocompile -nocompileeditor -cook -stage -package -pak -archive -archivedirectory="%ARCHIVE_DIR%" -map=%COOK_MAPS% -utf8output %EXTRA_ARGS%
set "COOK_EXIT_CODE=%ERRORLEVEL%"
if not "%COOK_EXIT_CODE%"=="0" (
    echo Server cook failed with exit code %COOK_EXIT_CODE%.
    pause
    exit /b %COOK_EXIT_CODE%
)

set "COOK_STAMP_NAME=.project_eden_server_cook_complete"
set "ARCHIVE_STAMP=%ARCHIVE_DIR%\WindowsServer\%COOK_STAMP_NAME%"
set "STAGED_STAMP=%PROJECT_ROOT%Saved\StagedBuilds\WindowsServer\%COOK_STAMP_NAME%"

if exist "%ARCHIVE_DIR%\WindowsServer\" (
    >"%ARCHIVE_STAMP%" echo Project_Eden server cook completed successfully.
)
if exist "%PROJECT_ROOT%Saved\StagedBuilds\WindowsServer\" (
    >"%STAGED_STAMP%" echo Project_Eden server cook completed successfully.
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ResolveCookedServer.ps1" -ProjectRoot "%PROJECT_ROOT%"
set "VERIFY_EXIT_CODE=%ERRORLEVEL%"
if not "%VERIFY_EXIT_CODE%"=="0" (
    if exist "%ARCHIVE_STAMP%" del /q "%ARCHIVE_STAMP%"
    if exist "%STAGED_STAMP%" del /q "%STAGED_STAMP%"
    echo Server cook verification failed with exit code %VERIFY_EXIT_CODE%.
    pause
    exit /b %VERIFY_EXIT_CODE%
)

echo Server cook completed and contains BP_RunPortal.
pause
exit /b 0
