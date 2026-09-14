// boot/drivers/timer.h

#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>

void timer_install(uint32_t frequency);
uint32_t timer_get_ticks(void);
uint32_t timer_get_frequency(void);
void timer_sleep_ticks(uint32_t ticks);

#endif
