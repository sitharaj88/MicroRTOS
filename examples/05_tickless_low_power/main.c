/**
 * MicroRTOS Example - Tickless Low Power Mode
 *
 * This example demonstrates:
 * - Tickless idle for battery-powered applications
 * - Power consumption reduction during idle periods
 * - Automatic wake on task timeout or external interrupt
 *
 * Hardware: Any Arduino board
 * - LED on pin 13 (built-in) shows activity
 * - Serial output for status messages
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "micrortos.h"

/* Ensure tickless is enabled */
#if !MR_USE_TICKLESS_IDLE
    #warning "This example requires MR_USE_TICKLESS_IDLE=1 in mr_config.h"
#endif

#ifdef __AVR__
#include <avr/io.h>
#define LED_PIN     PB5     /* Pin 13 on Arduino Uno */
#define LED_DDR     DDRB
#define LED_PORT    PORTB
#define LED_ON()    (LED_PORT |= (1 << LED_PIN))
#define LED_OFF()   (LED_PORT &= ~(1 << LED_PIN))
#define LED_TOGGLE() (LED_PORT ^= (1 << LED_PIN))
#elif defined(__arm__)
/* ARM - implementation depends on specific board */
#define LED_ON()
#define LED_OFF()
#define LED_TOGGLE()
#endif

/*===========================================================================*/
/* Task Definitions                                                           */
/*===========================================================================*/

/* Sensor task - samples periodically with long sleep between */
static mr_tcb_t sensor_tcb;
static uint8_t sensor_stack[192];

/* Monitor task - wakes up occasionally to check system health */
static mr_tcb_t monitor_tcb;
static uint8_t monitor_stack[192];

/*===========================================================================*/
/* Power Statistics                                                           */
/*===========================================================================*/

static volatile uint32_t total_active_ticks = 0;
static volatile uint32_t sensor_readings = 0;

/*===========================================================================*/
/* Sensor Task                                                                */
/*===========================================================================*/

/**
 * Simulated sensor sampling task.
 * Wakes up every 5 seconds to read sensors, then sleeps.
 * During sleep, the system enters tickless low-power mode.
 */
static void sensor_task(void *arg)
{
    (void)arg;
    uint32_t wake_tick;
    uint32_t sample_value = 0;

    while (1) {
        wake_tick = mr_tick_get();

        /* Indicate sensor activity */
        LED_ON();

        /*
         * Simulate sensor reading.
         * In a real application, you would:
         * - Power on the sensor
         * - Wait for stabilization
         * - Read ADC values
         * - Process data
         * - Power off the sensor
         */
        sample_value++;
        sensor_readings++;

        /* Simulate processing time */
        mr_task_delay(MR_MS_TO_TICKS(50));

        LED_OFF();

        /* Track active time */
        total_active_ticks += (mr_tick_get() - wake_tick);

        /*
         * Sleep for 5 seconds.
         * During this time, the RTOS will enter tickless idle mode
         * and stop the periodic tick interrupt to save power.
         */
        mr_task_delay(MR_MS_TO_TICKS(5000));
    }
}

/*===========================================================================*/
/* Monitor Task                                                               */
/*===========================================================================*/

/**
 * System health monitor task.
 * Wakes up every 30 seconds to check system status.
 * Demonstrates very long sleep periods with tickless mode.
 */
static void monitor_task(void *arg)
{
    (void)arg;
    uint32_t check_count = 0;

    while (1) {
        check_count++;

        /*
         * In a real application, you might:
         * - Check battery level
         * - Verify sensor health
         * - Transmit accumulated data
         * - Clear watchdog
         */

        /* Brief LED flash to indicate monitor activity */
        LED_TOGGLE();
        mr_task_delay(MR_MS_TO_TICKS(10));
        LED_TOGGLE();

#if MR_USE_TICKLESS_IDLE
        /*
         * Report tickless statistics.
         * In a real app, this could go to serial or be logged.
         */
        uint32_t sleep_count = mr_tickless_get_sleep_count();
        uint32_t total_slept = mr_tickless_get_total_slept();
        (void)sleep_count;
        (void)total_slept;
#endif

        /* Sleep for 30 seconds */
        mr_task_delay(MR_MS_TO_TICKS(30000));
    }
}

/*===========================================================================*/
/* Tickless Hooks (Optional)                                                  */
/*===========================================================================*/

#if MR_USE_TICKLESS_HOOKS

/**
 * Called before entering tickless sleep.
 * Prepare peripherals for low power mode.
 */
void mr_tickless_pre_sleep_hook(uint32_t expected_ticks)
{
    (void)expected_ticks;

    /*
     * In a real application, you could:
     * - Disable unused peripherals
     * - Reduce clock speed
     * - Put sensors to sleep
     * - Disable ADC
     */
}

/**
 * Called after waking from tickless sleep.
 * Restore peripheral states.
 */
void mr_tickless_post_sleep_hook(uint32_t slept_ticks)
{
    (void)slept_ticks;

    /*
     * In a real application, you could:
     * - Re-enable peripherals
     * - Restore clock speed
     * - Wake sensors
     */
}

#endif /* MR_USE_TICKLESS_HOOKS */

/*===========================================================================*/
/* Hardware Setup                                                             */
/*===========================================================================*/

static void hardware_init(void)
{
#ifdef __AVR__
    /* Set LED pin as output */
    LED_DDR |= (1 << LED_PIN);

    /* Start with LED off */
    LED_OFF();
#endif
}

/*===========================================================================*/
/* Main Entry Point                                                           */
/*===========================================================================*/

int main(void)
{
    /* Initialize hardware */
    hardware_init();

    /* Initialize the RTOS kernel */
    mr_kernel_init();

#if MR_USE_TICKLESS_IDLE
    /* Initialize and enable tickless mode */
    mr_tickless_init();
    mr_tickless_enable(true);

    /* Use idle sleep mode (fastest wake, moderate power savings) */
    mr_tickless_set_sleep_mode(MR_SLEEP_IDLE);

    /*
     * For deeper sleep (more power savings, slower wake):
     * mr_tickless_set_sleep_mode(MR_SLEEP_STANDBY);
     *
     * For deepest sleep (best power savings, slowest wake):
     * mr_tickless_set_sleep_mode(MR_SLEEP_POWER_DOWN);
     */
#endif

    /* Create sensor task (priority 3) */
    mr_task_create(
        &sensor_tcb,
        "Sensor",
        sensor_task,
        NULL,
        3,
        sensor_stack,
        sizeof(sensor_stack)
    );

    /* Create monitor task (priority 4, lower than sensor) */
    mr_task_create(
        &monitor_tcb,
        "Monitor",
        monitor_task,
        NULL,
        4,
        monitor_stack,
        sizeof(monitor_stack)
    );

    /* Start the scheduler */
    mr_kernel_start();

    return 0;
}
