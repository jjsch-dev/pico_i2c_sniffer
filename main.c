/**
 * i2c bus sniffer pico.
 * Sniffer for an i2c bus using the PIO of Raspberry Pi Pico (RP2040)
 * (C) Juan Schiavoni 2021
 *
 * It is composed of 4 state machines that communicate through an IRQ and 
 * two auxiliary pins. Three of them decode the START / STOP / DATA condition, 
 * and the last one serializes the events into a single FIFO; and reads the value 
 * of the 8-bit data plus the ACK.
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "i2c_sniffer.pio.h"
#include "pico/multicore.h"

#include "ram_fifo.h"

#undef PRINT_VAL
#define PRINT_TIME_T

const uint led_pin = 25;

void core1_print() {
    // Blocks the CPU waiting for FIFO values from the core 0.
    while (true)
    {
        uint32_t val = multicore_fifo_pop_blocking();
        
        gpio_put(led_pin, true);

        // The format of the uint32_t returned by the sniffer is composed of two event
        // code bits (EV1 = Bit12, EV0 = Bit11), and when it comes to data, the nine least
        // significant bits correspond to (ACK = Bit0), and the value 8 bits
        // where (B0 = Bit1 and B7 = Bit8).
        uint32_t ev_code = (val >> 10) & 0x03;
        uint8_t  data = ((val >> 1) & 0xFF);
        bool ack = (val & 1) ? false : true;

#ifdef PRINT_VAL
        printf("val: %x, ev_code: %x, data:%x, ack: %d \r\n", val, ev_code, data, ack);
#else
        if (ev_code == EV_START) {
#ifdef PRINT_TIME_T
            printf("%010lu ", time_us_32());
#endif
            printf("s");
        } else if (ev_code == EV_STOP) {
            printf("o\r\n");
        } else if (ev_code == EV_DATA) {
            printf("%02X%c", data, ack ? 'a' : 'n');
        } else {
            printf("u");
        }
#endif
        gpio_put(led_pin, false);
    }
}

int main()
{
    // Initialize LED pin
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin, true);

    // Full speed for the PIO clock divider
    float div = 1;
    PIO pio = pio0;

    // Initialize chosen serial port
    stdio_init_all();

    // Initialize the four state machines that decode the i2c bus states.
    uint sm_main = pio_claim_unused_sm(pio, true);
    uint offset_main = pio_add_program(pio, &i2c_main_program);
    i2c_main_program_init(pio, sm_main, offset_main, div);

    uint sm_data = pio_claim_unused_sm(pio, true);
    uint offset_data = pio_add_program(pio, &i2c_data_program);
    i2c_stop_program_init(pio, sm_data, offset_data, div);

    uint sm_start = pio_claim_unused_sm(pio, true);
    uint offset_start = pio_add_program(pio, &i2c_start_program);
    i2c_start_program_init(pio, sm_start, offset_start, div);

    uint sm_stop = pio_claim_unused_sm(pio, true);
    uint offset_stop = pio_add_program(pio, &i2c_stop_program);
    i2c_stop_program_init(pio, sm_stop, offset_stop, div);

    // Start running our PIO program in the state machine
    pio_sm_set_enabled(pio, sm_main, true);
    pio_sm_set_enabled(pio, sm_start, true);
    pio_sm_set_enabled(pio, sm_stop, true);
    pio_sm_set_enabled(pio, sm_data, true);

    multicore_launch_core1(core1_print);

    uint32_t capture_val = 0;

    ram_fifo_init(10000);

    printf("i2c sniffer pico initialiced!\r\n");

    // Blocks the CPU waiting for FIFO values from the main machine.
    while (true) {
        bool new_val = pio_sm_get_rx_fifo_level(pio, sm_main);
        if (new_val) {
            capture_val = pio_sm_get(pio, sm_main);
        }

        if (multicore_fifo_wready()) {
            if (!ram_fifo_empty()) {
                if (new_val) {
                    ram_fifo_set(capture_val);
                }
                capture_val = ram_fifo_get();
                new_val = true;
            }    
            
            if (new_val) {
                multicore_fifo_push_blocking(capture_val);
            }
        } else if (new_val) {
            ram_fifo_set(capture_val);   
        }
    }
}