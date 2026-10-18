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

Command line shell with an 8 entry command history (cycle using up & down arrows).

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

### Notepad

A text editor with a blinking cursor, arrow keys, Home, End, Delete, Backspace and Tab. Defaults to editing the file `/notes`. Using the `edit` command in shell allows editing of any file.

### Using apps

Click an app on the home screen to launch it. The `-` button (or the `Esc` key) minimizes an app and keeps it running in the taskbar whereas the `X` button closes it completely.

## How it works

1. **Stage 1:** the 512 byte boot sector the BIOS loads first, then it reads stage 2 from disk.
2. **Stage 2:** enables the A20 line (access to memory above 1mb), asks the BIOS for a 1024x768 graphics mode, sets up the GDT, and switches the CPU to protected mode
3. **The kernel:** (C) installs interrupt handlers, remaps the interrupt controller, starts the timer, keyboard and mouse, then hands control to the launcher, which runs forever

| Part          | Details                                                               |
| ------------- | --------------------------------------------------------------------- |
| CPU mode      | 32-bit protected mode, flat memory model                              |
| Display       | VBE linear framebuffer, 1024x768, 24 bits per pixel, 8x16 bitmap font |
| Interrupts    | IDT with CPU exception handlers, remapped 8259 PIC, 100 Hz timer      |
| Input         | PS/2 keyboard (Shift, Caps Lock, Ctrl, arrows) and PS/2 mouse         |
| Memory        | First-fit heap allocator over a 1 MB pool                             |
| Storage       | ATA PIO disk driver on a separate 10 MB virtual disk                  |
| Filesystem    | Flat (no folders), up to 16 files, 16 char long names                 |

## Running it

I work on VIGIL on windows (unfortunately). I have some helper batch scripts for executing simple actions for me, which need [NASM][nasm], the [i686-elf cross-compiler tools][elf-tools], and [QEMU][qemu].

```bash
:: compile everything into dist/vigil.img
vigil build

:: boot it in QEMU (use --sdl)
vigil run

:: build, then run
vigil dev

:: wipe build output (keeps the saved storage disk)
vigil clean
```

Side note - I would PROBABLY recommend running the run command with `--sdl` as an arguement (but this isnt available for dev, so I kind of stopped using it... I should probably add that functionality for dev......)

## License

Repo is under an [MIT license][license].

<!-- LINKS -->

[nasm]: https://www.nasm.us/
[elf-tools]: https://github.com/lordmilko/i686-elf-tools
[qemu]: https://www.qemu.org/

[license]: LICENSE
