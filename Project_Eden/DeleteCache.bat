@echo off
set CURRENT_DIR=%~dp0
set SLN_FILE=%CURRENT_DIR%Project_Eden.sln

if exist "%CURRENT_DIR%Binaries" (
    rd /s /q "%CURRENT_DIR%Binaries"
)
if exist "%CURRENT_DIR%Intermediate" (
    rd /s /q "%CURRENT_DIR%Intermediate"
)
if exist "%CURRENT_DIR%DerivedDataCache" (
    rd /s /q "%CURRENT_DIR%DerivedDataCache"
)
if exist "%SLN_FILE%" (
    del /f /q "%SLN_FILE%"
)