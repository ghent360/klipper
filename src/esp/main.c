// Main starting point for ESP boards.
//
// Copyright (C) 2021  Ghent The Slicer <ghent360@iqury.us>
//
// This file may be distributed under the terms of the MIT license.

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "command.h" // DECL_CONSTANT
#include "sched.h" // sched_main

DECL_CONSTANT_STR("MCU", CONFIG_IDF_TARGET);

void app_main(void)
{
#if 0    
    printf("Hello world\n");
    while(1) {
        vTaskDelay(1000);
    }
#endif    
    sched_main();
}
