#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp/irq.h" // irq_disable
#include "esp/misc.h" // timer_from_us
#include "command.h" // DECL_CONSTANT
#include "internal.h" // console_sleep
#include "sched.h" // DECL_INIT

uint32_t
timer_read_time(void) {
    return xTaskGetTickCount();
}

void
irq_disable(void)
{
}

void
irq_enable(void)
{
}

irqstatus_t
irq_save(void)
{
    return 0;
}

void
irq_restore(irqstatus_t flag)
{
}

void
irq_wait(void)
{
#if 0    
    // Must atomically sleep until signaled
    if (!readl(&TimerInfo.must_wake_timers)) {
        timer_disable_signals();
        if (!TimerInfo.must_wake_timers)
            console_sleep(&TimerInfo.ss_sleep);
        timer_enable_signals();
    }
#endif
    irq_poll();
}

void
irq_poll(void)
{
#if 0    
    if (readl(&TimerInfo.must_wake_timers))
        timer_dispatch();
#endif        
}
