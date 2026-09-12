// boot/kernel.c

#include "terminal.h"
#include "idt.h"
#include "irq.h"
#include "pic.h"
#include "keyboard.h"
#include "io.h"

void kernel_main(void) {
    terminal_initialize();
    terminal_print("VIGIL kernel online\n");

    idt_install();
    terminal_print("IDT installed\n");

    pic_remap();
    irq_install();
    keyboard_install();
    enable_interrupts();
    terminal_print("Keyboard ready - type something:\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}