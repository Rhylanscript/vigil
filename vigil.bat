@echo off
setlocal enabledelayedexpansion

:: overall batch file to run all commands in scripts\

:: check if subcommand was provided
if "%~1"=="" goto :usage

set "SUBCOMMAND=%~1"

:: route to correct script
if /i "%SUBCOMMAND%"=="build" (
    if not exist scripts/build.bat goto :missing
    call scripts/build.bat
    exit /b !errorlevel!
)

if /i "%SUBCOMMAND%"=="run" (
    if not exist scripts/run.bat goto :missing
    call scripts/run.bat
    exit /b !errorlevel!
)

if /i "%SUBCOMMAND%"=="clean" (
    if not exist scripts/clean.bat goto :missing
    call scripts/clean.bat
    exit /b !errorlevel!
)

if /i "%SUBCOMMAND%"=="dev" (
    if not exist scripts/dev.bat goto :missing
    call scripts/dev.bat
    exit /b !errorlevel!
)

echo [ERROR] unknown command: vigil %SUBCOMMAND%
goto :usage

:missing
echo [ERROR] &SUBCOMMAND%.bat missing from scripts/
exit /b 1

:usage
echo VIGIL OS Development Tool
echo.
echo Usage: vigil [command]
echo.
echo Commands:
echo    build   Compiles source code into build/ and dist/
echo    run     Launches QEMU using the existing disk image
echo    all     Builds the OS and immediately launches QEMU
echo    clean   Wipes the build/ and dist/ directories
exit /b 1
