@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"
set "CLIENT_URL=127.0.0.1:7778"
set "CLIENT_WINDOW_ARGS=-windowed -ResX=960 -ResY=540 -WinX=60 -WinY=60"

for /f "tokens=1,* delims==" %%A in ('findstr /b /c:"ServerDefaultMap=" "%PROJECT_ROOT%Config\DefaultEngine.ini"') do set "CLIENT_DEFAULT_MAP=%%B"
if "%CLIENT_DEFAULT_MAP%"=="" (
    echo ServerDefaultMap was not found in %PROJECT_ROOT%Config\DefaultEngine.ini
    pause
    exit /b 1
)

rem Optional first argument: engine root folder that contains the Engine directory.
rem Example: StartEditorClient_Localhost.bat "C:\Engine_server\Windows"
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
    echo %~nx0 "C:\Path\To\Your\EngineRoot"
    pause
    exit /b 1
)

set "EXTRA_ARGS="

:CollectExtraArgs
if "%~1"=="" goto ExtraArgsDone
set "EXTRA_ARGS=%EXTRA_ARGS% %~1"
shift /1
goto CollectExtraArgs

:ExtraArgsDone

echo Starting Project Eden local client: %CLIENT_URL%
echo Client GameDefaultMap override: %CLIENT_DEFAULT_MAP%
echo Engine root: %UE_SERVER_ROOT%
start "Project Eden Local Client" "%EDITOR_EXE%" "%UPROJECT%" %CLIENT_URL% -game -log -ini:Engine:[/Script/EngineSettings.GameMapsSettings]:GameDefaultMap=%CLIENT_DEFAULT_MAP% %CLIENT_WINDOW_ARGS% %EXTRA_ARGS%
exit /b 0
