@echo off

echo Assembling boot.asm...
nasm -f bin boot/boot.asm -o boot/boot.bin
if errorlevel 1 goto :error

echo Assembling stage2.asm...
nasm -f elf32 boot/stage2.asm -o boot/stage2.o
if errorlevel 1 goto :error

echo Compiling kernel.c...
i686-elf-gcc -ffreestanding -c boot/kernel.c -o boot/kernel.o
if errorlevel 1 goto :error

echo Compiling terminal.c...
i686-elf-gcc -ffreestanding -c boot/terminal.c -o boot/terminal.o
if errorlevel 1 goto :error

if not exist build mkdir build

echo Linking stage2.o, kernel.o, and terminal.o...
i686-elf-ld -T boot/linker.ld -o build/kernel_full.elf boot/stage2.o boot/kernel.o boot/terminal.o
if errorlevel 1 goto :error

echo Converting to flat binary...
i686-elf-objcopy -O binary build/kernel_full.elf build/kernel_full.bin
if errorlevel 1 goto :error

if exist build\padding.bin del build\padding.bin
fsutil file createnew build\padding.bin 32768 >nul

echo Building disk image...
copy /b boot\boot.bin+build\kernel_full.bin+build\padding.bin build\vigil.img >nul

echo Booting VIGIL in QEMU...
qemu-system-x86_64 -drive file=build\vigil.img,format=raw,if=floppy

goto :eof

:error
echo Build failed
exit /b 1

:eof
