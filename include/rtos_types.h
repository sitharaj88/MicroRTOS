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

#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "rtos_config.h"
#include "rtos_misra.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Forward Declarations                                                       */
/*===========================================================================*/

typedef struct rtos_tcb rtos_tcb_t;
typedef struct rtos_list rtos_list_t;
typedef struct rtos_list_node rtos_list_node_t;

/*===========================================================================*/
/* Status and Return Codes                                                    */
/*===========================================================================*/

/**
 * RTOS function return status codes.
 */
typedef enum rtos_status {
    RTOS_OK              = 0,     /**< Operation completed successfully */
    RTOS_ERR_TIMEOUT     = 1,     /**< Timeout waiting for resource */
    RTOS_ERR_FULL        = 2,     /**< Resource is full (queue, pool) */
    RTOS_ERR_EMPTY       = 3,     /**< Resource is empty (queue) */
    RTOS_ERR_PARAM       = 4,     /**< Invalid parameter */
    RTOS_ERR_STATE       = 5,     /**< Invalid state for operation */
    RTOS_ERR_NOMEM       = 6,     /**< Out of memory */
    RTOS_ERR_DELETED     = 7,     /**< Object was deleted */
    RTOS_ERR_ISR         = 8,     /**< Invalid call from ISR context */
    RTOS_ERR_RECURSIVE   = 9,     /**< Recursive lock not allowed */
    RTOS_ERR_NOT_OWNER   = 10,    /**< Not the mutex owner */
    RTOS_ERR_OVERFLOW    = 11,    /**< Stack overflow detected */
    RTOS_ERR_NO_MEMORY   = 12,    /**< No memory available */
    RTOS_ERR_NOT_SUPPORTED = 13,  /**< Feature not supported */
} rtos_status_t;

/**
 * Task states.
 */
typedef enum rtos_task_state {
    RTOS_TASK_READY      = 0,     /**< Task is ready to run */
    RTOS_TASK_RUNNING    = 1,     /**< Task is currently running */
    RTOS_TASK_BLOCKED    = 2,     /**< Task is blocked on resource */
    RTOS_TASK_SUSPENDED  = 3,     /**< Task is suspended */
    RTOS_TASK_DELETED    = 4,     /**< Task has been deleted */
} rtos_task_state_t;

/**
 * Block reasons (why a task is blocked).
 */
typedef enum rtos_block_reason {
    RTOS_BLOCK_NONE      = 0,     /**< Not blocked */
    RTOS_BLOCK_DELAY     = 1,     /**< Blocked on delay */
    RTOS_BLOCK_MUTEX     = 2,     /**< Blocked on mutex */
    RTOS_BLOCK_SEM       = 3,     /**< Blocked on semaphore */
    RTOS_BLOCK_QUEUE_SEND = 4,    /**< Blocked on queue send */
    RTOS_BLOCK_QUEUE_RECV = 5,    /**< Blocked on queue receive */
    RTOS_BLOCK_EVENT     = 6,     /**< Blocked on event group */
    RTOS_BLOCK_NOTIFY    = 7,     /**< Blocked on notification */
    RTOS_BLOCK_MEMPOOL   = 8,     /**< Blocked on memory pool */
} rtos_block_reason_t;

/*===========================================================================*/
/* List Structures (Intrusive Doubly-Linked List)                             */
/*===========================================================================*/

/**
 * List node structure (embedded in other structures).
 */
struct rtos_list_node {
    rtos_list_node_t *next;       /**< Next node in list */
    rtos_list_node_t *prev;       /**< Previous node in list */
    void *container;              /**< Pointer to containing structure */
    uint32_t value;               /**< Sort value (priority, tick, etc.) */
};

/**
 * List head structure.
 */
struct rtos_list {
    rtos_list_node_t *head;       /**< First node in list */
    rtos_list_node_t *tail;       /**< Last node in list */
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
struct rtos_tcb {
    /* Critical context switch data - DO NOT REORDER */
    void *stack_ptr;              /**< Current stack pointer (must be first!) */

    /* Priority management */
    uint8_t priority;             /**< Current priority (may be boosted) */
    uint8_t base_priority;        /**< Original assigned priority */

    /* Task state */
    rtos_task_state_t state;      /**< Current task state */
    rtos_block_reason_t block_reason; /**< Why task is blocked */

    /* Timing */
    uint32_t delay_ticks;         /**< Remaining ticks for delay/timeout */
    uint32_t wake_tick;           /**< Absolute tick to wake up */

    /* List nodes for various queues */
    rtos_list_node_t state_node;  /**< Node in ready/delayed list */
    rtos_list_node_t event_node;  /**< Node in event/resource wait list */

    /* Stack management */
    void *stack_base;             /**< Stack base address (for overflow check) */
    uint16_t stack_size;          /**< Stack size in bytes */

