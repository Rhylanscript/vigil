@echo off

echo Beginning Build process...
echo ====================================
echo.

if not exist "build" (
    echo build folder not found, creating at root directory...
    mkdir "build"
    echo.
)

set CFLAGS=-ffreestanding -Ikernel/cpu -Ikernel/drivers -Iapps/shell -Ikernel/lib -Ikernel/vigil -Ikernel/fs -Ikernel/mem -Ikernel/ui -Iapps/launcher -Iapps/notepad

echo Assembling boot.asm...
nasm -f bin boot/boot.asm -o build/boot.bin
if errorlevel 1 goto :error

echo Assembling stage2.asm...
nasm -f elf32 boot/stage2.asm -o build/stage2.o
if errorlevel 1 goto :error

echo Compiling kernel.c...
i686-elf-gcc %CFLAGS% -c kernel/kernel.c -o build/kernel.o
if errorlevel 1 goto :error

echo Compiling terminal.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/terminal.c -o build/terminal.o
if errorlevel 1 goto :error

echo Compiling idt.c...
i686-elf-gcc %CFLAGS% -c kernel/cpu/idt.c -o build/idt.o
if errorlevel 1 goto :error

echo Compiling isr.c...
i686-elf-gcc %CFLAGS% -c kernel/cpu/isr.c -o build/isr.o
if errorlevel 1 goto :error

echo Assembling isr.asm...
nasm -f elf32 kernel/cpu/isr.asm -o build/isr_asm.o
if errorlevel 1 goto :error

echo Assembling idt_load.asm...
nasm -f elf32 kernel/cpu/idt_load.asm -o build/idt_load.o
if errorlevel 1 goto :error

echo Compiling pic.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/pic.c -o build/pic.o
if errorlevel 1 goto :error

echo Compiling irq.c...
i686-elf-gcc %CFLAGS% -c kernel/cpu/irq.c -o build/irq.o
if errorlevel 1 goto :error

echo Assembling irq.asm...
nasm -f elf32 kernel/cpu/irq.asm -o build/irq_asm.o
if errorlevel 1 goto :error

echo Compiling keyboard.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/keyboard.c -o build/keyboard.o
if errorlevel 1 goto :error

echo Compiling kstring.c...
i686-elf-gcc %CFLAGS% -c kernel/lib/kstring.c -o build/kstring.o
if errorlevel 1 goto :error

echo Compiling kprintf.c...
i686-elf-gcc %CFLAGS% -c kernel/lib/kprintf.c -o build/kprintf.o
if errorlevel 1 goto :error

echo Compiling ata.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/ata.c -o build/ata.o
if errorlevel 1 goto :error

echo Compiling memory.c...
i686-elf-gcc %CFLAGS% -c kernel/vigil/memory.c -o build/vigil_memory.o
if errorlevel 1 goto :error

echo Compiling fs.c...
i686-elf-gcc %CFLAGS% -c kernel/fs/fs.c -o build/fs.o
if errorlevel 1 goto :error

echo Compiling timer.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/timer.c -o build/timer.o
if errorlevel 1 goto :error

echo Compiling heap.c...
i686-elf-gcc %CFLAGS% -c kernel/mem/heap.c -o build/heap.o
if errorlevel 1 goto :error

echo Compiling font.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/font.c -o build/font.o
if errorlevel 1 goto :error

echo Compiling graphics.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/graphics.c -o build/graphics.o
if errorlevel 1 goto :error

echo Compiling mouse.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/mouse.c -o build/mouse.o
if errorlevel 1 goto :error

echo Compiling button.c...
i686-elf-gcc %CFLAGS% -c kernel/ui/button.c -o build/button.o
if errorlevel 1 goto :error

echo Compiling title_bar.c...
i686-elf-gcc %CFLAGS% -c kernel/ui/title_bar.c -o build/title_bar.o
if errorlevel 1 goto :error

echo Compiling taskbar.c...
i686-elf-gcc %CFLAGS% -c kernel/ui/taskbar.c -o build/taskbar.o
if errorlevel 1 goto :error

echo Compiling notepad.c...
i686-elf-gcc %CFLAGS% -c apps/notepad/notepad.c -o build/notepad.o
if errorlevel 1 goto :error

echo Compiling shell.c...
i686-elf-gcc %CFLAGS% -c apps/shell/shell.c -o build/shell.o
if errorlevel 1 goto :error

echo Compiling launcher.c...
i686-elf-gcc %CFLAGS% -c apps/launcher/launcher.c -o build/launcher.o
if errorlevel 1 goto :error

echo.
echo Beginning img build...
echo.

echo Linking kernel objects...
i686-elf-ld -T boot/linker.ld -o build/kernel_full.elf build/stage2.o build/kernel.o build/terminal.o build/idt.o build/isr.o build/isr_asm.o build/idt_load.o build/pic.o build/irq.o build/irq_asm.o build/keyboard.o build/kstring.o build/kprintf.o build/shell.o build/ata.o build/vigil_memory.o build/fs.o build/timer.o build/heap.o build/font.o build/graphics.o build/mouse.o build/button.o build/title_bar.o build/taskbar.o build/launcher.o build/notepad.o
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
