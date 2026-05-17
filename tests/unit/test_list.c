/**
 * MicroRTOS Host Unit Tests - Linked List Module
 *
 * These tests build with the host compiler (no AVR/ARM cross-toolchain)
 * to exercise the intrusive doubly-linked list used by the scheduler
 * and wait-queues. Run via `make test`.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

/* Compile RTOS_ASSERT out for host tests; we use plain assert() instead. */
#define RTOS_USE_ASSERT 0
/* Pretend to be the AVR platform so rtos_config.h does not error out;
 * the list module does not actually touch any platform code.  */
#define RTOS_PLATFORM_AVR 1

#include "rtos_list.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/*===========================================================================*/
/* Tiny test harness                                                          */
/*===========================================================================*/

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond) do {                                                       \
    tests_run++;                                                               \
    if (!(cond)) {                                                             \
        tests_failed++;                                                        \
        fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
    }                                                                          \
} while (0)

/*===========================================================================*/
/* Test cases                                                                 */
/*===========================================================================*/

static void test_init_is_empty(void)
{
    rtos_list_t list;
    rtos_list_init(&list);
    CHECK(rtos_list_is_empty(&list));
    CHECK(rtos_list_count(&list) == 0);
    CHECK(rtos_list_peek_head(&list) == NULL);
    CHECK(rtos_list_peek_tail(&list) == NULL);
}

static void test_insert_end_and_remove_head(void)
{
    rtos_list_t list;
    rtos_list_node_t a, b, c;

    rtos_list_init(&list);
    rtos_list_node_init(&a, NULL);
    rtos_list_node_init(&b, NULL);
    rtos_list_node_init(&c, NULL);

    rtos_list_insert_end(&list, &a);
    rtos_list_insert_end(&list, &b);
    rtos_list_insert_end(&list, &c);

    CHECK(rtos_list_count(&list) == 3);
    CHECK(rtos_list_peek_head(&list) == &a);
    CHECK(rtos_list_peek_tail(&list) == &c);

    CHECK(rtos_list_remove_head(&list) == &a);
    CHECK(rtos_list_remove_head(&list) == &b);
    CHECK(rtos_list_remove_head(&list) == &c);
    CHECK(rtos_list_remove_head(&list) == NULL);
    CHECK(rtos_list_is_empty(&list));
}

static void test_insert_sorted_ascending(void)
{
    rtos_list_t list;
    rtos_list_node_t n1, n2, n3, n4;

    rtos_list_init(&list);
    rtos_list_node_init(&n1, NULL); n1.value = 30;
    rtos_list_node_init(&n2, NULL); n2.value = 10;
    rtos_list_node_init(&n3, NULL); n3.value = 20;
    rtos_list_node_init(&n4, NULL); n4.value = 20;     /* dup */

    rtos_list_insert_sorted(&list, &n1);
    rtos_list_insert_sorted(&list, &n2);
    rtos_list_insert_sorted(&list, &n3);
    rtos_list_insert_sorted(&list, &n4);

    /* Expected order: 10, 20, 20-dup, 30 */
    rtos_list_node_t *p = rtos_list_remove_head(&list);
    CHECK(p == &n2);
    p = rtos_list_remove_head(&list);
    CHECK(p->value == 20);
    p = rtos_list_remove_head(&list);
    CHECK(p->value == 20);
    p = rtos_list_remove_head(&list);
    CHECK(p == &n1);
    CHECK(rtos_list_is_empty(&list));
}

static void test_remove_arbitrary(void)
{
    rtos_list_t list;
    rtos_list_node_t a, b, c;

    rtos_list_init(&list);
    rtos_list_node_init(&a, NULL);
    rtos_list_node_init(&b, NULL);
    rtos_list_node_init(&c, NULL);

    rtos_list_insert_end(&list, &a);
    rtos_list_insert_end(&list, &b);
    rtos_list_insert_end(&list, &c);

    /* Remove from middle */
    rtos_list_remove(&list, &b);
    CHECK(rtos_list_count(&list) == 2);
    CHECK(!rtos_list_node_is_linked(&b));
    CHECK(rtos_list_peek_head(&list) == &a);
    CHECK(rtos_list_peek_tail(&list) == &c);

    /* Remove tail */
    rtos_list_remove(&list, &c);
    CHECK(rtos_list_count(&list) == 1);
    CHECK(rtos_list_peek_tail(&list) == &a);

    /* Remove head */
    rtos_list_remove(&list, &a);
    CHECK(rtos_list_is_empty(&list));
}

/*===========================================================================*/
/* Driver                                                                     */
/*===========================================================================*/

int main(void)
{
    test_init_is_empty();
    test_insert_end_and_remove_head();
    test_insert_sorted_ascending();
    test_remove_arbitrary();

    printf("Ran %d checks; %d failed.\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
