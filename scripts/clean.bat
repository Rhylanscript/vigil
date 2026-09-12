@echo off
echo Cleaning workspace...

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
    echo Emptying dist folder...
    del /f /q /s dist\* >nul 2>&1
    for /d %%p in (dist\*) do rmdir /s /q "%%p" >nul 2>&1
    type nul > dist\.gitkeep
) else (
    echo dist directory not found^^!
    exit /b 1
)

echo Workspace clean^^!
