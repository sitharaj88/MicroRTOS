/**
 * MicroRTOS - Core Type Definitions
 *
 * This file contains all fundamental types used throughout MicroRTOS.
 * It includes the Task Control Block (TCB), list structures, and
 * various status/state enumerations.
 *
 * Compliance Standards:
 * - MISRA C:2012 (Mandatory, Required, Advisory rules)
 * - IEC 61508 SIL 1-4
 * - ISO 26262 ASIL A-D
 * - CERT C Coding Standard
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_TYPES_H
#define MR_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "mr_config.h"
#include "mr_misra.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Forward Declarations                                                       */
/*===========================================================================*/

typedef struct mr_tcb mr_tcb_t;
typedef struct mr_list mr_list_t;
typedef struct mr_list_node mr_list_node_t;

/*===========================================================================*/
/* Status and Return Codes                                                    */
/*===========================================================================*/

/**
 * RTOS function return status codes.
 */
typedef enum mr_status {
    MR_OK              = 0,     /**< Operation completed successfully */
    MR_ERR_TIMEOUT     = 1,     /**< Timeout waiting for resource */
    MR_ERR_FULL        = 2,     /**< Resource is full (queue, pool) */
    MR_ERR_EMPTY       = 3,     /**< Resource is empty (queue) */
    MR_ERR_PARAM       = 4,     /**< Invalid parameter */
    MR_ERR_STATE       = 5,     /**< Invalid state for operation */
    MR_ERR_NOMEM       = 6,     /**< Out of memory */
    MR_ERR_DELETED     = 7,     /**< Object was deleted */
    MR_ERR_ISR         = 8,     /**< Invalid call from ISR context */
    MR_ERR_RECURSIVE   = 9,     /**< Recursive lock not allowed */
    MR_ERR_NOT_OWNER   = 10,    /**< Not the mutex owner */
    MR_ERR_OVERFLOW    = 11,    /**< Stack overflow detected */
    MR_ERR_NO_MEMORY   = 12,    /**< No memory available */
    MR_ERR_NOT_SUPPORTED = 13,  /**< Feature not supported */
} mr_status_t;

/**
 * Task states.
 */
typedef enum mr_task_state {
    MR_TASK_READY      = 0,     /**< Task is ready to run */
    MR_TASK_RUNNING    = 1,     /**< Task is currently running */
    MR_TASK_BLOCKED    = 2,     /**< Task is blocked on resource */
    MR_TASK_SUSPENDED  = 3,     /**< Task is suspended */
    MR_TASK_DELETED    = 4,     /**< Task has been deleted */
} mr_task_state_t;

/**
 * Block reasons (why a task is blocked).
 */
typedef enum mr_block_reason {
    MR_BLOCK_NONE      = 0,     /**< Not blocked */
    MR_BLOCK_DELAY     = 1,     /**< Blocked on delay */
    MR_BLOCK_MUTEX     = 2,     /**< Blocked on mutex */
    MR_BLOCK_SEM       = 3,     /**< Blocked on semaphore */
    MR_BLOCK_QUEUE_SEND = 4,    /**< Blocked on queue send */
    MR_BLOCK_QUEUE_RECV = 5,    /**< Blocked on queue receive */
    MR_BLOCK_EVENT     = 6,     /**< Blocked on event group */
    MR_BLOCK_NOTIFY    = 7,     /**< Blocked on notification */
    MR_BLOCK_MEMPOOL   = 8,     /**< Blocked on memory pool */
} mr_block_reason_t;

/*===========================================================================*/
/* List Structures (Intrusive Doubly-Linked List)                             */
/*===========================================================================*/

/**
 * List node structure (embedded in other structures).
 */
struct mr_list_node {
    mr_list_node_t *next;       /**< Next node in list */
    mr_list_node_t *prev;       /**< Previous node in list */
    void *container;              /**< Pointer to containing structure */
    uint32_t value;               /**< Sort value (priority, tick, etc.) */
};

/**
 * List head structure.
 */
struct mr_list {
    mr_list_node_t *head;       /**< First node in list */
    mr_list_node_t *tail;       /**< Last node in list */
    uint16_t count;               /**< Number of items in list */
};

/*===========================================================================*/
/* Task Control Block                                                         */
/*===========================================================================*/

/**
 * Task Control Block - Core task data structure.
 *
 * Note: stack_ptr MUST be the first member for efficient context switch.
 */
struct mr_tcb {
    /* Critical context switch data - DO NOT REORDER */
    void *stack_ptr;              /**< Current stack pointer (must be first!) */

    /* Priority management */
    uint8_t priority;             /**< Current priority (may be boosted) */
    uint8_t base_priority;        /**< Original assigned priority */

    /* Task state */
    mr_task_state_t state;      /**< Current task state */
    mr_block_reason_t block_reason; /**< Why task is blocked */

