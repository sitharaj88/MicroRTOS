/**
 * MicroRTOS Example - Producer-Consumer with Queue
 *
 * This example demonstrates:
 * - Message queues for inter-task communication
 * - Producer-consumer pattern
 * - Mutex for shared resource protection
 * - Multiple producers, single consumer
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "micrortos.h"
#include <string.h>

/*===========================================================================*/
/* Message Definition                                                         */
/*===========================================================================*/

typedef struct {
    uint8_t  producer_id;
    uint16_t sequence;
    uint32_t timestamp;
    uint8_t  data[8];
} message_t;

/*===========================================================================*/
/* Queue and Sync Primitives                                                  */
/*===========================================================================*/

/* Message queue: holds up to 8 messages */
static mr_queue_t message_queue;
static uint8_t queue_buffer[sizeof(message_t) * 8];

/* Mutex for protecting shared counter */
static mr_mutex_t counter_mutex;
static volatile uint32_t total_messages = 0;

/*===========================================================================*/
/* Task Definitions                                                           */
/*===========================================================================*/

/* Producer 1 */
static mr_tcb_t producer1_tcb;
static uint8_t producer1_stack[192];

/* Producer 2 */
static mr_tcb_t producer2_tcb;
static uint8_t producer2_stack[192];

/* Consumer */
static mr_tcb_t consumer_tcb;
static uint8_t consumer_stack[256];

/*===========================================================================*/
/* Producer Task                                                              */
/*===========================================================================*/

static void producer_task(void *arg)
{
    uint8_t producer_id = (uint8_t)(uintptr_t)arg;
    uint16_t sequence = 0;
    message_t msg;
    mr_status_t status;

    while (1) {
        /* Prepare message */
        msg.producer_id = producer_id;
        msg.sequence = sequence++;
        msg.timestamp = mr_tick_get();

        /* Fill data with pattern */
        for (int i = 0; i < 8; i++) {
            msg.data[i] = producer_id + i;
        }

        /* Send to queue (wait up to 100ms if full) */
        status = mr_queue_send(&message_queue, &msg, MR_MS_TO_TICKS(100));

        if (status == MR_OK) {
            /* Update shared counter with mutex protection */
            mr_mutex_lock(&counter_mutex, MR_WAIT_FOREVER);
            total_messages++;
            mr_mutex_unlock(&counter_mutex);
        }

        /* Producer rate depends on ID */
        if (producer_id == 1) {
            mr_task_delay(MR_MS_TO_TICKS(100));  /* 10 msg/sec */
        } else {
            mr_task_delay(MR_MS_TO_TICKS(150));  /* ~6.7 msg/sec */
        }
    }
}

/*===========================================================================*/
/* Consumer Task                                                              */
/*===========================================================================*/

static void consumer_task(void *arg)
{
    (void)arg;
    message_t msg;
    mr_status_t status;
    uint32_t processed = 0;

    while (1) {
        /* Wait for message (block indefinitely) */
        status = mr_queue_receive(&message_queue, &msg, MR_WAIT_FOREVER);

        if (status == MR_OK) {
            processed++;

            /*
             * Process the message here.
             * In a real application, you might:
             * - Parse the data
             * - Update a display
             * - Log to serial/flash
             * - Control actuators
             */

            /* Simulate processing time */
            mr_task_delay(MR_MS_TO_TICKS(10));
        }
    }
}

/*===========================================================================*/
/* Main Entry Point                                                           */
/*===========================================================================*/

int main(void)
{
    /* Initialize the RTOS kernel */
    mr_kernel_init();

    /* Initialize the message queue */
    mr_queue_init(
        &message_queue,
        queue_buffer,
        sizeof(message_t),
        8   /* capacity: 8 messages */
    );

    /* Initialize the mutex */
    mr_mutex_init(&counter_mutex);

    /* Create Producer 1 (priority 3) */
    mr_task_create(
        &producer1_tcb,
        "Prod1",
        producer_task,
        (void *)1,      /* producer_id = 1 */
        3,
        producer1_stack,
        sizeof(producer1_stack)
    );

    /* Create Producer 2 (priority 3) */
    mr_task_create(
        &producer2_tcb,
        "Prod2",
        producer_task,
        (void *)2,      /* producer_id = 2 */
        3,
        producer2_stack,
        sizeof(producer2_stack)
    );

    /* Create Consumer (priority 2, higher than producers) */
    mr_task_create(
        &consumer_tcb,
        "Consumer",
        consumer_task,
        NULL,
        2,
        consumer_stack,
        sizeof(consumer_stack)
    );

    /* Start the scheduler */
    mr_kernel_start();

    return 0;
}
