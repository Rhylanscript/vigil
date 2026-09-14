// boot/vigil/memory.h

#ifndef VIGIL_MEMORY_H
#define VIGIL_MEMORY_H
#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint32_t boot_count;
} vigil_state_t;

void vigil_memory_load(vigil_state_t* state);
void vigil_memory_save(const vigil_state_t* state);

#endif
