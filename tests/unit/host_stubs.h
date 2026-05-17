/**
 * Public surface of the host-test stubs. See host_stubs.c.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef HOST_STUBS_H
#define HOST_STUBS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Reset all stub counters and kernel globals before each test. */
void host_stubs_reset(void);

/* Test-visible trip counters. */
extern unsigned host_stub_yield_count;
extern unsigned host_stub_add_ready_count;
extern unsigned host_stub_remove_ready_count;

#ifdef __cplusplus
}
#endif

#endif /* HOST_STUBS_H */
