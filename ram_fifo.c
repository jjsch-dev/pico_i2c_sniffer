/**
 * FIFO in RAM.
 * (C) Juan Schiavoni 2021
 */
#include <stdlib.h>
#include "ram_fifo.h"

static uint32_t *capture_buf = NULL;

static uint32_t capture_set = 0;
static uint32_t capture_get = 0;
static uint32_t capture_count = 0;

bool ram_fifo_init(size_t count) {
    capture_buf  = malloc(count);
    capture_count = count;

    return  (capture_buf != NULL);
}

bool ram_fifo_empty(void){
    return (capture_get == capture_set);   
}

void ram_fifo_set(uint32_t item) {
    if (capture_set >= capture_count){
        capture_set = 0;    
    }

    capture_buf[capture_set++] = item;
}

uint32_t ram_fifo_get(void) {
    if (capture_get >= capture_count){
        capture_get = 0;    
    }

    return capture_buf[capture_get++];
}