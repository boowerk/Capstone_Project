@echo off
setlocal EnableExtensions EnableDelayedExpansion

for %%I in ("%~dp0..\..") do set "PROJECT_ROOT=%%~fI"
set "UPROJECT=%PROJECT_ROOT%\Project_Eden.uproject"
set "RESOLVE_ENGINE=%~dp0ResolveEngineRoot.ps1"
set "PREPARE_PCGEX=%~dp0PreparePCGExClientBuild.bat"

set "BUILD_CONFIG=Shipping"
set "ALLOW_DIRTY=0"
set "VALIDATE_ONLY=0"
set "PAUSE_AT_END=1"

:ParseArgs
if "%~1"=="" goto ArgsDone
if /I "%~1"=="--help" goto UsageSuccess
if /I "%~1"=="-h" goto UsageSuccess
if /I "%~1"=="--shipping" (
    set "BUILD_CONFIG=Shipping"
    shift /1
    goto ParseArgs
)
if /I "%~1"=="--development" (
    set "BUILD_CONFIG=Development"
    shift /1
    goto ParseArgs
)
if /I "%~1"=="--allow-dirty" (
    set "ALLOW_DIRTY=1"
    shift /1
    goto ParseArgs
)
if /I "%~1"=="--validate-only" (
    set "VALIDATE_ONLY=1"
    shift /1
    goto ParseArgs
)
if /I "%~1"=="--no-pause" (
    set "PAUSE_AT_END=0"
    shift /1
    goto ParseArgs
)
if exist "%~1\Engine\Build\BatchFiles\Build.bat" (
    set "UE_SERVER_ROOT=%~f1"
    shift /1
    goto ParseArgs
)

echo Unknown argument or invalid engine root: %~1
goto UsageFailure

:ArgsDone
if not exist "%UPROJECT%" (
    echo Project file was not found: %UPROJECT%
    exit /b 1
)

if not exist "%UE_SERVER_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    set "UE_SERVER_ROOT="
    for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%RESOLVE_ENGINE%" -UProject "%UPROJECT%" -Kind Build`) do set "UE_SERVER_ROOT=%%I"
)

set "BUILD_BAT=%UE_SERVER_ROOT%\Engine\Build\BatchFiles\Build.bat"
set "RUN_UAT=%UE_SERVER_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
if not exist "%BUILD_BAT%" (
    echo Build.bat was not found. Pass the engine root that contains the Engine directory.
    exit /b 1
)
if not exist "%RUN_UAT%" (
    echo RunUAT.bat was not found: %RUN_UAT%
    exit /b 1
)

set "PCGEX_GAME_MANIFEST=%UE_SERVER_ROOT%\Engine\Plugins\Marketplace\PCGExtendedToolkit\Intermediate\Build\Win64\x64\UnrealGame\%BUILD_CONFIG%\PCGExCore\PCGExCore.precompiled"
set "ENGINE_PCGEX=%UE_SERVER_ROOT%\Engine\Plugins\Marketplace\PCGExtendedToolkit\PCGExtendedToolkit.uplugin"
set "PROJECT_PCGEX=%PROJECT_ROOT%\Plugins\PCGExtendedToolkit\PCGExtendedToolkit.uplugin"
if not exist "%PCGEX_GAME_MANIFEST%" if not exist "%PROJECT_PCGEX%" (
    if not exist "%ENGINE_PCGEX%" (
        echo PCGExtendedToolkit source plugin was not found: %ENGINE_PCGEX%
        exit /b 1
    )
    if not exist "%PREPARE_PCGEX%" (
        echo PCGExtendedToolkit preparation script was not found: %PREPARE_PCGEX%
        exit /b 1
    )
)

set "GIT_SHA=nogit"
set "TREE_STATE=unknown"
where git >nul 2>nul
if not errorlevel 1 (
    for /f "delims=" %%I in ('git -C "%PROJECT_ROOT%" rev-parse --short^=8 HEAD 2^>nul') do set "GIT_SHA=%%I"
    set "TREE_STATE=clean"
    for /f "delims=" %%I in ('git -C "%PROJECT_ROOT%" status --porcelain --untracked-files^=normal 2^>nul') do set "TREE_STATE=dirty"
)

if /I "!TREE_STATE!"=="dirty" if "!ALLOW_DIRTY!"=="0" (
    echo.
    echo Final deployment stopped because the Git working tree is dirty:
    git -C "%PROJECT_ROOT%" status --short
    echo.
    echo Commit or stash the intended changes, or rerun with --allow-dirty.
    exit /b 2
)

for /f "delims=" %%I in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set "BUILD_STAMP=%%I"
set "CONFIG_LABEL=shipping"
if /I "%BUILD_CONFIG%"=="Development" set "CONFIG_LABEL=development"
set "RELEASE_NAME=%BUILD_STAMP%_%GIT_SHA%_%CONFIG_LABEL%"
if /I "!TREE_STATE!"=="dirty" set "RELEASE_NAME=!RELEASE_NAME!_dirty"

