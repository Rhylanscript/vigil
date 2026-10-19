// apps/calculator/calculator.h

#ifndef CALCULATOR_H
#define CALCULATOR_H
#include <stdint.h>

void calculator_start(void);
void calculator_redraw(void);

void calculator_handle_char(char c);
void calculator_handle_click(int32_t x, int32_t y);

#endif
