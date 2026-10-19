@echo off

echo Beginning Build process...
echo ====================================
echo.

if not exist "build" (
    echo build folder not found, creating at root directory...
    mkdir "build"
    echo.
)

set CFLAGS=-ffreestanding -Ikernel/cpu -Ikernel/drivers -Iapps/shell -Ikernel/lib -Ikernel/vigil -Ikernel/fs -Ikernel/mem -Ikernel/ui -Iapps/launcher

:: kernel objs

echo Assembling boot.asm...
nasm -f bin boot/boot.asm -o build/boot.bin
if errorlevel 1 goto :error

echo Assembling stage2.asm...
nasm -f elf32 boot/stage2.asm -o build/stage2.o
if errorlevel 1 goto :error

echo Compiling kernel.c...
i686-elf-gcc %CFLAGS% -c kernel/kernel.c -o build/kernel.o
if errorlevel 1 goto :error

echo.

:: cpu files

echo Assembling idt_load.asm...
nasm -f elf32 kernel/cpu/idt_load.asm -o build/idt_load.o
if errorlevel 1 goto :error

echo Assembling irq.asm...
nasm -f elf32 kernel/cpu/irq.asm -o build/irq_asm.o
if errorlevel 1 goto :error

echo Assembling isr.asm...
nasm -f elf32 kernel/cpu/isr.asm -o build/isr_asm.o
if errorlevel 1 goto :error

echo Compiling idt.c...
i686-elf-gcc %CFLAGS% -c kernel/cpu/idt.c -o build/idt.o
if errorlevel 1 goto :error

echo Compiling irq.c...
i686-elf-gcc %CFLAGS% -c kernel/cpu/irq.c -o build/irq.o
if errorlevel 1 goto :error

echo Compiling isr.c...
i686-elf-gcc %CFLAGS% -c kernel/cpu/isr.c -o build/isr.o
if errorlevel 1 goto :error

echo.

:: drivers

echo Compiling ata.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/ata.c -o build/ata.o
if errorlevel 1 goto :error

echo Compiling font.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/font.c -o build/font.o
if errorlevel 1 goto :error

echo Compiling graphics.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/graphics.c -o build/graphics.o
if errorlevel 1 goto :error

echo Compiling keyboard.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/keyboard.c -o build/keyboard.o
if errorlevel 1 goto :error

echo Compiling mouse.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/mouse.c -o build/mouse.o
if errorlevel 1 goto :error

echo Compiling pic.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/pic.c -o build/pic.o
if errorlevel 1 goto :error

echo Compiling terminal.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/terminal.c -o build/terminal.o
if errorlevel 1 goto :error

echo Compiling timer.c...
i686-elf-gcc %CFLAGS% -c kernel/drivers/timer.c -o build/timer.o
if errorlevel 1 goto :error

echo.

:: filesystem

echo Compiling fs.c...
i686-elf-gcc %CFLAGS% -c kernel/fs/fs.c -o build/fs.o
if errorlevel 1 goto :error

echo.

:: lib files

echo Compiling kstring.c...
i686-elf-gcc %CFLAGS% -c kernel/lib/kstring.c -o build/kstring.o
if errorlevel 1 goto :error

echo Compiling kprintf.c...
i686-elf-gcc %CFLAGS% -c kernel/lib/kprintf.c -o build/kprintf.o
if errorlevel 1 goto :error

echo.

:: heap

echo Compiling heap.c...
i686-elf-gcc %CFLAGS% -c kernel/mem/heap.c -o build/heap.o
if errorlevel 1 goto :error

echo.

:: memory

echo Compiling memory.c...
i686-elf-gcc %CFLAGS% -c kernel/vigil/memory.c -o build/vigil_memory.o
if errorlevel 1 goto :error

:: app loader

echo Compiling api.c...
i686-elf-gcc %CFLAGS% -c kernel/vigil/api.c -o build/vigil_api.o
if errorlevel 1 goto :error

echo Compiling loader.c...
i686-elf-gcc %CFLAGS% -c kernel/vigil/loader.c -o build/loader.o
if errorlevel 1 goto :error

echo.

:: UI elements

echo Compiling button.c...
i686-elf-gcc %CFLAGS% -c kernel/ui/button.c -o build/button.o
if errorlevel 1 goto :error

echo Compiling menu.c...
i686-elf-gcc %CFLAGS% -c kernel/ui/menu.c -o build/menu.o
if errorlevel 1 goto :error

echo Compiling prompt.c...
i686-elf-gcc %CFLAGS% -c kernel/ui/prompt.c -o build/prompt.o
if errorlevel 1 goto :error

echo Compiling title_bar.c...
i686-elf-gcc %CFLAGS% -c kernel/ui/title_bar.c -o build/title_bar.o
if errorlevel 1 goto :error

echo Compiling taskbar.c...
i686-elf-gcc %CFLAGS% -c kernel/ui/taskbar.c -o build/taskbar.o
if errorlevel 1 goto :error

echo.

:: compile app files

echo Compiling App Launcher (launcher.c)...
i686-elf-gcc %CFLAGS% -c apps/launcher/launcher.c -o build/launcher.o
if errorlevel 1 goto :error
echo Beginning app compilation...

echo Compiling Shell...
i686-elf-gcc %CFLAGS% -c apps/shell/shell.c -o build/shell.o
if errorlevel 1 goto :error

echo.

:: loadable apps (built separately from kernel then installed on disk)

set APP_CFLAGS=-ffreestanding -fno-asynchronous-unwind-tables -Iapps/launcher -Ikernel/vigil -Ikernel/ui -Ikernel/drivers -Ikernel/fs -Ikernel/lib -Ikernel/mem

