// boot/drivers/timer.c

#include "timer.h"
#include "irq.h"
#include "io.h"

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND        0x43
#define PIT_BASE_FREQUENCY 1193182

static volatile uint32_t tick_count = 0;
static uint32_t configured_frequency = 0;

static void timer_callback(struct registers regs) {
    (void) regs;
    tick_count++;
}

void timer_install(uint32_t frequency) {
    configured_frequency = frequency;
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;

    outb(PIT_COMMAND, 0x36);

    outb(PIT_CHANNEL0_DATA, (uint8_t) (divisor & 0xff));
    outb(PIT_CHANNEL0_DATA, (uint8_t) ((divisor >> 8) & 0xff));

    irq_install_handler(0, timer_callback);
}

uint32_t timer_get_ticks(void) {
    return tick_count;
}

uint32_t timer_get_frequency(void) {
    return configured_frequency;
}

void timer_sleep_ticks(uint32_t ticks) {
    uint32_t target = tick_count + ticks;
    while (tick_count < target) {
        __asm__ volatile ("hlt");
    }
}
