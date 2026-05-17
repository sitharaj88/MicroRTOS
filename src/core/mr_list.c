/**
 * MicroRTOS - Linked List Implementation
 *
 * Intrusive doubly-linked list implementation optimized for
 * embedded systems with minimal memory overhead.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_list.h"

/*===========================================================================*/
/* List Initialization                                                        */
/*===========================================================================*/

void mr_list_init(mr_list_t *list)
{
    MR_ASSERT(list != NULL);

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

void mr_list_node_init(mr_list_node_t *node, void *container)
{
    MR_ASSERT(node != NULL);

    node->next = NULL;
    node->prev = NULL;
    node->container = container;
    node->value = 0;
}

/*===========================================================================*/
/* List Operations                                                            */
/*===========================================================================*/

void mr_list_insert_end(mr_list_t *list, mr_list_node_t *node)
{
    MR_ASSERT(list != NULL);
    MR_ASSERT(node != NULL);
    MR_ASSERT(!mr_list_node_is_linked(node));

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

void mr_list_insert_sorted(mr_list_t *list, mr_list_node_t *node)
{
    mr_list_node_t *current;

    MR_ASSERT(list != NULL);
    MR_ASSERT(node != NULL);
    MR_ASSERT(!mr_list_node_is_linked(node));

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

void mr_list_insert_priority(mr_list_t *list, mr_list_node_t *node)
{
    mr_list_node_t *current;

    MR_ASSERT(list != NULL);
    MR_ASSERT(node != NULL);
    MR_ASSERT(!mr_list_node_is_linked(node));

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

void mr_list_remove(mr_list_t *list, mr_list_node_t *node)
{
    MR_ASSERT(list != NULL);
    MR_ASSERT(node != NULL);

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

mr_list_node_t *mr_list_remove_head(mr_list_t *list)
{
    mr_list_node_t *node;

    MR_ASSERT(list != NULL);

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
