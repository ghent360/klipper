#ifndef __ESP_MISC_H
#define __ESP_MISC_H

#include <stdarg.h> // va_list
#include <stdint.h> // uint8_t
#include "freertos/FreeRTOS.h"
#include "autoconf.h"

struct command_encoder;
void console_sendf(const struct command_encoder *ce, va_list args);
void *console_receive_buffer(void);

// Return the number of clock ticks for a given number of microseconds
inline uint32_t timer_from_us(uint32_t us)
{
    return (APB_CLK_FREQ / (2 * 1000000)) * us;
}

// Return true if time1 is before time2.  Always use this function to
// compare times as regular C comparisons can fail if the counter
// rolls over.
inline uint8_t
timer_is_before(uint32_t time1, uint32_t time2)
{
    return (int32_t)(time1 - time2) < 0;
}

uint32_t timer_read_time(void);
void timer_kick(void);

void *dynmem_start(void);
void *dynmem_end(void);

uint16_t crc16_ccitt(uint8_t *buf, uint_fast8_t len);

#endif // __ESP_MISC_H
