// boot/irq.c

#include "irq.h"
#include "io.h"

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

static irq_handler_t irq_handlers[16];

void irq_install_handler(int irq, irq_handler_t handler) {
    irq_handlers[irq] = handler;
}

void irq_install(void) {
    idt_set_gate(32, (unsigned int) irq0, 0x08, 0x8e);
    idt_set_gate(33, (unsigned int) irq1, 0x08, 0x8e);
    idt_set_gate(34, (unsigned int) irq2, 0x08, 0x8e);
    idt_set_gate(35, (unsigned int) irq3, 0x08, 0x8e);
    idt_set_gate(36, (unsigned int) irq4, 0x08, 0x8e);
    idt_set_gate(37, (unsigned int) irq5, 0x08, 0x8e);
    idt_set_gate(38, (unsigned int) irq6, 0x08, 0x8e);
    idt_set_gate(39, (unsigned int) irq7, 0x08, 0x8e);
    idt_set_gate(40, (unsigned int) irq8, 0x08, 0x8e);
    idt_set_gate(41, (unsigned int) irq9, 0x08, 0x8e);
    idt_set_gate(42, (unsigned int) irq10, 0x08, 0x8e);
    idt_set_gate(43, (unsigned int) irq11, 0x08, 0x8e);
    idt_set_gate(44, (unsigned int) irq12, 0x08, 0x8e);
    idt_set_gate(45, (unsigned int) irq13, 0x08, 0x8e);
    idt_set_gate(46, (unsigned int) irq14, 0x08, 0x8e);
    idt_set_gate(47, (unsigned int) irq15, 0x08, 0x8e);
}

void irq_handler(struct registers regs) {
    if (regs.int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);

    int irq_num = regs.int_no - 32;
    if (irq_handlers[irq_num] != 0) {
        irq_handlers[irq_num](regs);
    }
}
