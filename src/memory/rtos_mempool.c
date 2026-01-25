/**
 * MicroRTOS - Memory Pool Implementation
 *
 * Fixed-size block memory pools with O(1) allocation.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_memory.h"
#include "rtos_list.h"
#include "rtos_port.h"
#include "rtos_task.h"

#if RTOS_USE_MEMORY_POOLS

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

extern rtos_tcb_t *g_current_tcb;
extern rtos_list_t g_delayed_list;
extern volatile uint32_t g_tick_count;
extern volatile rtos_kernel_state_t g_kernel_state;

extern void rtos_scheduler_add_ready(rtos_tcb_t *tcb);
extern void rtos_scheduler_remove_ready(rtos_tcb_t *tcb);

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

void rtos_mempool_init(
    rtos_mempool_t *pool,
    void *buffer,
    uint16_t block_size,
    uint16_t block_count)
{
    uint8_t *block_ptr;
    free_block_t *prev_block;
    uint16_t i;
    uint16_t actual_block_size;

    RTOS_ASSERT(pool != NULL);
    RTOS_ASSERT(buffer != NULL);
    RTOS_ASSERT(block_size >= sizeof(void *));
    RTOS_ASSERT(block_count > 0);

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
    rtos_list_init(&pool->wait_list);

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

void *rtos_mempool_alloc(rtos_mempool_t *pool, uint32_t timeout)
{
    rtos_tcb_t *current;
    free_block_t *block;

    RTOS_ASSERT(pool != NULL);
    RTOS_ASSERT(!rtos_port_is_in_isr());

    rtos_port_enter_critical();

    current = g_current_tcb;

    /* Check if blocks are available */
    if (pool->free_list != NULL) {
        block = (free_block_t *)pool->free_list;
        pool->free_list = block->next;
        pool->free_count--;
        rtos_port_exit_critical();
        return (void *)block;
    }

    /* No blocks available - need to wait */
    if (timeout == RTOS_NO_WAIT) {
        rtos_port_exit_critical();
        return NULL;
    }

    /* Block waiting for memory */
    rtos_scheduler_remove_ready(current);
    current->state = RTOS_TASK_BLOCKED;
    current->block_reason = RTOS_BLOCK_MEMPOOL;
    current->blocked_on = pool;

    /* Add to wait list */
    current->event_node.value = current->priority;
    rtos_list_insert_priority(&pool->wait_list, &current->event_node);

    /* Set timeout */
    if (timeout != RTOS_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        rtos_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    rtos_port_exit_critical();

    /* Switch to another task */
    rtos_port_yield();

    /* We're back - try to allocate or report timeout */
    rtos_port_enter_critical();

    /* Check if we're still waiting (timeout) */
    if (rtos_list_node_is_linked(&current->event_node)) {
        rtos_list_remove(&pool->wait_list, &current->event_node);
        current->blocked_on = NULL;
        current->block_reason = RTOS_BLOCK_NONE;
        rtos_port_exit_critical();
        return NULL;
    }

    /* We were unblocked - a block should be available */
    if (pool->free_list != NULL) {
        block = (free_block_t *)pool->free_list;
        pool->free_list = block->next;
        pool->free_count--;
        current->blocked_on = NULL;
        current->block_reason = RTOS_BLOCK_NONE;
        rtos_port_exit_critical();
        return (void *)block;
    }

    /* Shouldn't happen, but handle gracefully */
    current->blocked_on = NULL;
    current->block_reason = RTOS_BLOCK_NONE;
    rtos_port_exit_critical();
    return NULL;
}

rtos_status_t rtos_mempool_free(rtos_mempool_t *pool, void *block)
{
    rtos_tcb_t *waiter;
    rtos_list_node_t *node;
    free_block_t *free_block;
    bool need_switch = false;

    RTOS_ASSERT(pool != NULL);

    if (block == NULL) {
        return RTOS_ERR_PARAM;
    }

    /* Validate block is within pool range */
    if ((uint8_t *)block < (uint8_t *)pool->buffer ||
        (uint8_t *)block >= (uint8_t *)pool->buffer +
                            (pool->block_size * pool->block_count)) {
        return RTOS_ERR_PARAM;
    }

    rtos_port_enter_critical();

    /* Add block back to free list */
    free_block = (free_block_t *)block;
    free_block->next = (free_block_t *)pool->free_list;
    pool->free_list = free_block;
    pool->free_count++;

    /* Wake any waiting tasks */
    node = rtos_list_remove_head(&pool->wait_list);
    if (node != NULL) {
        waiter = RTOS_TCB_FROM_EVENT_NODE(node);

        /* Remove from delayed list if there */
        if (rtos_list_node_is_linked(&waiter->state_node)) {
            rtos_list_remove(&g_delayed_list, &waiter->state_node);
        }

        /* Unblock */
        waiter->state = RTOS_TASK_READY;
        waiter->block_reason = RTOS_BLOCK_NONE;
        waiter->blocked_on = NULL;
        rtos_scheduler_add_ready(waiter);

        if (g_current_tcb != NULL && waiter->priority < g_current_tcb->priority) {
            need_switch = true;
        }
    }

    rtos_port_exit_critical();

    if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
        rtos_port_yield();
    }

    return RTOS_OK;
}

uint16_t rtos_mempool_available(rtos_mempool_t *pool)
{
    return (pool != NULL) ? pool->free_count : 0;
}

uint16_t rtos_mempool_total(rtos_mempool_t *pool)
{
    return (pool != NULL) ? pool->block_count : 0;
}

uint16_t rtos_mempool_block_size(rtos_mempool_t *pool)
{
    return (pool != NULL) ? pool->block_size : 0;
}

bool rtos_mempool_is_empty(rtos_mempool_t *pool)
{
    return (pool != NULL) ? (pool->free_count == 0) : true;
}

void *rtos_mempool_alloc_from_isr(rtos_mempool_t *pool)
{
    free_block_t *block;

    RTOS_ASSERT(pool != NULL);

    if (pool->free_list == NULL) {
        return NULL;
    }

    block = (free_block_t *)pool->free_list;
    pool->free_list = block->next;
    pool->free_count--;

    return (void *)block;
}

rtos_status_t rtos_mempool_free_from_isr(
    rtos_mempool_t *pool,
    void *block,
    bool *yield_required)
{
    rtos_tcb_t *waiter;
    rtos_list_node_t *node;
    free_block_t *free_block;

    RTOS_ASSERT(pool != NULL);

    if (yield_required != NULL) {
        *yield_required = false;
    }

    if (block == NULL) {
        return RTOS_ERR_PARAM;
    }

    /* Add block back to free list */
    free_block = (free_block_t *)block;
    free_block->next = (free_block_t *)pool->free_list;
    pool->free_list = free_block;
    pool->free_count++;

    /* Wake any waiting tasks */
    node = rtos_list_remove_head(&pool->wait_list);
    if (node != NULL) {
        waiter = RTOS_TCB_FROM_EVENT_NODE(node);

        if (rtos_list_node_is_linked(&waiter->state_node)) {
            rtos_list_remove(&g_delayed_list, &waiter->state_node);
        }

        waiter->state = RTOS_TASK_READY;
        waiter->block_reason = RTOS_BLOCK_NONE;
        waiter->blocked_on = NULL;
        rtos_scheduler_add_ready(waiter);

        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            waiter->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    }

    return RTOS_OK;
}

#endif /* RTOS_USE_MEMORY_POOLS */
