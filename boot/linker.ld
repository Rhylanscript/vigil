ENTRY(stage2_start)

SECTIONS
{
    . = 0x7e00;

    .text : {
        boot/stage2.o(.text)
        boot/kernel.o(.text)
        boot/terminal.o(.text)
        boot/isr_asm.o(.text)
        boot/idt.o(.text)
        boot/idt_load.o(.text)
    }

    .rodata : {
        boot/stage2.o(.rodata)
        boot/kernel.o(.rodata)
        boot/terminal.o(.rodata)
        boot/isr.o(.rodata)
        boot/idt.o(.rodata)
        boot/idt_load.o(.rodata)
    }

    .data : {
        boot/stage2.o(.data)
        boot/kernel.o(.data)
        boot/terminal.o(.data)
        boot/isr.o(.data)
        boot/idt.o(.data)
        boot/idt_load.o(.data)
    }

    .bss : {
        boot/stage2.o(.bss)
        boot/kernel.o(.bss)
        boot/terminal.o(.bss)
        boot/isr.o(.bss)
        boot/idt.o(.bss)
        boot/idt_load.o(.bss)
    }
}
