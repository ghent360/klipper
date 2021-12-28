#ifndef __ESP_IRQ_H__
#define __ESP_IRQ_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef unsigned long irqstatus_t;

void irq_disable(void);
void irq_enable(void);
irqstatus_t irq_save(void);
void irq_restore(irqstatus_t flag);
void irq_wait(void);

static inline void irq_poll(void) {
    vTaskDelay(1);
}

#endif // __ESP_IRQ_H__