// boot/isr.c
#include "idt.h"
#include "terminal.h"

static const char* exception_messages[32] = {
    "Division by zero", "Debug", "Non-maskable interrupt", "Breakpoint",
    "Overflow", "Bound range exceeded", "Invalid opcode",
    "Device not available", "Double fault", "Coprocessor segment overrun",
    "Invalid TSS", "Segment not present", "Stack-segment fault",
    "General protection fault", "Page fault", "Reserved",
    "x87 floating-point exception", "Alignment check", "Machine check",
    "SIMD floating-point exception", "Virtualization exception",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

void isr_handler(struct registers regs) {
    terminal_print("\nVIGIL: exception - ");
    terminal_print(exception_messages[regs.int_no]);
    terminal_print("\n");

    for (;;) {

    }
}