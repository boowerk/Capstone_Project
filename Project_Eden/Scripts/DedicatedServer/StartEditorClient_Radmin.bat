@echo off
setlocal EnableExtensions

echo WARNING: UnrealEditor.exe -game can crash in UE 5.7 while legacy animation
echo curve metadata is migrated during streamed village loading.
echo For dedicated-server validation, prefer:
echo   BuildDevClient.bat
echo   CookDevClient.bat
echo   StartDevClient_Cooked_Radmin.bat
echo.

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"
set "SERVER_PORT=7778"
set "CLIENT_WINDOW_ARGS=-windowed -ResX=960 -ResY=540 -WinX=60 -WinY=60"

for /f "tokens=1,* delims==" %%A in ('findstr /b /c:"ServerDefaultMap=" "%PROJECT_ROOT%Config\DefaultEngine.ini"') do set "CLIENT_DEFAULT_MAP=%%B"
if "%CLIENT_DEFAULT_MAP%"=="" (
    echo ServerDefaultMap was not found in %PROJECT_ROOT%Config\DefaultEngine.ini
    pause
    exit /b 1
)

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

if "%UE_SERVER_ROOT%"=="" (
    for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ResolveEngineRoot.ps1" -UProject "%UPROJECT%" -Kind Editor`) do set "UE_SERVER_ROOT=%%I"
)

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
echo Client GameDefaultMap override: %CLIENT_DEFAULT_MAP%
echo Engine root: %UE_SERVER_ROOT%
start "Project Eden Radmin Client" "%EDITOR_EXE%" "%UPROJECT%" %CLIENT_URL% -game -log -ini:Engine:[/Script/EngineSettings.GameMapsSettings]:GameDefaultMap=%CLIENT_DEFAULT_MAP% %CLIENT_WINDOW_ARGS% %EXTRA_ARGS%
exit /b 0
