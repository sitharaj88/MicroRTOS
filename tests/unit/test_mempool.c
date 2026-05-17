/**
 * MicroRTOS Host Unit Tests - Memory Pool Module
 *
 * Exercises the non-blocking memory-pool paths. The blocking-alloc path
 * is not covered here because it depends on a running scheduler.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_types.h"
#include "rtos_memory.h"
#include "host_stubs.h"
#include "test_harness.h"

#include <stdint.h>
#include <string.h>

TEST_DEFINE_GLOBALS();

/*===========================================================================*/
/* Helpers                                                                    */
/*===========================================================================*/

/* Block big enough to hold a free-list pointer on a 64-bit host. */
#define POOL_BLOCK_SIZE     16
#define POOL_BLOCK_COUNT    4

static void setup(rtos_mempool_t *pool, uint8_t *buffer)
{
    host_stubs_reset();
    memset(buffer, 0xAA, POOL_BLOCK_SIZE * POOL_BLOCK_COUNT);
    rtos_mempool_init(pool, buffer, POOL_BLOCK_SIZE, POOL_BLOCK_COUNT);
}

/*===========================================================================*/
/* Tests                                                                      */
/*===========================================================================*/

static void test_init(void)
{
    rtos_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    setup(&pool, buffer);

    CHECK(rtos_mempool_total(&pool) == POOL_BLOCK_COUNT);
    CHECK(rtos_mempool_available(&pool) == POOL_BLOCK_COUNT);
    CHECK(rtos_mempool_block_size(&pool) >= POOL_BLOCK_SIZE);
    CHECK(!rtos_mempool_is_empty(&pool));
}

static void test_alloc_until_empty(void)
{
    rtos_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    void *blocks[POOL_BLOCK_COUNT];
    setup(&pool, buffer);

    for (int i = 0; i < POOL_BLOCK_COUNT; i++) {
        blocks[i] = rtos_mempool_alloc(&pool, RTOS_NO_WAIT);
        CHECK(blocks[i] != NULL);
    }
    CHECK(rtos_mempool_available(&pool) == 0);
    CHECK(rtos_mempool_is_empty(&pool));

    /* Pool exhausted: nowait alloc returns NULL and does NOT block. */
    void *extra = rtos_mempool_alloc(&pool, RTOS_NO_WAIT);
    CHECK(extra == NULL);
    CHECK(host_stub_yield_count == 0);
    CHECK(host_stub_remove_ready_count == 0);
}

static void test_alloc_free_roundtrip(void)
{
    rtos_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    setup(&pool, buffer);

    void *a = rtos_mempool_alloc(&pool, RTOS_NO_WAIT);
    void *b = rtos_mempool_alloc(&pool, RTOS_NO_WAIT);
    CHECK(a != NULL && b != NULL && a != b);
    CHECK(rtos_mempool_available(&pool) == POOL_BLOCK_COUNT - 2);

    CHECK(rtos_mempool_free(&pool, a) == RTOS_OK);
    CHECK(rtos_mempool_free(&pool, b) == RTOS_OK);
    CHECK(rtos_mempool_available(&pool) == POOL_BLOCK_COUNT);
}

static void test_free_rejects_out_of_range(void)
{
    rtos_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    uint8_t bystander;
    setup(&pool, buffer);

    CHECK(rtos_mempool_free(&pool, &bystander) == RTOS_ERR_PARAM);
    CHECK(rtos_mempool_free(&pool, NULL) == RTOS_ERR_PARAM);
    CHECK(rtos_mempool_available(&pool) == POOL_BLOCK_COUNT);
}

static void test_isr_alloc_and_free(void)
{
    rtos_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    bool yield_needed = true;
    setup(&pool, buffer);

    void *p = rtos_mempool_alloc_from_isr(&pool);
    CHECK(p != NULL);
    CHECK(rtos_mempool_available(&pool) == POOL_BLOCK_COUNT - 1);

    rtos_status_t s = rtos_mempool_free_from_isr(&pool, p, &yield_needed);
    CHECK(s == RTOS_OK);
    CHECK(rtos_mempool_available(&pool) == POOL_BLOCK_COUNT);
    /* No waiters were registered, so no yield should be required. */
    CHECK(yield_needed == false);
}

/*===========================================================================*/
/* Driver                                                                     */
/*===========================================================================*/

int main(void)
{
    test_init();
    test_alloc_until_empty();
    test_alloc_free_roundtrip();
    test_free_rejects_out_of_range();
    test_isr_alloc_and_free();

    TEST_REPORT("mempool");
}
