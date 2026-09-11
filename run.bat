@echo off

echo Assembling boot.asm...
nasm -f bin boot/boot.asm -o boot/boot.bin
if errorlevel 1 goto :error

echo Assembling stage2.asm...
nasm -f bin boot/stage2.asm -o boot/stage2.bin
if errorlevel 1 goto :error

if not exist build mkdir build

echo Building disk image...
copy /b boot\boot.bin+boot\stage2.bin build\vigil.img >nul

echo Booting VIGIL in QEMU...
qemu-system-x86_64 -fda build\vigil.img

goto :eof

:error
echo Build failed
exit /b 1

:eof