    /* Timing */
    uint32_t delay_ticks;         /**< Remaining ticks for delay/timeout */
    uint32_t wake_tick;           /**< Absolute tick to wake up */

    /* List nodes for various queues */
    mr_list_node_t state_node;  /**< Node in ready/delayed list */
    mr_list_node_t event_node;  /**< Node in event/resource wait list */

    /* Stack management */
    void *stack_base;             /**< Stack base address (for overflow check) */
    uint16_t stack_size;          /**< Stack size in bytes */

    /* Blocking object */
    void *blocked_on;             /**< Object task is blocked on */

#if MR_USE_TASK_NOTIFICATIONS
    /* Task notifications */
    uint32_t notify_value;        /**< Notification value */
    uint8_t notify_state;         /**< Notification state */
#endif

#if MR_USE_MUTEXES
    /* Mutex handling for priority inheritance */
    struct mr_mutex *mutex_held; /**< List of held mutexes */
    uint8_t mutexes_held_count;    /**< Number of mutexes held */
#endif

#if MR_USE_TASK_NAMES
    const char *name;             /**< Task name for debugging */
#endif

#if MR_USE_RUNTIME_STATS
    uint32_t runtime_ticks;       /**< Total runtime in ticks */
    uint32_t switch_in_tick;      /**< Tick when task started running */
#endif

    /* Time slice tracking */
#if MR_USE_TIMESLICING
    uint8_t timeslice_remaining;  /**< Remaining time slice ticks */
#endif
};

/*===========================================================================*/
/* Synchronization Object Types                                               */
/*===========================================================================*/

#if MR_USE_MUTEXES
/**
 * Mutex structure with priority inheritance support.
 */
typedef struct mr_mutex {
    mr_tcb_t *owner;            /**< Current owner task */
    mr_list_t wait_list;        /**< Tasks waiting for mutex */
    uint8_t lock_count;           /**< Recursive lock count */
    struct mr_mutex *next;      /**< Next mutex in owner's held list */
} mr_mutex_t;
#endif

#if MR_USE_SEMAPHORES
/**
 * Semaphore structure (binary or counting).
 */
typedef struct mr_sem {
    volatile uint32_t count;      /**< Current count */
    uint32_t max_count;           /**< Maximum count (1 for binary) */
    mr_list_t wait_list;        /**< Tasks waiting for semaphore */
} mr_sem_t;
#endif

#if MR_USE_QUEUES
/**
 * Message queue structure.
 */
typedef struct mr_queue {
    uint8_t *buffer;              /**< Message buffer */
    uint16_t item_size;           /**< Size of each item */
    uint16_t capacity;            /**< Maximum number of items */
    volatile uint16_t count;      /**< Current number of items */
    uint16_t head;                /**< Read index */
    uint16_t tail;                /**< Write index */
    mr_list_t send_wait_list;   /**< Tasks waiting to send */
    mr_list_t recv_wait_list;   /**< Tasks waiting to receive */
} mr_queue_t;
#endif

#if MR_USE_EVENT_GROUPS
/**
 * Event group structure.
 */
typedef struct mr_event {
    volatile uint32_t bits;       /**< Current event bits */
    mr_list_t wait_list;        /**< Tasks waiting for events */
} mr_event_t;

/**
 * Event wait info (stored in task while waiting).
 */
typedef struct mr_event_wait_info {
    uint32_t wait_bits;           /**< Bits to wait for */
    bool wait_all;                /**< Wait for all bits */
    bool clear_on_exit;           /**< Clear bits on exit */
} mr_event_wait_info_t;
#endif

#if MR_USE_TIMERS
/**
 * Software timer states.
 */
typedef enum mr_timer_state {
    MR_TIMER_STOPPED   = 0,     /**< Timer is stopped */
    MR_TIMER_RUNNING   = 1,     /**< Timer is running */
    MR_TIMER_EXPIRED   = 2,     /**< Timer has expired */
} mr_timer_state_t;

/**
 * Software timer structure.
 */
typedef struct mr_timer {
    mr_list_node_t node;        /**< Node in timer list */
    uint32_t period_ticks;        /**< Timer period in ticks */
    uint32_t expire_tick;         /**< Next expiration tick */
    void (*callback)(struct mr_timer *timer); /**< Callback function */
    void *user_data;              /**< User data pointer */
    mr_timer_state_t state;     /**< Timer state */
    bool auto_reload;             /**< Auto-reload after expiration */
#if MR_USE_TASK_NAMES
    const char *name;             /**< Timer name for debugging */
#endif
} mr_timer_t;
#endif

#if MR_USE_MEMORY_POOLS
/**
 * Memory pool structure.
 */
typedef struct mr_mempool {
    void *buffer;                 /**< Pool memory buffer */
    void *free_list;              /**< Head of free block list */
    uint16_t block_size;          /**< Size of each block */
    uint16_t block_count;         /**< Total number of blocks */
    volatile uint16_t free_count; /**< Number of free blocks */
    mr_list_t wait_list;        /**< Tasks waiting for blocks */
} mr_mempool_t;
#endif

