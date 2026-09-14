@echo off

if not exist dist\vigil.img (
    echo [ERROR] dist\vigil.img not found^^! Run build.bat first
    exit /b 1
)

if not exist dist\storage.img (
    echo [ERROR] dist\storage.img not found^^! Run build.bat first
    exit /b 1
)

echo Booting VIGIL in QEMU...
qemu-system-x86_64 -drive file=dist\vigil.img,format=raw,if=floppy -hda dist\storage.img
