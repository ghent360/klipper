#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "autoconf.h"

#include "esp/irq.h" // irq_disable
#include "esp/misc.h" // timer_from_us
#include "command.h" // DECL_CONSTANT
#include "internal.h" // console_sleep
#include "sched.h" // DECL_INIT

DECL_CONSTANT("CLOCK_FREQ", CONFIG_CLOCK_FREQ);

// Return true if time1 is before time2.  Always use this function to
// compare times as regular C comparisons can fail if the counter
// rolls over.
uint8_t
timer_is_before(uint32_t time1, uint32_t time2)
{
    return (int32_t)(time1 - time2) < 0;
}

// Activate timer dispatch as soon as possible
void
timer_kick(void)
{
}

void
timer_init(void)
{
}
DECL_INIT(timer_init);

void
timer_reset(void)
{
}
DECL_SHUTDOWN(timer_reset);

uint32_t
timer_read_time(void) {
    return xTaskGetTickCount();
}

// Return the number of clock ticks for a given number of microseconds
uint32_t
timer_from_us(uint32_t us)
{
    return us / 1000;
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
