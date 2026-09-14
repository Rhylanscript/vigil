// boot/lib/kstring.h

#ifndef KSTRING_H
#define KSTRING_H
#include <stdint.h>

int kstrcmp(const char* a, const char* b);
int kstrncmp(const char* a, const char* b, int n);

// converts unsigned int into decimal string
void kuitoa(uint32_t value, char* buffer);

#endif
