// boot/mem/heap.c

#include "heap.h"

#define HEAP_SIZE (1024 * 1024) // 1mb

typedef struct block_header {
    uint32_t size;
    int free;
    struct block_header* next;
} block_header_t;

static uint8_t heap_pool[HEAP_SIZE];
static block_header_t* heap_start;

void heap_init(void) {
    heap_start = (block_header_t*) heap_pool;
    heap_start->size = HEAP_SIZE - sizeof(block_header_t);
    heap_start->free = 1;
    heap_start->next = 0;
    // pool is one free block, nothing allocated yet 
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return 0;
    }

    size = (size + 3) & ~((size_t) 3);

    block_header_t* current = heap_start;
    while (current != 0) {
        if (current->free && current->size >= size) {
            if (current->size >= size + sizeof(block_header_t) + 4) {
                block_header_t* new_block = (block_header_t*) ((uint8_t*) current + sizeof(block_header_t) + size);
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }

            current->free = 0;
            return (uint8_t*) current + sizeof(block_header_t);
        }
        current = current->next;
    }
    return 0;
}

void kfree(void* ptr) {
    if (ptr == 0) {
        return;
    }

    block_header_t* block = (block_header_t*) ((uint8_t*) ptr - sizeof(block_header_t));
    block->free = 1;

    block_header_t* current = heap_start;
    while (current != 0 && current->next != 0) {
        if (current->free && current->next->free) {
            current->size += sizeof(block_header_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void heap_get_stats(uint32_t* out_total, uint32_t* out_free) {
    uint32_t total = 0;
    uint32_t free_bytes = 0;

    block_header_t* current = heap_start;
    while (current != 0) {
        total += current->size;
        if (current->free) {
            free_bytes += current->size;
        }
        current = current->next;
    }

    *out_total = total;
    *out_free = free_bytes;
}
