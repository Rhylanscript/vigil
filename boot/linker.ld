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
        build/pic.o(.text)
        build/irq.o(.text)
        build/irq_asm.o(.text)
        build/keyboard.o(.text)
        build/shell.o(.text)
        build/kstring.o(.text)
        build/ata.o(.text)
        build/vigil_memory.o(.text)
        build/fs.o(.text)
    }

    .rodata : {
        build/stage2.o(.rodata)
        build/kernel.o(.rodata)
        build/terminal.o(.rodata)
        build/isr.o(.rodata)
        build/idt.o(.rodata)
        build/idt_load.o(.rodata)
        build/pic.o(.rodata)
        build/irq.o(.rodata)
        build/irq_asm.o(.rodata)
        build/keyboard.o(.rodata)
        build/shell.o(.rodata)
        build/kstring.o(.rodata)
        build/ata.o(.rodata)
        build/vigil_memory.o(.rodata)
        build/fs.o(.rodata)
    }

    .data : {
        build/stage2.o(.data)
        build/kernel.o(.data)
        build/terminal.o(.data)
        build/isr.o(.data)
        build/idt.o(.data)
        build/idt_load.o(.data)
        build/pic.o(.data)
        build/irq.o(.data)
        build/irq_asm.o(.data)
        build/keyboard.o(.data)
        build/shell.o(.data)
        build/kstring.o(.data)
        build/ata.o(.data)
        build/vigil_memory.o(.data)
        build/fs.o(.data)
    }

    .bss : {
        build/stage2.o(.bss)
        build/kernel.o(.bss)
        build/terminal.o(.bss)
        build/isr.o(.bss)
        build/idt.o(.bss)
        build/idt_load.o(.bss)
        build/pic.o(.bss)
        build/irq.o(.bss)
        build/irq_asm.o(.bss)
        build/keyboard.o(.bss)
        build/shell.o(.bss)
        build/kstring.o(.bss)
        build/ata.o(.bss)
        build/vigil_memory.o(.bss)
        build/fs.o(.bss)
    }
}
