// boot/mem/heap.h

#ifndef HEAP_H
#define HEAP_H
#include <stddef.h>
#include <stdint.h>

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void heap_get_stats(uint32_t* out_total, uint32_t* out_free);

#endif
