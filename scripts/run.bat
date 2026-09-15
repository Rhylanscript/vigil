@echo off

if not exist dist\vigil.img (
    echo.
    echo [ERROR] dist\vigil.img not found^^! Run build.bat first
    exit /b 1
)

if not exist dist\storage.img (
    echo.
    echo [ERROR] dist\storage.img not found^^! Run build.bat first
    exit /b 1
)

if "%~1"=="--nat" (
    echo Booting VIGIL in QEMU using native terminal...
    qemu-system-x86_64 -drive file=dist\vigil.img,format=raw,if=ide,index=0 -drive file=dist\storage.img,format=raw,if=ide,index=1 -no-reboot -no-shutdown -nographic -serial mon:stdio
) else (
    echo Booting VIGIL in QEMU...
    qemu-system-x86_64 -drive file=dist\vigil.img,format=raw,if=ide,index=0 -drive file=dist\storage.img,format=raw,if=ide,index=1 -no-reboot -no-shutdown
)
