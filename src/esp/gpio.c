#include "autoconf.h"

#include "esp/gpio.h"
#include "driver/gpio.h"
#include "hal/gpio_ll.h"

void
gpio_out_toggle_noirq(struct gpio_out g)
{
    if (g.pin < 32) {
        GPIO.out ^= g.bit;
    } else {
        GPIO.out1.data ^= g.bit;
    }
}

void
gpio_out_toggle(struct gpio_out g)
{
    gpio_out_toggle_noirq(g);
}

struct gpio_out gpio_out_setup(uint8_t pin, uint8_t val) {
    struct gpio_out g;
    g.pin = pin;
    if (pin < 32) {
        g.bit = (1UL << pin);
    } else {
        g.bit = (1UL << (pin-32));
    }
    gpio_out_reset(g, val);
    return g;
}

void gpio_out_reset(struct gpio_out g, uint8_t val) {
    gpio_config_t cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = g.bit,
        .pull_down_en = 0,
        .pull_up_en = 0
    };

    gpio_config(&cfg);
    gpio_set_level(g.pin, val);
}

void gpio_out_write(struct gpio_out g, uint8_t val) {
    gpio_set_level(g.pin, val);
}

struct gpio_in gpio_in_setup(uint8_t pin, int8_t pull_up) {
    struct gpio_in g;
    g.pin = pin;
    if (pin < 32) {
        g.bit = (1UL << pin);
    } else {
        g.bit = (1UL << (pin-32));
    }
    gpio_in_reset(g, pull_up);
    return g;
}

void
gpio_in_reset(struct gpio_in g, int8_t pull_up)
{
    gpio_config_t cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = g.bit,
        .pull_down_en = pull_up < 0,
        .pull_up_en = pull_up > 0
    };

    gpio_config(&cfg);
}

uint8_t
gpio_in_read(struct gpio_in g)
{
    return gpio_get_level(g.pin);
}
