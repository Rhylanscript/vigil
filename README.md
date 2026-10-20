# VIGIL

A small hobby operating system written from scratch in x86 assembly and C.

VIGIL boots from its own two stage bootloader into a 32-bit graphical desktop with a mouse cursor, a home screen, a taskbar, and a couple of apps (more will be added trust). No libraries are used and no libc, every driver, the memory allocator, the filesystem and the UI are written from scratch. I am building this so I can get a better undertsanding of how computers work at the lowest level.

## What it does

- Boots from a custom bootloader, switch the CPU into 32-bit protected mode, and run a C kernel
- Draws a 1024x768, 24 bit colour graphical interface with its own bitmap font
- Respond to PS/2 keyboard and mouse inputs
- Save data permanently to a virtual disk, so files and the boot counter remain after restarts
- Run apps from a home screen, with a cool title bar (minimize and close buttons) and a taskbar to show open apps

## Apps

### Terminal

The Terminal is a Command line shell built into the kernel with an 8 entry command history (cycle using up & down arrows).

| Command                   | What it does                                                              |
| ------------------------- | ------------------------------------------------------------------------- |
| `help`                    | Lists all commands and their usages                                       |
| `erase`                   | Clears the terminal screen                                                |
| `recall <text>`           | Prints the given text                                                     |
| `status`                  | Shows a one line description of VIGIL                                     |
| `cycles`                  | Shows how many times VIGIL has booted                                     |
| `uptime`                  | Shows seconds elapsed since boot                                          |
| `meminfo`                 | Shows free and total heap memory                                          |
| `write <name> <text>`     | Saves `<text>` to a file under `<name>`                                   |
| `read <name>`             | Shows the contents of file `<name>`                                       |
| `forget <name>`           | Deletes file `<name>`                                                     |
| `edit <name>`             | Opens file `<name>` in Notepad to edit                                    |
| `folder <path>`           | Create a folder at `<path>`                                               |
| `rename <path> <name>`    | Rename a file at `<path>` to `<name>`                                     |
| `files [folder]`          | Lists saved files in specified folder. If left blank defaults to root dir |
| `loadapp <path>`          | Loads an app file from disk                                               |

### Notepad

A text editor with a blinking cursor, arrow keys, Home, End, Delete, Backspace and Tab. Defaults to editing the file `records/notes`. Using the `edit` command in shell allows editing of any file.

### Files

A simple file browser for navigating the file tree. Click a file to open it in Notepad, and click a folder to view its contents. Right click a file for options such as renaming, deleting, or viewing more information about it.

### Calculator

A simple calculator supporting `-`, `+`, `*`, `/`, and `%`, operated with the keyboard.

### Tetris

The classic falling blocks game. Left/Right move, Up rotates, Down drops one row, Space drops all the way, `P` pauses and `R` restarts. The game speeds up every 10 rows cleared, and your best score is saved to the file `records/tetris_best` on the storage disk.

### Hello

A small example app demonstrating that apps can be loaded from the storage disk rather than being built into the kernel.

### Using apps

Click an app on the home screen to launch it. The `-` button (or the `Esc` key) minimizes an app and keeps it running in the taskbar, while the `X` button closes it completely.

### Builtin vs Loaded Apps

The Terminal is built into the kernel, while Calculator, Files, Hello, Tetris and Notepad are compiled separately and installed in the `apps` folder on the persistent storage disk and loaded by the launcher at startup. Loaded apps use the kernel's VIGIL API to access services such as graphics and the filesystem, they are not isolated from the kernel.

## How it works

1. **Stage 1:** The BIOS loads the 512-byte boot sector, which then reads stage 2 from disk
2. **Stage 2:** Enables the A20 line (access to memory above 1mb), asks the BIOS for a 1024x768 graphics mode, sets up the GDT, and switches the CPU to protected mode
3. **The kernel:** Installs interrupt handlers, remaps the interrupt controller, starts the timer, keyboard, and mouse, then hands control to the launcher
4. **Loadable apps:** The launcher loads app files from the `apps` folder on the separate storage disk into memory and registers them for the home screen

| Part          | Details                                                               |
| ------------- | --------------------------------------------------------------------- |
| CPU mode      | 32-bit protected mode, flat memory model                              |
| Display       | VBE linear framebuffer, 1024x768, 24 bits per pixel, 8x16 bitmap font |
| Interrupts    | IDT with CPU exception handlers, remapped 8259 PIC, 100 Hz timer      |
| Input         | PS/2 keyboard (Shift, Caps Lock, Ctrl, arrows) and PS/2 mouse         |
| Memory        | First-fit heap allocator over a 1 MB pool                             |
| Storage       | ATA PIO disk driver on a separate 10 MB virtual disk                  |
| Filesystem    | Up to 32 files and folders, 15-character names, with nested folders   |

## Running it

VIGILs helper batch scripts are for Windows and require [NASM][nasm], the [i686-elf cross-compiler tools][elf-tools], [Python 3][python], and [QEMU][qemu].

```bash
:: compile everything into dist/vigil.img
vigil build

:: boot it in QEMU with an SDL window (--no-sdl boots it normally)
vigil run [--no-sdl]

:: build, then run
vigil dev

:: wipe build output (keeps the saved storage disk)
vigil clean
```

`vigil build` creates the boot image and installs the loadable apps onto `dist/storage.img`. The storage disk persists between builds, so close QEMU before building and reinstalling apps.

## License

Repo is under an [MIT license][license].

<!-- LINKS -->

[nasm]: https://www.nasm.us/
[elf-tools]: https://github.com/lordmilko/i686-elf-tools
[qemu]: https://www.qemu.org/
[python]: https://www.python.org/downloads/

[license]: LICENSE