/*===========================================================================*/
/* Task Notification States                                                   */
/*===========================================================================*/

#if MR_USE_TASK_NOTIFICATIONS
typedef enum mr_notify_state {
    MR_NOTIFY_NOT_WAITING = 0,  /**< Not waiting for notification */
    MR_NOTIFY_WAITING     = 1,  /**< Waiting for notification */
    MR_NOTIFY_RECEIVED    = 2,  /**< Notification received */
} mr_notify_state_t;

typedef enum mr_notify_action {
    MR_NOTIFY_SET_VALUE   = 0,  /**< Set notification value */
    MR_NOTIFY_INCREMENT   = 1,  /**< Increment notification value */
    MR_NOTIFY_SET_BITS    = 2,  /**< Set bits in notification value */
    MR_NOTIFY_NO_ACTION   = 3,  /**< Just wake the task */
} mr_notify_action_t;
#endif

/*===========================================================================*/
/* Kernel State                                                               */
/*===========================================================================*/

/**
 * Kernel states.
 */
typedef enum mr_kernel_state {
    MR_KERNEL_NOT_STARTED = 0,  /**< Kernel not yet started */
    MR_KERNEL_RUNNING     = 1,  /**< Kernel is running */
    MR_KERNEL_SUSPENDED   = 2,  /**< Scheduler suspended */
} mr_kernel_state_t;

/*===========================================================================*/
/* Utility Macros                                                             */
/*===========================================================================*/

/**
 * Get the containing structure from a list node.
 */
#define MR_CONTAINER_OF(ptr, type, member) \
    ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

/**
 * Get the TCB from a state node.
 */
#define MR_TCB_FROM_STATE_NODE(node) \
    MR_CONTAINER_OF(node, mr_tcb_t, state_node)

/**
 * Get the TCB from an event node.
 */
#define MR_TCB_FROM_EVENT_NODE(node) \
    MR_CONTAINER_OF(node, mr_tcb_t, event_node)

/**
 * Convert milliseconds to ticks.
 */
#define MR_MS_TO_TICKS(ms) \
    (((uint32_t)(ms) * MR_TICK_RATE_HZ) / 1000U)

/**
 * Convert ticks to milliseconds.
 */
#define MR_TICKS_TO_MS(ticks) \
    (((uint32_t)(ticks) * 1000U) / MR_TICK_RATE_HZ)

/*===========================================================================*/
/* Assertion Support                                                          */
/*===========================================================================*/

#if MR_USE_ASSERT
    /**
     * Assertion failure handler (MISRA Rule 17.3 compliant declaration)
     *
     * @param file  Source file name
     * @param line  Source line number
     */
    extern MR_NORETURN void mr_assert_failed(const char *file, int line);

    #define MR_ASSERT(expr) \
        do { if (!(expr)) { mr_assert_failed(__FILE__, __LINE__); } } while(0)
#else
    #define MR_ASSERT(expr) ((void)0)
#endif

/*===========================================================================*/
/* Type Safety Verification (MISRA Directive 4.6)                             */
/*===========================================================================*/

/* Verify critical type sizes at compile time */
MR_STATIC_ASSERT(sizeof(mr_status_t) >= 1, "mr_status_t too small");
MR_STATIC_ASSERT(sizeof(mr_task_state_t) >= 1, "mr_task_state_t too small");

/* Verify TCB alignment for efficient access */
MR_STATIC_ASSERT(
    offsetof(struct mr_tcb, stack_ptr) == 0,
    "stack_ptr must be first member of TCB for context switch"
);

/*===========================================================================*/
/* Safe Type Conversion Helpers (MISRA Rules 10.x)                            */
/*===========================================================================*/

/**
 * Convert milliseconds to ticks with overflow protection
 */
MR_INLINE uint32_t mr_ms_to_ticks_safe(uint32_t ms)
{
    uint32_t ticks;
    /* Check for potential overflow */
    if (ms > (UINT32_MAX / MR_TICK_RATE_HZ)) {
        ticks = UINT32_MAX;
    } else {
        ticks = (ms * (uint32_t)MR_TICK_RATE_HZ) / 1000U;
    }
    return ticks;
}

/**
 * Convert ticks to milliseconds with overflow protection
 */
MR_INLINE uint32_t mr_ticks_to_ms_safe(uint32_t ticks)
{
    uint32_t ms;
    /* Check for potential overflow */
    if (ticks > (UINT32_MAX / 1000U)) {
        ms = UINT32_MAX;
    } else {
        ms = (ticks * 1000U) / (uint32_t)MR_TICK_RATE_HZ;
    }
    return ms;
}

#ifdef __cplusplus
}
#endif

#endif /* MR_TYPES_H */
