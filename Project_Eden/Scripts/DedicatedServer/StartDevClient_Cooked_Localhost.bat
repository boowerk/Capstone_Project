@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "PACKAGED_CLIENT_EXE=%PROJECT_ROOT%Saved\DedicatedClient\Windows\Project_Eden.exe"
set "PACKAGED_CLIENT_EXE_ALT=%PROJECT_ROOT%Saved\DedicatedClient\Windows\Project_Eden\Binaries\Win64\Project_Eden.exe"
set "STAGED_CLIENT_EXE=%PROJECT_ROOT%Saved\StagedBuilds\Windows\Project_Eden.exe"
set "STAGED_CLIENT_EXE_ALT=%PROJECT_ROOT%Saved\StagedBuilds\Windows\Project_Eden\Binaries\Win64\Project_Eden.exe"
set "CLIENT_WINDOW_ARGS=-windowed -ResX=960 -ResY=540 -WinX=60 -WinY=60"

if exist "%PACKAGED_CLIENT_EXE%" (
    set "CLIENT_EXE=%PACKAGED_CLIENT_EXE%"
) else if exist "%PACKAGED_CLIENT_EXE_ALT%" (
    set "CLIENT_EXE=%PACKAGED_CLIENT_EXE_ALT%"
) else if exist "%STAGED_CLIENT_EXE%" (
    set "CLIENT_EXE=%STAGED_CLIENT_EXE%"
) else if exist "%STAGED_CLIENT_EXE_ALT%" (
    set "CLIENT_EXE=%STAGED_CLIENT_EXE_ALT%"
) else (
    echo Packaged client executable was not found.
    echo Run BuildDevClient.bat and CookDevClient.bat first.
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
echo Starting packaged Project Eden local client.
echo Client exe: %CLIENT_EXE%
start "Project Eden Cooked Local Client" "%CLIENT_EXE%" -log %CLIENT_WINDOW_ARGS% %EXTRA_ARGS%
exit /b 0
