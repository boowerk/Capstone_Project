@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0..\..\"
set "UPROJECT=%PROJECT_ROOT%Project_Eden.uproject"

rem Optional first argument: engine root folder that contains the Engine directory.
rem Example: PreparePCGExClientBuild.bat "C:\Engine_server\Windows"
if not "%~1"=="" (
    if exist "%~1\Engine\Plugins\Marketplace\PCGExtendedToolkit\PCGExtendedToolkit.uplugin" (
        set "UE_SERVER_ROOT=%~1"
        shift /1
    )
)

if "%UE_SERVER_ROOT%"=="" (
    for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ResolveEngineRoot.ps1" -UProject "%UPROJECT%" -Kind Build`) do set "UE_SERVER_ROOT=%%I"
)

if "%UE_SERVER_ROOT%"=="" (
    echo Engine root was not found.
    pause
    exit /b 1
)

set "SOURCE_PLUGIN=%UE_SERVER_ROOT%\Engine\Plugins\Marketplace\PCGExtendedToolkit"
set "PROJECT_PLUGIN=%PROJECT_ROOT%Plugins\PCGExtendedToolkit"
if not exist "%SOURCE_PLUGIN%\PCGExtendedToolkit.uplugin" (
    echo PCGExtendedToolkit source plugin was not found:
    echo %SOURCE_PLUGIN%
    pause
    exit /b 1
)

if not exist "%PROJECT_PLUGIN%" mkdir "%PROJECT_PLUGIN%"
for %%D in (Config Content Resources Scripts Source) do (
    if exist "%SOURCE_PLUGIN%\%%D" (
        xcopy "%SOURCE_PLUGIN%\%%D" "%PROJECT_PLUGIN%\%%D\" /E /I /Y /Q >nul
        if errorlevel 1 (
            echo Failed to copy PCGExtendedToolkit\%%D.
            pause
            exit /b 1
        )
    )
)

copy /Y "%SOURCE_PLUGIN%\PCGExtendedToolkit.uplugin" "%PROJECT_PLUGIN%\PCGExtendedToolkit.uplugin" >nul
if errorlevel 1 (
    echo Failed to copy PCGExtendedToolkit.uplugin.
    pause
    exit /b 1
)

rem Marketplace installs are marked Installed=true and therefore require
rem precompiled UnrealGame manifests. The project-local source copy must compile.
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$Path = '%PROJECT_PLUGIN%\PCGExtendedToolkit.uplugin'; $Text = [IO.File]::ReadAllText($Path); $Text = $Text.Replace('\"Installed\": true', '\"Installed\": false'); [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))"
if errorlevel 1 (
    echo Failed to mark the project-local PCGExtendedToolkit as source-built.
    pause
    exit /b 1
)

echo Prepared project-local PCGExtendedToolkit source for Game builds:
echo %PROJECT_PLUGIN%
exit /b 0
