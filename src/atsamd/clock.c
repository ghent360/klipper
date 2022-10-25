// Code to setup peripheral clocks on the SAMD21
//
// Copyright (C) 2018-2019  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "command.h" // DECL_CONSTANT_STR
#include "compiler.h" // DIV_ROUND_CLOSEST
#include "internal.h" // enable_pclock

// The "generic clock generators" that are configured
#define CLKGEN_MAIN 0
#define CLKGEN_ULP32K 2

#define FREQ_MAIN 48000000
#define FREQ_32K 32768

// Configure a clock generator using a given source as input
static inline void
gen_clock(uint32_t clkgen_id, uint32_t flags)
{
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(clkgen_id);
    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(clkgen_id) | flags | GCLK_GENCTRL_GENEN;
}

// Route a peripheral clock to a given clkgen
static inline void
route_pclock(uint32_t pclk_id, uint32_t clkgen_id)
{
    GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_ID(pclk_id)
                         | GCLK_CLKCTRL_GEN(clkgen_id) | GCLK_CLKCTRL_CLKEN);
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY)
        ;
}

// Enable a peripheral clock and power to that peripheral
void
enable_pclock(uint32_t pclk_id, int32_t pm_id)
{
    route_pclock(pclk_id, CLKGEN_MAIN);
    if (pm_id >= 0) {
        uint32_t pm_port = pm_id / 32, pm_bit = 1 << (pm_id % 32);
        (&PM->APBAMASK.reg)[pm_port] |= pm_bit;
    }
}

// Return the frequency of the given peripheral clock
uint32_t
get_pclock_frequency(uint32_t pclk_id)
{
    return FREQ_MAIN;
}

#if CONFIG_CLOCK_REF_X32K
DECL_CONSTANT_STR("RESERVE_PINS_crystal", "PA0,PA1");
#endif

// Initialize the clocks using an external 32K crystal
static void
clock_init_32k(void)
{
#if 0    
    // Enable external 32Khz crystal
    uint32_t val = (SYSCTRL_XOSC32K_STARTUP(6)
                    | SYSCTRL_XOSC32K_XTALEN | SYSCTRL_XOSC32K_EN32K);
    SYSCTRL->XOSC32K.reg = val;
    SYSCTRL->XOSC32K.reg = val | SYSCTRL_XOSC32K_ENABLE;
    while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_XOSC32KRDY))
        ;

    // Generate 48Mhz clock on DPLL (with XOSC32K as reference)
    SYSCTRL->DPLLCTRLA.reg = 0;
    uint32_t mul = DIV_ROUND_CLOSEST(FREQ_MAIN, FREQ_32K);
    SYSCTRL->DPLLRATIO.reg = SYSCTRL_DPLLRATIO_LDR(mul - 1);
    SYSCTRL->DPLLCTRLB.reg = SYSCTRL_DPLLCTRLB_LBYPASS;
    SYSCTRL->DPLLCTRLA.reg = SYSCTRL_DPLLCTRLA_ENABLE;
    uint32_t mask = SYSCTRL_DPLLSTATUS_CLKRDY | SYSCTRL_DPLLSTATUS_LOCK;
    while ((SYSCTRL->DPLLSTATUS.reg & mask) != mask)
        ;

    // Switch main clock to DPLL clock
    gen_clock(CLKGEN_MAIN, GCLK_GENCTRL_SRC_DPLL96M);
