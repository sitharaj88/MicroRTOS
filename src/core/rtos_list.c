/**
 * MicroRTOS - Linked List Implementation
 *
 * Intrusive doubly-linked list implementation optimized for
 * embedded systems with minimal memory overhead.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_list.h"

/*===========================================================================*/
/* List Initialization                                                        */
/*===========================================================================*/

void rtos_list_init(rtos_list_t *list)
{
    RTOS_ASSERT(list != NULL);

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

void rtos_list_node_init(rtos_list_node_t *node, void *container)
{
    RTOS_ASSERT(node != NULL);

    node->next = NULL;
    node->prev = NULL;
    node->container = container;
    node->value = 0;
}

/*===========================================================================*/
/* List Operations                                                            */
/*===========================================================================*/

void rtos_list_insert_end(rtos_list_t *list, rtos_list_node_t *node)
{
    RTOS_ASSERT(list != NULL);
    RTOS_ASSERT(node != NULL);
    RTOS_ASSERT(!rtos_list_node_is_linked(node));

    node->next = NULL;
    node->prev = list->tail;

    if (list->tail != NULL) {
        list->tail->next = node;
    } else {
        /* List was empty */
        list->head = node;
    }

    list->tail = node;
    list->count++;
}

void rtos_list_insert_sorted(rtos_list_t *list, rtos_list_node_t *node)
{
    rtos_list_node_t *current;

    RTOS_ASSERT(list != NULL);
    RTOS_ASSERT(node != NULL);
    RTOS_ASSERT(!rtos_list_node_is_linked(node));

    /* Empty list or insert at head */
    if (list->head == NULL || node->value < list->head->value) {
        node->next = list->head;
        node->prev = NULL;

        if (list->head != NULL) {
            list->head->prev = node;
        } else {
            list->tail = node;
        }

        list->head = node;
        list->count++;
        return;
    }

    /* Find insertion point */
    current = list->head;
    while (current->next != NULL && current->next->value <= node->value) {
        current = current->next;
    }

    /* Insert after current */
    node->next = current->next;
    node->prev = current;

    if (current->next != NULL) {
        current->next->prev = node;
    } else {
        list->tail = node;
    }

    current->next = node;
    list->count++;
}

void rtos_list_insert_priority(rtos_list_t *list, rtos_list_node_t *node)
{
    rtos_list_node_t *current;

    RTOS_ASSERT(list != NULL);
    RTOS_ASSERT(node != NULL);
    RTOS_ASSERT(!rtos_list_node_is_linked(node));

    /* Empty list or highest priority (lowest value) */
    if (list->head == NULL || node->value < list->head->value) {
        node->next = list->head;
        node->prev = NULL;

        if (list->head != NULL) {
            list->head->prev = node;
        } else {
            list->tail = node;
        }

        list->head = node;
        list->count++;
        return;
    }

    /* Find insertion point - insert after equal priorities (FIFO within priority) */
    current = list->head;
    while (current->next != NULL && current->next->value <= node->value) {
        current = current->next;
    }

    /* Insert after current */
    node->next = current->next;
    node->prev = current;

    if (current->next != NULL) {
        current->next->prev = node;
    } else {
        list->tail = node;
    }

    current->next = node;
    list->count++;
}

void rtos_list_remove(rtos_list_t *list, rtos_list_node_t *node)
{
    RTOS_ASSERT(list != NULL);
    RTOS_ASSERT(node != NULL);

    /* Node not in any list */
    if (node->next == NULL && node->prev == NULL &&
        list->head != node) {
        return;
    }

    /* Update previous node or head */
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }

    /* Update next node or tail */
    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }

    /* Clear node links */
    node->next = NULL;
    node->prev = NULL;

    list->count--;
}

rtos_list_node_t *rtos_list_remove_head(rtos_list_t *list)
{
    rtos_list_node_t *node;

    RTOS_ASSERT(list != NULL);

    node = list->head;
    if (node == NULL) {
        return NULL;
    }

    list->head = node->next;

    if (list->head != NULL) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }

    node->next = NULL;
    node->prev = NULL;

    list->count--;

    return node;
}
