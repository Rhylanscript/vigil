// boot/irq.h

#ifndef IRQ_H
#define IRQ_H
#include "idt.h"

typedef void (*irq_handler_t)(struct registers regs);

void irq_install(void);
void irq_install_handler(int irq, irq_handler_t handler);

#endif