    /* Blocking object */
    void *blocked_on;             /**< Object task is blocked on */

#if RTOS_USE_TASK_NOTIFICATIONS
    /* Task notifications */
    uint32_t notify_value;        /**< Notification value */
    uint8_t notify_state;         /**< Notification state */
#endif

#if RTOS_USE_MUTEXES
    /* Mutex handling for priority inheritance */
    struct rtos_mutex *mutex_held; /**< List of held mutexes */
    uint8_t mutexes_held_count;    /**< Number of mutexes held */
#endif

#if RTOS_USE_TASK_NAMES
    const char *name;             /**< Task name for debugging */
#endif

#if RTOS_USE_RUNTIME_STATS
    uint32_t runtime_ticks;       /**< Total runtime in ticks */
    uint32_t switch_in_tick;      /**< Tick when task started running */
#endif

    /* Time slice tracking */
#if RTOS_USE_TIMESLICING
    uint8_t timeslice_remaining;  /**< Remaining time slice ticks */
#endif
};

/*===========================================================================*/
/* Synchronization Object Types                                               */
/*===========================================================================*/

#if RTOS_USE_MUTEXES
/**
 * Mutex structure with priority inheritance support.
 */
typedef struct rtos_mutex {
    rtos_tcb_t *owner;            /**< Current owner task */
    rtos_list_t wait_list;        /**< Tasks waiting for mutex */
    uint8_t lock_count;           /**< Recursive lock count */
    struct rtos_mutex *next;      /**< Next mutex in owner's held list */
} rtos_mutex_t;
#endif

#if RTOS_USE_SEMAPHORES
/**
 * Semaphore structure (binary or counting).
 */
typedef struct rtos_sem {
    volatile uint32_t count;      /**< Current count */
    uint32_t max_count;           /**< Maximum count (1 for binary) */
    rtos_list_t wait_list;        /**< Tasks waiting for semaphore */
} rtos_sem_t;
#endif

#if RTOS_USE_QUEUES
/**
 * Message queue structure.
 */
typedef struct rtos_queue {
    uint8_t *buffer;              /**< Message buffer */
    uint16_t item_size;           /**< Size of each item */
    uint16_t capacity;            /**< Maximum number of items */
    volatile uint16_t count;      /**< Current number of items */
    uint16_t head;                /**< Read index */
    uint16_t tail;                /**< Write index */
    rtos_list_t send_wait_list;   /**< Tasks waiting to send */
    rtos_list_t recv_wait_list;   /**< Tasks waiting to receive */
} rtos_queue_t;
#endif

#if RTOS_USE_EVENT_GROUPS
/**
 * Event group structure.
 */
typedef struct rtos_event {
    volatile uint32_t bits;       /**< Current event bits */
    rtos_list_t wait_list;        /**< Tasks waiting for events */
} rtos_event_t;

/**
 * Event wait info (stored in task while waiting).
 */
typedef struct rtos_event_wait_info {
    uint32_t wait_bits;           /**< Bits to wait for */
    bool wait_all;                /**< Wait for all bits */
    bool clear_on_exit;           /**< Clear bits on exit */
} rtos_event_wait_info_t;
#endif

#if RTOS_USE_TIMERS
/**
 * Software timer states.
 */
typedef enum rtos_timer_state {
    RTOS_TIMER_STOPPED   = 0,     /**< Timer is stopped */
    RTOS_TIMER_RUNNING   = 1,     /**< Timer is running */
    RTOS_TIMER_EXPIRED   = 2,     /**< Timer has expired */
} rtos_timer_state_t;

/**
 * Software timer structure.
 */
typedef struct rtos_timer {
    rtos_list_node_t node;        /**< Node in timer list */
    uint32_t period_ticks;        /**< Timer period in ticks */
    uint32_t expire_tick;         /**< Next expiration tick */
    void (*callback)(struct rtos_timer *timer); /**< Callback function */
    void *user_data;              /**< User data pointer */
    rtos_timer_state_t state;     /**< Timer state */
    bool auto_reload;             /**< Auto-reload after expiration */
#if RTOS_USE_TASK_NAMES
    const char *name;             /**< Timer name for debugging */
#endif
} rtos_timer_t;
#endif

#if RTOS_USE_MEMORY_POOLS
/**
 * Memory pool structure.
 */
typedef struct rtos_mempool {
    void *buffer;                 /**< Pool memory buffer */
    void *free_list;              /**< Head of free block list */
    uint16_t block_size;          /**< Size of each block */
    uint16_t block_count;         /**< Total number of blocks */
    volatile uint16_t free_count; /**< Number of free blocks */
    rtos_list_t wait_list;        /**< Tasks waiting for blocks */
} rtos_mempool_t;
#endif

/*===========================================================================*/
/* Task Notification States                                                   */
/*===========================================================================*/

#if RTOS_USE_TASK_NOTIFICATIONS
typedef enum rtos_notify_state {
    RTOS_NOTIFY_NOT_WAITING = 0,  /**< Not waiting for notification */
    RTOS_NOTIFY_WAITING     = 1,  /**< Waiting for notification */
    RTOS_NOTIFY_RECEIVED    = 2,  /**< Notification received */
} rtos_notify_state_t;

typedef enum rtos_notify_action {
    RTOS_NOTIFY_SET_VALUE   = 0,  /**< Set notification value */
    RTOS_NOTIFY_INCREMENT   = 1,  /**< Increment notification value */
    RTOS_NOTIFY_SET_BITS    = 2,  /**< Set bits in notification value */
    RTOS_NOTIFY_NO_ACTION   = 3,  /**< Just wake the task */
} rtos_notify_action_t;
#endif

/*===========================================================================*/
/* Kernel State                                                               */
/*===========================================================================*/

/**
 * Kernel states.
 */
typedef enum rtos_kernel_state {
    RTOS_KERNEL_NOT_STARTED = 0,  /**< Kernel not yet started */
    RTOS_KERNEL_RUNNING     = 1,  /**< Kernel is running */
    RTOS_KERNEL_SUSPENDED   = 2,  /**< Scheduler suspended */
} rtos_kernel_state_t;

/*===========================================================================*/
/* Utility Macros                                                             */
/*===========================================================================*/

/**
 * Get the containing structure from a list node.
 */
#define RTOS_CONTAINER_OF(ptr, type, member) \
    ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

/**
 * Get the TCB from a state node.
 */
#define RTOS_TCB_FROM_STATE_NODE(node) \
    RTOS_CONTAINER_OF(node, rtos_tcb_t, state_node)

/**
 * Get the TCB from an event node.
 */
#define RTOS_TCB_FROM_EVENT_NODE(node) \
    RTOS_CONTAINER_OF(node, rtos_tcb_t, event_node)

/**
 * Convert milliseconds to ticks.
 */
#define RTOS_MS_TO_TICKS(ms) \
    (((uint32_t)(ms) * RTOS_TICK_RATE_HZ) / 1000U)

/**
 * Convert ticks to milliseconds.
 */
#define RTOS_TICKS_TO_MS(ticks) \
    (((uint32_t)(ticks) * 1000U) / RTOS_TICK_RATE_HZ)

/*===========================================================================*/
/* Assertion Support                                                          */
/*===========================================================================*/

#if RTOS_USE_ASSERT
    /**
     * Assertion failure handler (MISRA Rule 17.3 compliant declaration)
     *
     * @param file  Source file name
     * @param line  Source line number
     */
    extern RTOS_NORETURN void rtos_assert_failed(const char *file, int line);

