// boot/kernel.c

#include "terminal.h"
#include "idt.h"
#include "irq.h"
#include "pic.h"
#include "keyboard.h"
#include "io.h"
#include "shell.h"
#include "memory.h"
#include "fs.h"

void kernel_main(void) {
    terminal_initialize();
    terminal_print("VIGIL kernel online\n");

    vigil_state_t state;
    vigil_memory_load(&state);
    state.boot_count++;
    vigil_memory_save(&state);

    idt_install();
    terminal_print("IDT installed\n");

    pic_remap();
    irq_install();
    keyboard_install();
    enable_interrupts();

    shell_init();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
