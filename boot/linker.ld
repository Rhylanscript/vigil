ENTRY(stage2_start)

SECTIONS
{
    . = 0x7e00;

    .text : {
        boot/stage2.o(.text)
        boot/kernel.o(.text)
    }

    .rodata : {
        boot/stage2.o(.rodata)
        boot/kernel.o(.rodata)
    }

    .data : {
        boot/stage2.o(.data)
        boot/kernel.o(.data)
    }

    .bss : {
        boot/stage2.o(.bss)
        boot/kernel.o(.bss)
    }
}
