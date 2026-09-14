@echo off
setlocal EnableDelayedExpansion
echo Cleaning workspace...

:: check args
set "FULL_CLEAN=0"
set "FORCE_CLEAN=0"

:parse_args

if "%~1"=="" goto :args_done
if /i "%~1"=="--full" set "FULL_CLEAN=1"
if /i "%~1"=="-fc" set "FULL_CLEAN=1"
if /i "%~1"=="--force" set "FORCE_CLEAN=1"
if /i "%~1"=="-rf" set "FORCE_CLEAN=1"
shift
goto :parse_args

:args_done

if "%FULL_CLEAN%"=="1" if not "%FORCE_CLEAN%"=="1" (
    echo.
    set /p CONFIRM="Delete all build files? (Y/n) "
    if /i not "!CONFIRM!"=="y" if not "!CONFIRM!"=="" (
        echo Clean Aborted - nothing was deleted.
        exit /b 0
    )
)

if "%FULL_CLEAN%"=="1" (
    echo Full clean requested - all build files will be deleted.
    echo.
)

:: clean build/
if exist build (
    echo Emptying build folder...
    del /f /q /s build\* >nul 2>&1
    for /d %%p in (build\*) do rmdir /s /q "%%p" >nul 2>&1
    type nul > build\.gitkeep
) else (
    echo build directory not found^^!
    exit /b 1
)

:: clean dist/
if exist dist (
    if "%FULL_CLEAN%"=="1" (
        echo Emptying dist folder...
        del /f /q /s dist\* >nul 2>&1
    ) else (
        echo Emptying dist folder ^(preserving storage.img^)...
        for %%f in (dist\*) do (
            if /i not "%%~nxf"=="storage.img" del /f /q "%%f" >nul 2>&1
        )
    )
    for /d %%p in (dist\*) do rmdir /s /q "%%p" >nul 2>&1
    if not exist dist\.gitkeep type nul > dist\.gitkeep
) else (
    echo dist directory not found^^!
    exit /b 1
)

echo.
echo Workspace clean^^!

goto :eof

:full_clean

set "FULL_CLEAN=1"
echo Full clean requested - storage.img will also be removed.

:eof
