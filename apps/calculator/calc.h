// apps/calculator/calc.h

#ifndef CALC_H
#define CALC_H
#include <stdint.h>

typedef enum {
    CALC_OP_NONE = 0,
    CALC_OP_ADD,
    CALC_OP_SUB,
    CALC_OP_MUL,
    CALC_OP_DIV,
    CALC_OP_MOD,
} calc_op_t;

typedef enum {
    CALC_OK = 0,
    CALC_ERR_DIV_ZERO,
    CALC_ERR_OVERFLOW,
} calc_error_t;

// --- keypresses ---

void calc_clear(void);
void calc_press_digit(int digit);
void calc_press_operator(calc_op_t op);
void calc_press_equals(void);
void calc_press_negate(void);
void calc_press_backspace(void);

// --- draw ---

int32_t calc_display_value(void);
calc_error_t calc_error(void);
calc_op_t calc_pending_op(void);

// --- helpers ---

char calc_op_symbol(calc_op_t op);      // CALC_OP_ADD -> '+' CALC_OP_NONE -> 0
calc_op_t calc_op_from_char(char c);    // '+' -> CALC_OP_ADD, anything else -> CALC_OP_NONE

#endif
