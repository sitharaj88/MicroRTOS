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

#include "mr_list.h"
#include "test_harness.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

TEST_DEFINE_GLOBALS();

/*===========================================================================*/
/* Test cases                                                                 */
/*===========================================================================*/

static void test_init_is_empty(void)
{
    mr_list_t list;
    mr_list_init(&list);
    CHECK(mr_list_is_empty(&list));
    CHECK(mr_list_count(&list) == 0);
    CHECK(mr_list_peek_head(&list) == NULL);
    CHECK(mr_list_peek_tail(&list) == NULL);
}

static void test_insert_end_and_remove_head(void)
{
    mr_list_t list;
    mr_list_node_t a, b, c;

    mr_list_init(&list);
    mr_list_node_init(&a, NULL);
    mr_list_node_init(&b, NULL);
    mr_list_node_init(&c, NULL);

    mr_list_insert_end(&list, &a);
    mr_list_insert_end(&list, &b);
    mr_list_insert_end(&list, &c);

    CHECK(mr_list_count(&list) == 3);
    CHECK(mr_list_peek_head(&list) == &a);
    CHECK(mr_list_peek_tail(&list) == &c);

    CHECK(mr_list_remove_head(&list) == &a);
    CHECK(mr_list_remove_head(&list) == &b);
    CHECK(mr_list_remove_head(&list) == &c);
    CHECK(mr_list_remove_head(&list) == NULL);
    CHECK(mr_list_is_empty(&list));
}

static void test_insert_sorted_ascending(void)
{
    mr_list_t list;
    mr_list_node_t n1, n2, n3, n4;

    mr_list_init(&list);
    mr_list_node_init(&n1, NULL); n1.value = 30;
    mr_list_node_init(&n2, NULL); n2.value = 10;
    mr_list_node_init(&n3, NULL); n3.value = 20;
    mr_list_node_init(&n4, NULL); n4.value = 20;     /* dup */

    mr_list_insert_sorted(&list, &n1);
    mr_list_insert_sorted(&list, &n2);
    mr_list_insert_sorted(&list, &n3);
    mr_list_insert_sorted(&list, &n4);

    /* Expected order: 10, 20, 20-dup, 30 */
    mr_list_node_t *p = mr_list_remove_head(&list);
    CHECK(p == &n2);
    p = mr_list_remove_head(&list);
    CHECK(p->value == 20);
    p = mr_list_remove_head(&list);
    CHECK(p->value == 20);
    p = mr_list_remove_head(&list);
    CHECK(p == &n1);
    CHECK(mr_list_is_empty(&list));
}

static void test_remove_arbitrary(void)
{
    mr_list_t list;
    mr_list_node_t a, b, c;

    mr_list_init(&list);
    mr_list_node_init(&a, NULL);
    mr_list_node_init(&b, NULL);
    mr_list_node_init(&c, NULL);

    mr_list_insert_end(&list, &a);
    mr_list_insert_end(&list, &b);
    mr_list_insert_end(&list, &c);

    /* Remove from middle */
    mr_list_remove(&list, &b);
    CHECK(mr_list_count(&list) == 2);
    CHECK(!mr_list_node_is_linked(&b));
    CHECK(mr_list_peek_head(&list) == &a);
    CHECK(mr_list_peek_tail(&list) == &c);

    /* Remove tail */
    mr_list_remove(&list, &c);
    CHECK(mr_list_count(&list) == 1);
    CHECK(mr_list_peek_tail(&list) == &a);

    /* Remove head */
    mr_list_remove(&list, &a);
    CHECK(mr_list_is_empty(&list));
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

    TEST_REPORT("list");
}
