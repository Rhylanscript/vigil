@echo off

echo Beginning Build process...
echo ====================================
echo.

if not exist "build" (
    echo build folder not found, creating at root directory...
    mkdir "build"
    echo.
)

set CFLAGS=-ffreestanding -Iboot/cpu -Iboot/drivers -Iboot/shell -Iboot/lib -Iboot/vigil -Iboot/fs -Iboot/mem

echo Assembling boot.asm...
nasm -f bin boot/boot.asm -o build/boot.bin
if errorlevel 1 goto :error

echo Assembling stage2.asm...
nasm -f elf32 boot/stage2.asm -o build/stage2.o
if errorlevel 1 goto :error

echo Compiling kernel.c...
i686-elf-gcc %CFLAGS% -c boot/kernel.c -o build/kernel.o
if errorlevel 1 goto :error

echo Compiling terminal.c...
i686-elf-gcc %CFLAGS% -c boot/drivers/terminal.c -o build/terminal.o
if errorlevel 1 goto :error

echo Compiling idt.c...
i686-elf-gcc %CFLAGS% -c boot/cpu/idt.c -o build/idt.o
if errorlevel 1 goto :error

echo Compiling isr.c...
i686-elf-gcc %CFLAGS% -c boot/cpu/isr.c -o build/isr.o
if errorlevel 1 goto :error

echo Assembling isr.asm...
nasm -f elf32 boot/cpu/isr.asm -o build/isr_asm.o
if errorlevel 1 goto :error

echo Assembling idt_load.asm...
nasm -f elf32 boot/cpu/idt_load.asm -o build/idt_load.o
if errorlevel 1 goto :error

echo Compiling pic.c...
i686-elf-gcc %CFLAGS% -c boot/drivers/pic.c -o build/pic.o
if errorlevel 1 goto :error

echo Compiling irq.c...
i686-elf-gcc %CFLAGS% -c boot/cpu/irq.c -o build/irq.o
if errorlevel 1 goto :error

echo Assembling irq.asm...
nasm -f elf32 boot/cpu/irq.asm -o build/irq_asm.o
if errorlevel 1 goto :error

echo Compiling keyboard.c...
i686-elf-gcc %CFLAGS% -c boot/drivers/keyboard.c -o build/keyboard.o
if errorlevel 1 goto :error

echo Compiling kstring.c...
i686-elf-gcc %CFLAGS% -c boot/lib/kstring.c -o build/kstring.o
if errorlevel 1 goto :error

echo Compiling shell.c...
i686-elf-gcc %CFLAGS% -c boot/shell/shell.c -o build/shell.o
if errorlevel 1 goto :error

echo Compiling ata.c...
i686-elf-gcc %CFLAGS% -c boot/drivers/ata.c -o build/ata.o
if errorlevel 1 goto :error

echo Compiling memory.c...
i686-elf-gcc %CFLAGS% -c boot/vigil/memory.c -o build/vigil_memory.o
if errorlevel 1 goto :error

echo Compiling fs.c...
i686-elf-gcc %CFLAGS% -c boot/fs/fs.c -o build/fs.o
if errorlevel 1 goto :error

echo Compiling timer.c...
i686-elf-gcc %CFLAGS% -c boot/drivers/timer.c -o build/timer.o
if errorlevel 1 goto :error

echo Compiling heap.c...
i686-elf-gcc %CFLAGS% -c boot/mem/heap.c -o build/heap.o
if errorlevel 1 goto :error

echo.
echo Beginning img build...
echo.

echo Linking kernel objects...
i686-elf-ld -T boot/linker.ld -o build/kernel_full.elf build/stage2.o build/kernel.o build/terminal.o build/idt.o build/isr.o build/isr_asm.o build/idt_load.o build/pic.o build/irq.o build/irq_asm.o build/keyboard.o build/kstring.o build/shell.o build/ata.o build/vigil_memory.o build/fs.o build/timer.o build/heap.o
if errorlevel 1 goto :error

echo.

echo Converting to flat binary...
i686-elf-objcopy -O binary build/kernel_full.elf build/kernel_full.bin
if errorlevel 1 goto :error
echo.

if exist build\padding.bin del build\padding.bin
fsutil file createnew build\padding.bin 32768 >nul

if not exist "dist" (
    echo dist folder not found, creating at root directory...
    mkdir "dist"
    echo.
)

if not exist "dist\storage.img" (
    echo Creating persistent storage disk ^(dist\storage.img^)...
    fsutil file createnew dist\storage.img 10485760 >nul
    echo.
)

echo Building disk image...
copy /b build\boot.bin+build\kernel_full.bin+build\padding.bin dist\vigil.img >nul

echo Build Success^^! Check dist\vigil.img for output

goto :eof

:error
echo Build failed^^!
exit /b 1

:eof
