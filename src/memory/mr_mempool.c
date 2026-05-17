/**
 * MicroRTOS - Memory Pool Implementation
 *
 * Fixed-size block memory pools with O(1) allocation.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_memory.h"
#include "mr_list.h"
#include "mr_port.h"
#include "mr_task.h"

#if MR_USE_MEMORY_POOLS

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

extern mr_tcb_t *g_current_tcb;
extern mr_list_t g_delayed_list;
extern volatile uint32_t g_tick_count;
extern volatile mr_kernel_state_t g_kernel_state;

extern void mr_scheduler_add_ready(mr_tcb_t *tcb);
extern void mr_scheduler_remove_ready(mr_tcb_t *tcb);

/*===========================================================================*/
/* Free Block Structure                                                       */
/*===========================================================================*/

/*
 * Each free block contains a pointer to the next free block.
 * This means block_size must be at least sizeof(void*).
 */
typedef struct free_block {
    struct free_block *next;
} free_block_t;

/*===========================================================================*/
/* Memory Pool API Implementation                                             */
/*===========================================================================*/

void mr_mempool_init(
    mr_mempool_t *pool,
    void *buffer,
    uint16_t block_size,
    uint16_t block_count)
{
    uint8_t *block_ptr;
    free_block_t *prev_block;
    uint16_t i;
    uint16_t actual_block_size;

    MR_ASSERT(pool != NULL);
    MR_ASSERT(buffer != NULL);
    MR_ASSERT(block_size >= sizeof(void *));
    MR_ASSERT(block_count > 0);

    /* Ensure block size is at least pointer-sized and aligned */
    actual_block_size = block_size;
    if (actual_block_size < sizeof(void *)) {
        actual_block_size = sizeof(void *);
    }

    /* Align to pointer size */
    actual_block_size = (actual_block_size + sizeof(void *) - 1) &
                        ~(sizeof(void *) - 1);

    pool->buffer = buffer;
    pool->block_size = actual_block_size;
    pool->block_count = block_count;
    pool->free_count = block_count;
    mr_list_init(&pool->wait_list);

    /* Build free list by linking all blocks */
    block_ptr = (uint8_t *)buffer;
    pool->free_list = (void *)block_ptr;
    prev_block = (free_block_t *)block_ptr;

    for (i = 1; i < block_count; i++) {
        block_ptr += actual_block_size;
        prev_block->next = (free_block_t *)block_ptr;
        prev_block = (free_block_t *)block_ptr;
    }

    /* Last block points to NULL */
    prev_block->next = NULL;
}

