/**
 * MicroRTOS - Linked List Utilities
 *
 * Intrusive doubly-linked list implementation for task queues,
 * wait lists, and timer management. These lists are sorted by
 * a value field (priority, wake time, etc.).
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_LIST_H
#define MR_LIST_H

#include "mr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* List Initialization                                                        */
/*===========================================================================*/

/**
 * Initialize a list to empty state.
 *
 * @param list Pointer to list structure
 */
void mr_list_init(mr_list_t *list);

/**
 * Initialize a list node.
 *
 * @param node      Pointer to node structure
 * @param container Pointer to containing structure
 */
void mr_list_node_init(mr_list_node_t *node, void *container);

/*===========================================================================*/
/* List Operations                                                            */
/*===========================================================================*/

/**
 * Insert a node at the end of the list.
 *
 * @param list Pointer to list structure
 * @param node Node to insert
 */
void mr_list_insert_end(mr_list_t *list, mr_list_node_t *node);

/**
 * Insert a node sorted by value (ascending order).
 * Nodes with equal values are inserted after existing nodes.
 *
 * @param list Pointer to list structure
 * @param node Node to insert
 */
void mr_list_insert_sorted(mr_list_t *list, mr_list_node_t *node);

/**
 * Insert a node sorted by value (descending order - for priority).
 * Lower value = higher priority, inserted earlier.
 *
 * @param list Pointer to list structure
 * @param node Node to insert
 */
void mr_list_insert_priority(mr_list_t *list, mr_list_node_t *node);

/**
 * Remove a node from its current list.
 *
 * @param list Pointer to list structure
 * @param node Node to remove
 */
void mr_list_remove(mr_list_t *list, mr_list_node_t *node);

/**
 * Remove and return the first node from the list.
 *
 * @param list Pointer to list structure
 * @return Removed node, or NULL if list is empty
 */
mr_list_node_t *mr_list_remove_head(mr_list_t *list);

/*===========================================================================*/
/* List Queries                                                               */
/*===========================================================================*/

/**
 * Check if a list is empty.
 *
 * @param list Pointer to list structure
 * @return true if empty, false otherwise
 */
static inline bool mr_list_is_empty(const mr_list_t *list)
{
    return (list->count == 0);
}

/**
 * Get the number of items in a list.
 *
 * @param list Pointer to list structure
 * @return Number of items
 */
static inline uint16_t mr_list_count(const mr_list_t *list)
{
    return list->count;
}

/**
 * Get the first node in a list without removing it.
 *
 * @param list Pointer to list structure
 * @return First node, or NULL if list is empty
 */
static inline mr_list_node_t *mr_list_peek_head(const mr_list_t *list)
{
    return list->head;
}

/**
 * Get the last node in a list without removing it.
 *
 * @param list Pointer to list structure
 * @return Last node, or NULL if list is empty
 */
static inline mr_list_node_t *mr_list_peek_tail(const mr_list_t *list)
{
    return list->tail;
}

/**
 * Check if a node is currently in a list.
 *
 * @param node Pointer to node structure
 * @return true if node is in a list, false otherwise
 */
static inline bool mr_list_node_is_linked(const mr_list_node_t *node)
{
    return (node->next != NULL || node->prev != NULL);
}

/*===========================================================================*/
/* List Iteration                                                             */
/*===========================================================================*/

/**
 * Iterate over all nodes in a list.
 * Usage: mr_list_for_each(node, &list) { ... }
 */
#define mr_list_for_each(node, list) \
    for ((node) = (list)->head; (node) != NULL; (node) = (node)->next)

/**
 * Safe iteration that allows removal during iteration.
 * Usage: mr_list_for_each_safe(node, tmp, &list) { ... }
 */
#define mr_list_for_each_safe(node, tmp, list) \
    for ((node) = (list)->head, (tmp) = ((node) ? (node)->next : NULL); \
         (node) != NULL; \
         (node) = (tmp), (tmp) = ((node) ? (node)->next : NULL))

#ifdef __cplusplus
}
#endif

#endif /* MR_LIST_H */
