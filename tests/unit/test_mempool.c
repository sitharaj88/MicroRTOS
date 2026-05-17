/**
 * MicroRTOS Host Unit Tests - Memory Pool Module
 *
 * Exercises the non-blocking memory-pool paths. The blocking-alloc path
 * is not covered here because it depends on a running scheduler.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_types.h"
#include "mr_memory.h"
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

static void setup(mr_mempool_t *pool, uint8_t *buffer)
{
    host_stubs_reset();
    memset(buffer, 0xAA, POOL_BLOCK_SIZE * POOL_BLOCK_COUNT);
    mr_mempool_init(pool, buffer, POOL_BLOCK_SIZE, POOL_BLOCK_COUNT);
}

/*===========================================================================*/
/* Tests                                                                      */
/*===========================================================================*/

static void test_init(void)
{
    mr_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    setup(&pool, buffer);

    CHECK(mr_mempool_total(&pool) == POOL_BLOCK_COUNT);
    CHECK(mr_mempool_available(&pool) == POOL_BLOCK_COUNT);
    CHECK(mr_mempool_block_size(&pool) >= POOL_BLOCK_SIZE);
    CHECK(!mr_mempool_is_empty(&pool));
}

static void test_alloc_until_empty(void)
{
    mr_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    void *blocks[POOL_BLOCK_COUNT];
    setup(&pool, buffer);

    for (int i = 0; i < POOL_BLOCK_COUNT; i++) {
        blocks[i] = mr_mempool_alloc(&pool, MR_NO_WAIT);
        CHECK(blocks[i] != NULL);
    }
    CHECK(mr_mempool_available(&pool) == 0);
    CHECK(mr_mempool_is_empty(&pool));

    /* Pool exhausted: nowait alloc returns NULL and does NOT block. */
    void *extra = mr_mempool_alloc(&pool, MR_NO_WAIT);
    CHECK(extra == NULL);
    CHECK(host_stub_yield_count == 0);
    CHECK(host_stub_remove_ready_count == 0);
}

static void test_alloc_free_roundtrip(void)
{
    mr_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    setup(&pool, buffer);

    void *a = mr_mempool_alloc(&pool, MR_NO_WAIT);
    void *b = mr_mempool_alloc(&pool, MR_NO_WAIT);
    CHECK(a != NULL && b != NULL && a != b);
    CHECK(mr_mempool_available(&pool) == POOL_BLOCK_COUNT - 2);

    CHECK(mr_mempool_free(&pool, a) == MR_OK);
    CHECK(mr_mempool_free(&pool, b) == MR_OK);
    CHECK(mr_mempool_available(&pool) == POOL_BLOCK_COUNT);
}

static void test_free_rejects_out_of_range(void)
{
    mr_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    uint8_t bystander;
    setup(&pool, buffer);

    CHECK(mr_mempool_free(&pool, &bystander) == MR_ERR_PARAM);
    CHECK(mr_mempool_free(&pool, NULL) == MR_ERR_PARAM);
    CHECK(mr_mempool_available(&pool) == POOL_BLOCK_COUNT);
}

static void test_isr_alloc_and_free(void)
{
    mr_mempool_t pool;
    uint8_t buffer[POOL_BLOCK_SIZE * POOL_BLOCK_COUNT];
    bool yield_needed = true;
    setup(&pool, buffer);

    void *p = mr_mempool_alloc_from_isr(&pool);
    CHECK(p != NULL);
    CHECK(mr_mempool_available(&pool) == POOL_BLOCK_COUNT - 1);

    mr_status_t s = mr_mempool_free_from_isr(&pool, p, &yield_needed);
    CHECK(s == MR_OK);
    CHECK(mr_mempool_available(&pool) == POOL_BLOCK_COUNT);
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
