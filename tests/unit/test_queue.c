/**
 * MicroRTOS Host Unit Tests - Message Queue Module
 *
 * Covers the non-blocking queue paths. Blocking sends/receives need a
 * running scheduler and are out of scope for these host tests.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_types.h"
#include "mr_queue.h"
#include "host_stubs.h"
#include "test_harness.h"

#include <stdint.h>
#include <string.h>

TEST_DEFINE_GLOBALS();

#define ITEM_SIZE   sizeof(uint32_t)
#define CAPACITY    4

static void setup(mr_queue_t *q, uint8_t *storage)
{
    host_stubs_reset();
    memset(storage, 0, ITEM_SIZE * CAPACITY);
    mr_queue_init(q, storage, ITEM_SIZE, CAPACITY);
}

/*===========================================================================*/
/* Tests                                                                      */
/*===========================================================================*/

static void test_init_state(void)
{
    mr_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    CHECK(mr_queue_count(&q) == 0);
    CHECK(mr_queue_spaces_available(&q) == CAPACITY);
    CHECK(mr_queue_is_empty(&q));
    CHECK(!mr_queue_is_full(&q));
}

static void test_send_until_full_from_isr(void)
{
    mr_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    bool yield = true;
    for (uint32_t i = 0; i < CAPACITY; i++) {
        mr_status_t s = mr_queue_send_from_isr(&q, &i, &yield);
        CHECK(s == MR_OK);
    }
    CHECK(mr_queue_count(&q) == CAPACITY);
    CHECK(mr_queue_is_full(&q));

    /* No room: from-ISR returns FULL without blocking. */
    uint32_t extra = 99;
    CHECK(mr_queue_send_from_isr(&q, &extra, &yield) == MR_ERR_FULL);
}

static void test_fifo_order_from_isr(void)
{
    mr_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    bool yield = false;
    for (uint32_t i = 0; i < CAPACITY; i++) {
        CHECK(mr_queue_send_from_isr(&q, &i, &yield) == MR_OK);
    }

    for (uint32_t i = 0; i < CAPACITY; i++) {
        uint32_t out = 0xFFFFFFFFu;
        CHECK(mr_queue_receive_from_isr(&q, &out, &yield) == MR_OK);
        CHECK(out == i);
    }
    CHECK(mr_queue_is_empty(&q));
}

static void test_receive_from_empty_from_isr(void)
{
    mr_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    bool yield = false;
    uint32_t out = 0;
    CHECK(mr_queue_receive_from_isr(&q, &out, &yield) == MR_ERR_EMPTY);
}

static void test_reset_clears_contents(void)
{
    mr_queue_t q;
    uint8_t buf[ITEM_SIZE * CAPACITY];
    setup(&q, buf);

    bool yield = false;
    uint32_t v = 7;
    CHECK(mr_queue_send_from_isr(&q, &v, &yield) == MR_OK);
    CHECK(mr_queue_count(&q) == 1);

    mr_queue_reset(&q);
    CHECK(mr_queue_is_empty(&q));
    CHECK(mr_queue_count(&q) == 0);
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
