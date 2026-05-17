/**
 * MicroRTOS Host Unit Tests - Message Queue Module
 *
 * Covers the non-blocking queue paths. Blocking sends/receives need a
 * running scheduler and are out of scope for these host tests.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_types.h"
#include "rtos_queue.h"
#include "host_stubs.h"
#include "test_harness.h"

#include <stdint.h>
#include <string.h>

TEST_DEFINE_GLOBALS();

#define ITEM_SIZE   sizeof(uint32_t)
#define CAPACITY    4

static void setup(rtos_queue_t *q, uint8_t *storage)
{
    host_stubs_reset();
    memset(storage, 0, ITEM_SIZE * CAPACITY);
    rtos_queue_init(q, storage, ITEM_SIZE, CAPACITY);
}

/*===========================================================================*/
/* Tests                                                                      */
/*===========================================================================*/

static void test_init_state(void)
{
    rtos_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    CHECK(rtos_queue_count(&q) == 0);
    CHECK(rtos_queue_spaces_available(&q) == CAPACITY);
    CHECK(rtos_queue_is_empty(&q));
    CHECK(!rtos_queue_is_full(&q));
}

static void test_send_until_full_from_isr(void)
{
    rtos_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    bool yield = true;
    for (uint32_t i = 0; i < CAPACITY; i++) {
        rtos_status_t s = rtos_queue_send_from_isr(&q, &i, &yield);
        CHECK(s == RTOS_OK);
    }
    CHECK(rtos_queue_count(&q) == CAPACITY);
    CHECK(rtos_queue_is_full(&q));

    /* No room: from-ISR returns FULL without blocking. */
    uint32_t extra = 99;
    CHECK(rtos_queue_send_from_isr(&q, &extra, &yield) == RTOS_ERR_FULL);
}

static void test_fifo_order_from_isr(void)
{
    rtos_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    bool yield = false;
    for (uint32_t i = 0; i < CAPACITY; i++) {
        CHECK(rtos_queue_send_from_isr(&q, &i, &yield) == RTOS_OK);
    }

    for (uint32_t i = 0; i < CAPACITY; i++) {
        uint32_t out = 0xFFFFFFFFu;
        CHECK(rtos_queue_receive_from_isr(&q, &out, &yield) == RTOS_OK);
        CHECK(out == i);
    }
    CHECK(rtos_queue_is_empty(&q));
}

static void test_receive_from_empty_from_isr(void)
{
    rtos_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    bool yield = false;
    uint32_t out = 0;
    CHECK(rtos_queue_receive_from_isr(&q, &out, &yield) == RTOS_ERR_EMPTY);
}

static void test_reset_clears_contents(void)
{
    rtos_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    bool yield = false;
    uint32_t v = 7;
    CHECK(rtos_queue_send_from_isr(&q, &v, &yield) == RTOS_OK);
    CHECK(rtos_queue_count(&q) == 1);

    rtos_queue_reset(&q);
    CHECK(rtos_queue_is_empty(&q));
    CHECK(rtos_queue_count(&q) == 0);
}

/*===========================================================================*/
/* Driver                                                                     */
/*===========================================================================*/

int main(void)
{
    test_init_state();
    test_send_until_full_from_isr();
    test_fifo_order_from_isr();
    test_receive_from_empty_from_isr();
    test_reset_clears_contents();

    TEST_REPORT("queue");
}
