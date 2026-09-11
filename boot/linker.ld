ENTRY(stage2_start)

SECTIONS
{
    . = 0x7e00;

    .text : {
        boot/stage2.o(.text)
        boot/kernel.o(.text)
        boot/terminal.o(.text)
    }

    .rodata : {
        boot/stage2.o(.rodata)
        boot/kernel.o(.rodata)
        boot/terminal.o(.rodata)
    }

    .data : {
        boot/stage2.o(.data)
        boot/kernel.o(.data)
        boot/terminal.o(.data)
    }

    .bss : {
        boot/stage2.o(.bss)
        boot/kernel.o(.bss)
        boot/terminal.o(.bss)
    }
}