set "RELEASE_BASE=%PROJECT_ROOT%\Saved\FinalDeploy"
set "RELEASE_ROOT=%RELEASE_BASE%\!RELEASE_NAME!"
set "CLIENT_ARCHIVE=!RELEASE_ROOT!\Client"
set "SERVER_ARCHIVE=!RELEASE_ROOT!\Server"
set "CLIENT_OUTPUT=!CLIENT_ARCHIVE!\Windows"
set "SERVER_OUTPUT=!SERVER_ARCHIVE!\WindowsServer"

set "CLIENT_MAPS=/Game/Maps/MainMap/MainMenuMap+/Game/Maps/MainMap/LobbyMap+/Game/Maps/MainMap/L_LandscapeMap+/Game/WorldLayout/L_Village_00+/Game/WorldLayout/L_Village_01+/Game/WorldLayout/L_Village_02+/Game/WorldLayout/L_Village_03"
set "SERVER_MAPS=/Game/Maps/MainMap/LobbyMap+/Game/Maps/MainMap/L_LandscapeMap+/Game/WorldLayout/L_Village_00+/Game/WorldLayout/L_Village_01+/Game/WorldLayout/L_Village_02+/Game/WorldLayout/L_Village_03"

echo.
echo Project:       %UPROJECT%
echo Engine:        %UE_SERVER_ROOT%
echo Configuration: %BUILD_CONFIG%
echo Commit:        !GIT_SHA!
echo Source tree:   !TREE_STATE!
echo Deploy root:   !RELEASE_ROOT!
echo.

if "%VALIDATE_ONLY%"=="1" (
    echo Final deployment preflight validation succeeded.
    exit /b 0
)

if exist "!RELEASE_ROOT!" (
    echo Release directory already exists; nothing was overwritten:
    echo !RELEASE_ROOT!
    exit /b 1
)

mkdir "!RELEASE_ROOT!" >nul 2>nul
if errorlevel 1 (
    echo Failed to create release directory: !RELEASE_ROOT!
    exit /b 1
)

> "!RELEASE_ROOT!\BUILD_INCOMPLETE.txt" echo Build started at %DATE% %TIME%.

if not exist "%PCGEX_GAME_MANIFEST%" if not exist "%PROJECT_PCGEX%" (
    set "CURRENT_STEP=Prepare PCGExtendedToolkit source"
    call "%PREPARE_PCGEX%" "%UE_SERVER_ROOT%" --no-pause
    if errorlevel 1 (
        set "FAIL_CODE=!ERRORLEVEL!"
        goto BuildFailed
    )
)

set "CURRENT_STEP=Build %BUILD_CONFIG% client"
echo [1/5] !CURRENT_STEP!
call "%BUILD_BAT%" Project_Eden Win64 %BUILD_CONFIG% -Project="%UPROJECT%" -WaitMutex -FromMsBuild -architecture=x64
if errorlevel 1 (
    set "FAIL_CODE=!ERRORLEVEL!"
    goto BuildFailed
)

if exist "%PROJECT_PCGEX%" (
    set "CURRENT_STEP=Build Development editor cook host"
    echo [2/5] !CURRENT_STEP!
    call "%BUILD_BAT%" Project_EdenEditor Win64 Development -Project="%UPROJECT%" -WaitMutex -FromMsBuild -architecture=x64
    if errorlevel 1 (
        set "FAIL_CODE=!ERRORLEVEL!"
        goto BuildFailed
    )
) else (
    echo [2/5] Installed PCGExtendedToolkit has a %BUILD_CONFIG% Game manifest; editor cook-host build is not required.
)

set "CURRENT_STEP=Build %BUILD_CONFIG% dedicated server"
echo [3/5] !CURRENT_STEP!
call "%BUILD_BAT%" Project_EdenServer Win64 %BUILD_CONFIG% -Project="%UPROJECT%" -WaitMutex -FromMsBuild -architecture=x64
if errorlevel 1 (
    set "FAIL_CODE=!ERRORLEVEL!"
    goto BuildFailed
)

set "CURRENT_STEP=Cook and package %BUILD_CONFIG% client"
echo [4/5] !CURRENT_STEP!
call "%RUN_UAT%" BuildCookRun -project="%UPROJECT%" -noP4 -targetplatform=Win64 -clientconfig=%BUILD_CONFIG% -nocompile -nocompileeditor -cook -stage -package -pak -prereqs -archive -archivedirectory="!CLIENT_ARCHIVE!" -map=%CLIENT_MAPS% -unattended -utf8output
if errorlevel 1 (
    set "FAIL_CODE=!ERRORLEVEL!"
    goto BuildFailed
)

set "CURRENT_STEP=Cook and package %BUILD_CONFIG% dedicated server"
echo [5/5] !CURRENT_STEP!
call "%RUN_UAT%" BuildCookRun -project="%UPROJECT%" -noP4 -server -noclient -serverplatform=Win64 -serverconfig=%BUILD_CONFIG% -nocompile -nocompileeditor -cook -stage -package -pak -prereqs -archive -archivedirectory="!SERVER_ARCHIVE!" -map=%SERVER_MAPS% -unattended -utf8output
if errorlevel 1 (
    set "FAIL_CODE=!ERRORLEVEL!"
    goto BuildFailed
)

