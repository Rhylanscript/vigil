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
