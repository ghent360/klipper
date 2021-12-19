#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint32_t
timer_read_time(void) {
    return xTaskGetTickCount();
}