echo Building app: hello (slot 0 = 0x400000)...
i686-elf-gcc %APP_CFLAGS% -c apps/hello/hello.c -o build/hello.o
if errorlevel 1 goto :error
i686-elf-ld -T apps/sdk/app.ld -Ttext=0x400000 -o build/hello.elf build/hello.o
if errorlevel 1 goto :error
i686-elf-objcopy -O binary build/hello.elf build/hello.vapp
if errorlevel 1 goto :error

echo Building app: calculator (slot 1 = 0x440000)...
i686-elf-gcc %APP_CFLAGS% -Iapps/calculator -c apps/calculator/calc.c -o build/calc_app.o
if errorlevel 1 goto :error
i686-elf-gcc %APP_CFLAGS% -Iapps/calculator -c apps/calculator/calculator.c -o build/calculator_app.o
if errorlevel 1 goto :error
i686-elf-ld -T apps/sdk/app.ld -Ttext=0x440000 -o build/calculator.elf build/calculator_app.o build/calc_app.o
if errorlevel 1 goto :error
i686-elf-objcopy -O binary build/calculator.elf build/calculator.vapp
if errorlevel 1 goto :error

echo Building app: files (slot 2 = 0x480000)...
i686-elf-gcc %APP_CFLAGS% -c apps/files/files.c -o build/files_app.o
if errorlevel 1 goto :error
i686-elf-ld -T apps/sdk/app.ld -Ttext=0x480000 -o build/files.elf build/files_app.o
if errorlevel 1 goto :error
i686-elf-objcopy -O binary build/files.elf build/files.vapp
if errorlevel 1 goto :error

echo Building app: notepad (slot 3 = 0x4c0000)...
i686-elf-gcc %APP_CFLAGS% -c apps/notepad/notepad.c -o build/notepad_app.o
if errorlevel 1 goto :error
i686-elf-ld -T apps/sdk/app.ld -Ttext=0x4c0000 -o build/notepad.elf build/notepad_app.o
if errorlevel 1 goto :error
i686-elf-objcopy -O binary build/notepad.elf build/notepad.vapp
if errorlevel 1 goto :error

echo.

:: link build files using linker script

echo.
echo Building img build...
echo ====================================
echo.

echo Linking kernel objects...
i686-elf-ld -T boot/linker.ld -o build/kernel_full.elf build/stage2.o build/kernel.o build/terminal.o build/idt.o build/isr.o build/isr_asm.o build/idt_load.o build/pic.o build/irq.o build/irq_asm.o build/keyboard.o build/kstring.o build/kprintf.o build/shell.o build/ata.o build/vigil_memory.o build/fs.o build/timer.o build/heap.o build/font.o build/graphics.o build/mouse.o build/button.o build/title_bar.o build/taskbar.o build/launcher.o build/menu.o build/prompt.o build/vigil_api.o build/loader.o
if errorlevel 1 goto :error
echo.

:: convert to binary

echo Converting to flat binary...
i686-elf-objcopy -O binary build/kernel_full.elf build/kernel_full.bin
if errorlevel 1 goto :error
echo.

:: build into img

:: check size of kernel_full.bin
echo Validating kernel size against bootloader sector specification...

:: extract sector count from boot.asm, default to 64
set "SECTORS="
for /f "tokens=3" %%A in ('findstr /R /C:"SECTOR_COUNT equ" boot\boot.asm') do (
    set "SECTORS=%%A"
)
if "%SECTORS%"=="" (
    echo [WARNING] Could not parse sector count from boot.asm - Defaulting to 64
    set SECTORS=64
)

:: read kernel_full.bin size
set /a MAX_BYTES=%SECTORS% * 512
for %%I in ("build\kernel_full.bin") do set "KERNEL_BYTES=%%~zI"

echo Max capacity from boot.asm: %MAX_BYTES% bytes (%SECTORS% sectors)
echo Actual kernel_full.bin size: %KERNEL_BYTES% bytes

:: check if kernel fits within allocated space
if %KERNEL_BYTES% GTR %MAX_BYTES% (
    goto :kernel_too_big
)

echo Verified kernel size fits within the allocated sectors
echo.

:: build padding
set /a PADDING_SIZE=%MAX_BYTES%
if exist build\padding.bin del build\padding.bin
fsutil file createnew build\padding.bin %PADDING_SIZE% >nul

:: create storage.img / dist if not already present
if not exist "dist" (
    echo dist folder not found, creating at root directory...
    mkdir "dist"
    echo.
)
if not exist "dist\storage.img" (
    echo Storage disk not found
    echo Creating persistent storage disk ^(dist\storage.img^)...
    fsutil file createnew dist\storage.img 10485760 >nul
    echo.
)

echo Building disk image...
copy /b build\boot.bin+build\kernel_full.bin+build\padding.bin dist\vigil.img >nul
echo.

:: install apps onto disk

echo Installing apps onto storage disk...
python tools/install_app.py dist/storage.img build/hello.vapp apps/hello
if errorlevel 1 goto :error
python tools/install_app.py dist/storage.img build/calculator.vapp apps/calculator
if errorlevel 1 goto :error
python tools/install_app.py dist/storage.img build/files.vapp apps/files
if errorlevel 1 goto :error
python tools/install_app.py dist/storage.img build/notepad.vapp apps/notepad
echo.

:: successful build

echo Build Success^^! Check dist\vigil.img for output

goto :eof

:kernel_too_big

echo.
echo =======================================================================
echo [ERROR] CRITICAL BUILD FAILURE: KERNEL IS TOO LARGE FOR THE BOOTLOADER!
echo Kernel size is %KERNEL_BYTES% bytes, but boot.asm is only configured to
echo read %SECTORS% sectors (%MAX_BYTES% bytes).
echo =======================================================================
echo.

:error
echo [ERROR] Build failed^^!
exit /b 1

:eof
