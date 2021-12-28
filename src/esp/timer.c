#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "autoconf.h"

#include "esp/irq.h" // irq_disable
#include "esp/misc.h" // timer_from_us
#include "command.h" // DECL_CONSTANT
#include "internal.h" // console_sleep
#include "sched.h" // DECL_INIT

//DECL_CONSTANT("CLOCK_FREQ", CONFIG_CLOCK_FREQ);
DECL_CONSTANT("CLOCK_FREQ", APB_CLK_FREQ);

static const uint32_t ticks_per_us = APB_CLK_FREQ / 1000000;

// Return true if time1 is before time2.  Always use this function to
// compare times as regular C comparisons can fail if the counter
// rolls over.
inline uint8_t IRAM_ATTR
timer_is_before(uint32_t time1, uint32_t time2)
{
    return (int32_t)(time1 - time2) < 0;
}

// Return the number of clock ticks for a given number of microseconds
inline uint32_t IRAM_ATTR
timer_from_us(uint32_t us)
{
    return ticks_per_us * us;
}

static uint32_t IRAM_ATTR
timer_read_time_isr(void) {
    uint64_t time = timer_group_get_counter_value_in_isr(0, 0);
    return (uint32_t)time;
}

static uint32_t timer_repeat_until;
#define TIMER_IDLE_REPEAT_TICKS timer_from_us(500)
#define TIMER_REPEAT_TICKS timer_from_us(100)

#define TIMER_MIN_TRY_TICKS timer_from_us(2)
#define TIMER_DEFER_REPEAT_TICKS timer_from_us(5)

// Invoke timers - called from board irq code.
static uint32_t IRAM_ATTR
timer_dispatch_many(void)
{
    uint32_t tru = timer_repeat_until;
    for (;;) {
        // Run the next software timer
        uint32_t next = sched_timer_dispatch();

        uint32_t now = timer_read_time_isr();
        int32_t diff = next - now;
        if (diff > (int32_t)TIMER_MIN_TRY_TICKS)
            // Schedule next timer normally.
            return next;

        if (unlikely(timer_is_before(tru, now))) {
            // Check if there are too many repeat timers
            if (diff < (int32_t)(-timer_from_us(1000)))
                try_shutdown("Rescheduled timer in the past");
            if (sched_tasks_busy()) {
                timer_repeat_until = now + TIMER_REPEAT_TICKS;
                return now + TIMER_DEFER_REPEAT_TICKS;
            }
            timer_repeat_until = tru = now + TIMER_IDLE_REPEAT_TICKS;
        }

        // Next timer in the past or near future - wait for it to be ready
        irq_enable();
        while (unlikely(diff > 0))
            diff = next - timer_read_time();
        irq_disable();
    }
}

static inline void IRAM_ATTR
timer_set_isr(uint32_t next)
{
    uint64_t time = timer_group_get_counter_value_in_isr(0, 0);
    // clear low 32 bits
    time &= 0xffffffff00000000;
    timer_group_set_alarm_value_in_isr(0, 0, time | next);
}

static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    irq_disable();
    uint32_t next = timer_dispatch_many();
    timer_set_isr(next);
    irq_enable();

    return pdFALSE; // return whether a task switch is needed
}

// Make sure timer_repeat_until doesn't wrap 32bit comparisons
void
timer_task(void)
{
    uint32_t now = timer_read_time();
    irq_disable();
    if (timer_is_before(timer_repeat_until, now))
        timer_repeat_until = now;
    irq_enable();
}
DECL_TASK(timer_task);

static void
timer_set(uint32_t next)
{
    uint64_t time;
    timer_get_counter_value(0, 0, &time);
    // clear low 32 bits
    time &= 0xffffffff00000000;
    ESP_ERROR_CHECK(timer_set_alarm_value(0, 0, time | next));
}

// Activate timer dispatch as soon as possible
void
timer_kick(void)
{
    timer_set(timer_read_time() + 50);
}

void
klipper_timer_init(void)
{
    timer_config_t config = {
        .clk_src = TIMER_SRC_CLK_APB,
        .divider = 1,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = TIMER_AUTORELOAD_DIS,
    };
    ESP_ERROR_CHECK(timer_init(0, 0, &config));

    // For the timer counter to a initial value
    ESP_ERROR_CHECK(timer_set_counter_value(0, 0, 0));
    // Set alarm value and enable alarm interrupt
    //ESP_ERROR_CHECK(timer_set_alarm_value(0, 0, user_data->alarm_value));
    ESP_ERROR_CHECK(timer_enable_intr(0, 0));
    // Hook interrupt callback
    ESP_ERROR_CHECK(timer_isr_callback_add(0, 0, timer_group_isr_callback, NULL, 0));
    // Start timer
    ESP_ERROR_CHECK(timer_start(0, 0));
    timer_kick();
}
DECL_INIT(klipper_timer_init);

uint32_t
timer_read_time(void) {
    uint64_t time;
    timer_get_counter_value(0, 0, &time);
    return (uint32_t)time;
}

void
irq_disable(void)
{
    portDISABLE_INTERRUPTS();
}

void
irq_enable(void)
{
    portENABLE_INTERRUPTS();
}

irqstatus_t
irq_save(void)
{
    return XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
}

void
irq_restore(irqstatus_t flag)
{
    XTOS_RESTORE_INTLEVEL(flag);
}

void
irq_wait(void)
{
    irq_poll();
}