#else
    /* The following code is sourced from
       https://blog.thea.codes/understanding-the-sam-d21-clocks/
     */
    SYSCTRL->XOSC32K.reg =
        /* Crystal oscillators can take a long time to startup. This
        waits the maximum amount of time (4 seconds). This can be
        reduced depending on your crystal oscillator. */
        SYSCTRL_XOSC32K_STARTUP(0x7) |
        SYSCTRL_XOSC32K_EN32K |
        SYSCTRL_XOSC32K_XTALEN;

    /* This has to be a separate write as per datasheet section 17.6.3 */
    SYSCTRL->XOSC32K.bit.ENABLE = 1;

    /* Wait for the external crystal to be ready */
    while(!SYSCTRL->PCLKSR.bit.XOSC32KRDY);

    /* Configure GCLK1's divider - in this case, no division - so just divide by one */
    GCLK->GENDIV.reg =
        GCLK_GENDIV_ID(1) |
        GCLK_GENDIV_DIV(1);

    /* Setup GCLK1 using the external 32.768 kHz oscillator */
    GCLK->GENCTRL.reg = 
        GCLK_GENCTRL_ID(1) |
        GCLK_GENCTRL_SRC_XOSC32K |
        /* Improve the duty cycle. */
        GCLK_GENCTRL_IDC |
        GCLK_GENCTRL_GENEN;

    /* Wait for the write to complete */
    while(GCLK->STATUS.bit.SYNCBUSY);

    /* Configure GCLK1 as source for DFLL48 */
    GCLK->CLKCTRL.reg =
        GCLK_CLKCTRL_ID_DFLL48 |
        GCLK_CLKCTRL_GEN_GCLK1 |
        GCLK_CLKCTRL_CLKEN;

    /* This works around a quirk in the hardware (errata 1.2.1) -
    the DFLLCTRL register must be manually reset to this value before
    configuration. */
    while(!SYSCTRL->PCLKSR.bit.DFLLRDY);
    SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
    while(!SYSCTRL->PCLKSR.bit.DFLLRDY);

    /* Set up the multiplier. This tells the DFLL to multiply the 32.768 kHz
    reference clock to 48 MHz */
    SYSCTRL->DFLLMUL.reg =
        /* This value is output frequency / reference clock frequency so 
           48 MHz / 32.768 kHz */
        SYSCTRL_DFLLMUL_MUL(1465) |
        /* The coarse and fine step are used by the DFLL to lock on to the target
           frequency. These are set to half of the maximum value. Lower values
           mean less overshoot, whereas higher values typically result in some
           overshoot but faster locking. */
        SYSCTRL_DFLLMUL_FSTEP(511) | // max value: 1023
        SYSCTRL_DFLLMUL_CSTEP(31);   // max value: 63

    /* Wait for the write to finish */
    while(!SYSCTRL->PCLKSR.bit.DFLLRDY);

    /* Load factory calibration value */
    uint32_t coarse = (*((uint32_t *)FUSES_DFLL48M_COARSE_CAL_ADDR)
        & FUSES_DFLL48M_COARSE_CAL_Msk) >> FUSES_DFLL48M_COARSE_CAL_Pos;
    SYSCTRL->DFLLVAL.bit.COARSE = coarse;
    /* Wait for the write to finish */
    while(!SYSCTRL->PCLKSR.bit.DFLLRDY);

    SYSCTRL->DFLLCTRL.reg |= 
        /* Closed loop mode */
        SYSCTRL_DFLLCTRL_MODE |
        /* Wait for the frequency to be locked before outputting the clock */
        SYSCTRL_DFLLCTRL_WAITLOCK |
        /* Enable it */
        SYSCTRL_DFLLCTRL_ENABLE;

    /* Wait for the frequency to lock */
    while (!SYSCTRL->PCLKSR.bit.DFLLLCKC || !SYSCTRL->PCLKSR.bit.DFLLLCKF);

    /* Setup GCLK0 using the DFLL @ 48 MHz */
    GCLK->GENCTRL.reg =
        GCLK_GENCTRL_ID(0) |
        GCLK_GENCTRL_SRC_DFLL48M |
        /* Improve the duty cycle. */
        GCLK_GENCTRL_IDC |
        GCLK_GENCTRL_GENEN;

    /* Wait for the write to complete */
    while(GCLK->STATUS.bit.SYNCBUSY);
#endif    
}

// Initialize clocks from factory calibrated internal clock
static void
clock_init_internal(void)
{
    // Configure DFLL48M clock in open loop mode
    SYSCTRL->DFLLCTRL.reg = 0;
    uint32_t coarse = GET_FUSE(FUSES_DFLL48M_COARSE_CAL);
    uint32_t fine = GET_FUSE(FUSES_DFLL48M_FINE_CAL);
    SYSCTRL->DFLLVAL.reg = (SYSCTRL_DFLLVAL_COARSE(coarse)
                            | SYSCTRL_DFLLVAL_FINE(fine));
    if (CONFIG_USB) {
        // Enable USB clock recovery mode
        uint32_t mul = DIV_ROUND_CLOSEST(FREQ_MAIN, 1000);
        SYSCTRL->DFLLMUL.reg = (SYSCTRL_DFLLMUL_FSTEP(10)
                                | SYSCTRL_DFLLMUL_MUL(mul));
        SYSCTRL->DFLLCTRL.reg = (SYSCTRL_DFLLCTRL_MODE | SYSCTRL_DFLLCTRL_USBCRM
                                 | SYSCTRL_DFLLCTRL_CCDIS
                                 | SYSCTRL_DFLLCTRL_ENABLE);
    } else {
        SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
    }
    while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY))
        ;

    // Switch main clock to DFLL48M clock
    gen_clock(CLKGEN_MAIN, GCLK_GENCTRL_SRC_DFLL48M);
}

void
SystemInit(void)
{
    // Setup flash to work with 48Mhz clock
    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_RWS_HALF;

    // Reset GCLK
    GCLK->CTRL.reg = GCLK_CTRL_SWRST;
    while (GCLK->CTRL.reg & GCLK_CTRL_SWRST)
        ;

    // Init clocks
    if (CONFIG_CLOCK_REF_X32K)
        clock_init_32k();
    else
        clock_init_internal();
}
