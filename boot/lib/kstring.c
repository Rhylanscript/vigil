// boot/lib/kstring.c

#include "kstring.h"

int kstrcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char) *a - (unsigned char) *b;
}

int kstrncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return (unsigned char) a[i] - (unsigned char) b[i];
        }
        if (a[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

void kuitoa(uint32_t value, char* buffer) {
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    char digits[10];
    int count = 0;

    while (value > 0) {
        digits[count] = '0' + (value % 10);
        count++;
        value /= 10;
    }

    for (int i = 0; i < count; i++) {
        buffer[i] = digits[count - 1 - i];
    }
    buffer[count] = '\0';
}

void kitoa(int32_t value, char* buffer) {
    int i = 0;
    uint32_t uvalue;

    if (value < 0) {
        buffer[i++] = '-';
        uvalue = (uint32_t) value;
        uvalue = (~uvalue) + 1;
    } else {
        uvalue = (uint32_t) value;
    }

    if (uvalue == 0) {
        buffer[i] = '0';
        buffer[i + 1] = '\0';
        return;
    }

    char digits[10];
    int count = 0;

    while (uvalue > 0) {
        digits[count] = '0' + (uvalue % 10);
        count++;
        uvalue /= 10;
    }

    for (int j = 0; j < count; j++) {
        buffer[i + j] = digits[count - 1 - j];
    }
    buffer[i + count] = '\0';
}

void khtoa(uint32_t value, char* buffer) {
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    const char* hex_chars = "0123456789abcdef";
    char digits[8];
    int count = 0;

    while (value > 0) {
        digits[count] = hex_chars[value % 16];
        count++;
        value /= 16;
    }

    for (int i = 0; i < count; i++) {
        buffer[i] = digits[count - 1 - i];
    }
    buffer[count] = '\0';
}
