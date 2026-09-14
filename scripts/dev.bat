@echo off

:: build process
if exist "scripts/build.bat" (
    call scripts/build.bat
) else (
    echo scripts/build.bat not found^^!
    exit /b 1
)

echo.

:: run process
if exist "scripts/run.bat" (
    call scripts/run.bat
) else (
    echo scripts/run.bat not found^^!
    exit /b 1
)
