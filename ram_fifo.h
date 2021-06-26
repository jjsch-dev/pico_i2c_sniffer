/**
 * FIFO in RAM.
 * (C) Juan Schiavoni 2021
 */
#include <stddef.h>
#include "pico/types.h"

bool ram_fifo_init(size_t count);
bool ram_fifo_is_empty(void);
void ram_fifo_set(uint32_t item);
uint32_t ram_fifo_get(void);