set "CURRENT_STEP=Verify deployment outputs"
for %%F in (
    "!CLIENT_OUTPUT!\Project_Eden.exe"
    "!CLIENT_OUTPUT!\Project_Eden\Binaries\Win64\Project_Eden.exe"
    "!CLIENT_OUTPUT!\Project_Eden\Content\Paks\Project_Eden-Windows.pak"
    "!CLIENT_OUTPUT!\Project_Eden\Content\Paks\Project_Eden-Windows.ucas"
    "!CLIENT_OUTPUT!\Project_Eden\Content\Paks\Project_Eden-Windows.utoc"
    "!SERVER_OUTPUT!\Project_EdenServer.exe"
    "!SERVER_OUTPUT!\Project_Eden\Binaries\Win64\Project_EdenServer.exe"
    "!SERVER_OUTPUT!\Project_Eden\Content\Paks\Project_Eden-WindowsServer.pak"
    "!SERVER_OUTPUT!\Project_Eden\Content\Paks\Project_Eden-WindowsServer.ucas"
    "!SERVER_OUTPUT!\Project_Eden\Content\Paks\Project_Eden-WindowsServer.utoc"
) do (
    if not exist "%%~F" (
        echo Required deployment file is missing: %%~F
        set "FAIL_CODE=1"
        goto BuildFailed
    )
)

set "VERIFY_SERVER_UTOC=!SERVER_OUTPUT!\Project_Eden\Content\Paks\Project_Eden-WindowsServer.utoc"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$Bytes = [IO.File]::ReadAllBytes($env:VERIFY_SERVER_UTOC); $Text = [Text.Encoding]::UTF8.GetString($Bytes); if (-not $Text.Contains('BP_RunPortal.uasset')) { exit 1 }"
if errorlevel 1 (
    echo Server package does not contain BP_RunPortal.uasset.
    set "FAIL_CODE=1"
    goto BuildFailed
)

if not exist "!CLIENT_OUTPUT!\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe" (
    echo WARNING: UE prerequisite installer was not staged. Test on a clean client PC before distribution.
)
if not exist "!SERVER_OUTPUT!\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe" (
    echo WARNING: UE prerequisite installer was not staged. Install the VC++ 2015-2022 x64 runtime on the server host.
)

> "!SERVER_OUTPUT!\.project_eden_server_cook_complete" echo Project_Eden %BUILD_CONFIG% server cook completed successfully.
> "!RELEASE_ROOT!\BUILD_INFO.txt" (
    echo Project Eden Final Deployment
    echo BuiltAt=%BUILD_STAMP%
    echo Commit=!GIT_SHA!
    echo SourceTree=!TREE_STATE!
    echo Configuration=%BUILD_CONFIG%
    echo ClientFolder=Client\Windows
    echo ServerFolder=Server\WindowsServer
    echo ServerPort=7778/UDP
)
del /q "!RELEASE_ROOT!\BUILD_INCOMPLETE.txt" >nul 2>nul

echo.
echo Final build, cook, package, and verification succeeded.
echo Deploy the complete folders below; do not copy only the executables.
echo Client: !CLIENT_OUTPUT!
echo Server: !SERVER_OUTPUT!
echo Server command: Project_EdenServer.exe -log -port=7778
echo.
if "%PAUSE_AT_END%"=="1" pause
exit /b 0

:BuildFailed
if not defined FAIL_CODE set "FAIL_CODE=1"
>> "!RELEASE_ROOT!\BUILD_INCOMPLETE.txt" echo FailedStep=!CURRENT_STEP!
>> "!RELEASE_ROOT!\BUILD_INCOMPLETE.txt" echo ExitCode=!FAIL_CODE!
echo.
echo Final deployment failed during: !CURRENT_STEP!
echo Exit code: !FAIL_CODE!
echo Incomplete output was kept for diagnostics:
echo !RELEASE_ROOT!
echo.
if "%PAUSE_AT_END%"=="1" pause
exit /b !FAIL_CODE!

:UsageSuccess
call :PrintUsage
exit /b 0

:UsageFailure
call :PrintUsage
exit /b 2

:PrintUsage
echo Usage: %~nx0 [EngineRoot] [options]
echo.
echo Options:
echo   --shipping       Build Shipping client and server (default).
echo   --development    Build Development packages for a release smoke test.
echo   --allow-dirty    Allow a non-reproducible build from local changes.
echo   --validate-only  Run preflight checks without building or creating output.
echo   --no-pause       Do not pause after a completed or failed build.
echo   --help            Show this help.
echo.
echo Output:
echo   Saved\FinalDeploy\TIMESTAMP_COMMIT_CONFIG\Client\Windows
echo   Saved\FinalDeploy\TIMESTAMP_COMMIT_CONFIG\Server\WindowsServer
exit /b 0
