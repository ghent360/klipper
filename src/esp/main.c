// Main starting point for ESP boards.
//
// Copyright (C) 2021  Ghent The Slicer <ghent360@iqury.us>
//
// This file may be distributed under the terms of the MIT license.

#include <stdio.h>
#include "sdkconfig.h"
#include "command.h" // DECL_CONSTANT
#include "sched.h" // sched_main
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "esp/misc.h"

DECL_CONSTANT_STR("MCU", CONFIG_IDF_TARGET);

void app_main(void)
{
/*    
    int t1 = timer_is_before(0xffffffff, 0x20);
    int t2 = timer_is_before(0x20, 0xffffffff);
    printf("test t1= %d, t2=%d\n", t1, t2);
    t1 = timer_is_before(0x7fffffff, 0x80000000);
    t2 = timer_is_before(0x1, 0x7ffffffe);
    printf("test t1= %d, t2=%d\n", t1, t2);
*/    
    sched_main();
}

// Periodically suspend the main app, so the idle hook has opportunity to reset the WDT. 
void idle_task(void) {
    vTaskDelay(1);
}
DECL_TASK(idle_task);
