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

void kernel_main(void) {
    boot_info_t* info = boot_info();
    uint8_t* framebuffer = (uint8_t*) info->framebuffer_addr;

    for (uint32_t y = 0; y < info->height; y++) {
        uint8_t* row = framebuffer + (y * info->pitch);
        for (uint32_t x = 0; x < info->width; x++) {
            uint8_t* pixel = row + (x * 3);
            pixel[0] = 0x00;
            pixel[1] = 0xff;
            pixel[2] = 0x00;
        }
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }

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
