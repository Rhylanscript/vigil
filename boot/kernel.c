#include "terminal.h"
#include "idt.h"

void kernel_main(void) {
    terminal_initialize();
    terminal_print("VIGIL kernel online\n");

    idt_install();
    terminal_print("IDT installed\n");

    for (;;) {
        
    }
}