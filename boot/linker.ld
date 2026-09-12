ENTRY(stage2_start)

SECTIONS
{
    . = 0x7e00;

    .text : {
        build/stage2.o(.text)
        build/kernel.o(.text)
        build/terminal.o(.text)
        build/isr_asm.o(.text)
        build/idt.o(.text)
        build/idt_load.o(.text)
    }

    .rodata : {
        build/stage2.o(.rodata)
        build/kernel.o(.rodata)
        build/terminal.o(.rodata)
        build/isr.o(.rodata)
        build/idt.o(.rodata)
        build/idt_load.o(.rodata)
    }

    .data : {
        build/stage2.o(.data)
        build/kernel.o(.data)
        build/terminal.o(.data)
        build/isr.o(.data)
        build/idt.o(.data)
        build/idt_load.o(.data)
    }

    .bss : {
        build/stage2.o(.bss)
        build/kernel.o(.bss)
        build/terminal.o(.bss)
        build/isr.o(.bss)
        build/idt.o(.bss)
        build/idt_load.o(.bss)
    }
}
