// apps/calculator/calc.c

#include "calc.h"

// --- state ---

static int32_t accumulator = 0;
static int32_t entry = 0;

static int fresh = 1;

static calc_op_t pending_op = CALC_OP_NONE;
static calc_error_t error = CALC_OK;

// --- maths ---

static calc_error_t apply(int32_t left, calc_op_t op, int32_t right, int32_t* result) {
    switch (op) {
        case CALC_OP_ADD:
            return __builtin_add_overflow(left, right, result) ? CALC_ERR_OVERFLOW : CALC_OK;
        case CALC_OP_SUB:
            return __builtin_sub_overflow(left, right, result) ? CALC_ERR_OVERFLOW : CALC_OK;
        case CALC_OP_MUL:
            return __builtin_mul_overflow(left, right, result) ? CALC_ERR_OVERFLOW : CALC_OK;

        case CALC_OP_DIV:
        case CALC_OP_MOD:
            if (right == 0) {
                return CALC_ERR_DIV_ZERO;
            }
            if (left == INT32_MIN && right == -1) {
                return CALC_ERR_OVERFLOW;
            }
            *result = (op == CALC_OP_DIV) ? (left / right) : (left % right);
            return CALC_OK;

        default:
            *result = right;
            return CALC_OK;
    }
}

static int resolve_pending(void) {
    int32_t result;
    calc_error_t problem = apply(accumulator, pending_op, entry, &result);

    if (problem != CALC_OK) {
        error = problem;
        return 0;
    }

    accumulator = result;
    entry = result;
    return 1;
}

// --- key presses ---

void calc_clear(void) {
    accumulator = 0;
    entry = 0;
    pending_op = CALC_OP_NONE;
    fresh = 1;
    error = CALC_OK;
}

void calc_press_digit(int digit) {
    if (error != CALC_OK || digit < 0 || digit > 9) {
        return;
    }

    if (fresh) {
        entry = 0;
        fresh = 0;
    }

    int32_t signed_digit = (entry < 0) ? -digit : digit;
    int32_t next;

    if (__builtin_mul_overflow(entry, 10, &next) ||
        __builtin_add_overflow(next, signed_digit, &next)) {
        return;
    }
    entry = next;
}

void calc_press_operator(calc_op_t op) {
    if (error != CALC_OK) {
        return;
    }

    if (pending_op == CALC_OP_NONE) {
        accumulator = entry;
    } else if (!fresh) {
        if (!resolve_pending()) {
            return;
        }
    }
    
    pending_op = op;
    fresh = 1;
}

void calc_press_equals(void) {
    if (error != CALC_OK || pending_op == CALC_OP_NONE) {
        return;
    }

    if (resolve_pending()) {
        pending_op = CALC_OP_NONE;
    }
    fresh = 1;
}

void calc_press_negate(void) {
    if (error != CALC_OK) {
        return;
    }

    if (entry == INT32_MIN) {
        error = CALC_ERR_OVERFLOW;
        return;
    }
    entry = -entry;
}

void calc_press_backspace(void) {
    if (error != CALC_OK || fresh) {
        return;
    }
    entry /= 10;
}

// --- read-only access for the app ---

int32_t calc_display_value(void) { return entry; }
calc_error_t calc_error(void) { return error; }
calc_op_t calc_pending_op(void) { return pending_op; }

// --- helpers ---

char calc_op_symbol(calc_op_t op) {
    switch (op) {
        case CALC_OP_ADD: return '+';
        case CALC_OP_SUB: return '-';
        case CALC_OP_MUL: return '*';
        case CALC_OP_DIV: return '/';
        case CALC_OP_MOD: return '%';
        default:          return 0;
    }
}

calc_op_t calc_op_from_char(char c) {
    for (int op = CALC_OP_ADD; op <= CALC_OP_MOD; op++) {
        if (calc_op_symbol((calc_op_t) op) == c) {
            return (calc_op_t) op;
        }
    }
    return CALC_OP_NONE;
}