void *mr_mempool_alloc(mr_mempool_t *pool, uint32_t timeout)
{
    mr_tcb_t *current;
    free_block_t *block;

    MR_ASSERT(pool != NULL);
    MR_ASSERT(!mr_port_is_in_isr());

    mr_port_enter_critical();

    current = g_current_tcb;

    /* Check if blocks are available */
    if (pool->free_list != NULL) {
        block = (free_block_t *)pool->free_list;
        pool->free_list = block->next;
        pool->free_count--;
        mr_port_exit_critical();
        return (void *)block;
    }

    /* No blocks available - need to wait */
    if (timeout == MR_NO_WAIT) {
        mr_port_exit_critical();
        return NULL;
    }

    /* Block waiting for memory */
    mr_scheduler_remove_ready(current);
    current->state = MR_TASK_BLOCKED;
    current->block_reason = MR_BLOCK_MEMPOOL;
    current->blocked_on = pool;

    /* Add to wait list */
    current->event_node.value = current->priority;
    mr_list_insert_priority(&pool->wait_list, &current->event_node);

    /* Set timeout */
    if (timeout != MR_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        mr_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    mr_port_exit_critical();

    /* Switch to another task */
    mr_port_yield();

    /* We're back - try to allocate or report timeout */
    mr_port_enter_critical();

    /* Check if we're still waiting (timeout) */
    if (mr_list_node_is_linked(&current->event_node)) {
        mr_list_remove(&pool->wait_list, &current->event_node);
        current->blocked_on = NULL;
        current->block_reason = MR_BLOCK_NONE;
        mr_port_exit_critical();
        return NULL;
    }

    /* We were unblocked - a block should be available */
    if (pool->free_list != NULL) {
        block = (free_block_t *)pool->free_list;
        pool->free_list = block->next;
        pool->free_count--;
        current->blocked_on = NULL;
        current->block_reason = MR_BLOCK_NONE;
        mr_port_exit_critical();
        return (void *)block;
    }

    /* Shouldn't happen, but handle gracefully */
    current->blocked_on = NULL;
    current->block_reason = MR_BLOCK_NONE;
    mr_port_exit_critical();
    return NULL;
}

mr_status_t mr_mempool_free(mr_mempool_t *pool, void *block)
{
    mr_tcb_t *waiter;
    mr_list_node_t *node;
    free_block_t *free_block;
    bool need_switch = false;

    MR_ASSERT(pool != NULL);

    if (block == NULL) {
        return MR_ERR_PARAM;
    }

    /* Validate block is within pool range */
    if ((uint8_t *)block < (uint8_t *)pool->buffer ||
        (uint8_t *)block >= (uint8_t *)pool->buffer +
                            (pool->block_size * pool->block_count)) {
        return MR_ERR_PARAM;
    }

    mr_port_enter_critical();

    /* Add block back to free list */
    free_block = (free_block_t *)block;
    free_block->next = (free_block_t *)pool->free_list;
    pool->free_list = free_block;
    pool->free_count++;

    /* Wake any waiting tasks */
    node = mr_list_remove_head(&pool->wait_list);
    if (node != NULL) {
        waiter = MR_TCB_FROM_EVENT_NODE(node);

        /* Remove from delayed list if there */
        if (mr_list_node_is_linked(&waiter->state_node)) {
            mr_list_remove(&g_delayed_list, &waiter->state_node);
        }

        /* Unblock */
        waiter->state = MR_TASK_READY;
        waiter->block_reason = MR_BLOCK_NONE;
        waiter->blocked_on = NULL;
        mr_scheduler_add_ready(waiter);

        if (g_current_tcb != NULL && waiter->priority < g_current_tcb->priority) {
            need_switch = true;
        }
    }

    mr_port_exit_critical();

    if (need_switch && g_kernel_state == MR_KERNEL_RUNNING) {
        mr_port_yield();
    }

    return MR_OK;
}

uint16_t mr_mempool_available(mr_mempool_t *pool)
{
    return (pool != NULL) ? pool->free_count : 0;
}

uint16_t mr_mempool_total(mr_mempool_t *pool)
{
    return (pool != NULL) ? pool->block_count : 0;
}

uint16_t mr_mempool_block_size(mr_mempool_t *pool)
{
    return (pool != NULL) ? pool->block_size : 0;
}

bool mr_mempool_is_empty(mr_mempool_t *pool)
{
    return (pool != NULL) ? (pool->free_count == 0) : true;
}

void *mr_mempool_alloc_from_isr(mr_mempool_t *pool)
{
    free_block_t *block;

    MR_ASSERT(pool != NULL);

    if (pool->free_list == NULL) {
        return NULL;
    }

    block = (free_block_t *)pool->free_list;
    pool->free_list = block->next;
    pool->free_count--;

    return (void *)block;
}

mr_status_t mr_mempool_free_from_isr(
    mr_mempool_t *pool,
    void *block,
    bool *yield_required)
{
    mr_tcb_t *waiter;
    mr_list_node_t *node;
    free_block_t *free_block;

    MR_ASSERT(pool != NULL);

    if (yield_required != NULL) {
        *yield_required = false;
    }

    if (block == NULL) {
        return MR_ERR_PARAM;
    }

    /* Add block back to free list */
    free_block = (free_block_t *)block;
    free_block->next = (free_block_t *)pool->free_list;
    pool->free_list = free_block;
    pool->free_count++;

    /* Wake any waiting tasks */
    node = mr_list_remove_head(&pool->wait_list);
    if (node != NULL) {
        waiter = MR_TCB_FROM_EVENT_NODE(node);

        if (mr_list_node_is_linked(&waiter->state_node)) {
            mr_list_remove(&g_delayed_list, &waiter->state_node);
        }

        waiter->state = MR_TASK_READY;
        waiter->block_reason = MR_BLOCK_NONE;
        waiter->blocked_on = NULL;
        mr_scheduler_add_ready(waiter);

        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            waiter->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    }

    return MR_OK;
}

#endif /* MR_USE_MEMORY_POOLS */