    #define RTOS_ASSERT(expr) \
        do { if (!(expr)) { rtos_assert_failed(__FILE__, __LINE__); } } while(0)
#else
    #define RTOS_ASSERT(expr) ((void)0)
#endif

/*===========================================================================*/
/* Type Safety Verification (MISRA Directive 4.6)                             */
/*===========================================================================*/

/* Verify critical type sizes at compile time */
RTOS_STATIC_ASSERT(sizeof(rtos_status_t) >= 1, "rtos_status_t too small");
RTOS_STATIC_ASSERT(sizeof(rtos_task_state_t) >= 1, "rtos_task_state_t too small");

/* Verify TCB alignment for efficient access */
RTOS_STATIC_ASSERT(
    offsetof(struct rtos_tcb, stack_ptr) == 0,
    "stack_ptr must be first member of TCB for context switch"
);

/*===========================================================================*/
/* Safe Type Conversion Helpers (MISRA Rules 10.x)                            */
/*===========================================================================*/

/**
 * Convert milliseconds to ticks with overflow protection
 */
RTOS_INLINE uint32_t rtos_ms_to_ticks_safe(uint32_t ms)
{
    uint32_t ticks;
    /* Check for potential overflow */
    if (ms > (UINT32_MAX / RTOS_TICK_RATE_HZ)) {
        ticks = UINT32_MAX;
    } else {
        ticks = (ms * (uint32_t)RTOS_TICK_RATE_HZ) / 1000U;
    }
    return ticks;
}

/**
 * Convert ticks to milliseconds with overflow protection
 */
RTOS_INLINE uint32_t rtos_ticks_to_ms_safe(uint32_t ticks)
{
    uint32_t ms;
    /* Check for potential overflow */
    if (ticks > (UINT32_MAX / 1000U)) {
        ms = UINT32_MAX;
    } else {
        ms = (ticks * 1000U) / (uint32_t)RTOS_TICK_RATE_HZ;
    }
    return ms;
}

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TYPES_H */
