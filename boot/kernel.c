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
#include "timer.h"
#include "heap.h"
#include "bootinfo.h"
#include "graphics.h"
#include "font.h"

void kernel_main(void) {
    // temporary test

    gfx_init();
    gfx_clear((gfx_color_t){0x10, 0x10, 0x10}); // dark grey, not pure black, so we can tell "cleared" apart from "still uninitialized memory"

    const char* test_message = "VIGIL graphics online";
    uint32_t x = 16;
    uint32_t y = 16;
    for (int i = 0; test_message[i] != '\0'; i++) {
        gfx_draw_char(x, y, test_message[i], GFX_WHITE);
        x += FONT_WIDTH; // advance one glyph's width for the next character
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }

    // end temp test

    terminal_initialize();
    terminal_print("VIGIL kernel online\n");

    heap_init();

    vigil_state_t state;
    vigil_memory_load(&state);
    state.boot_count++;
    vigil_memory_save(&state);

    idt_install();
    terminal_print("IDT installed\n");

    pic_remap();
    irq_install();
    keyboard_install();
    timer_install(100);
    enable_interrupts();

    shell_init();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
