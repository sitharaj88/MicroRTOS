/**
 * Minimal host test harness. Single-process, single-file; each suite
 * declares main() and calls these macros.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>

extern int tests_run;
extern int tests_failed;

#define TEST_DEFINE_GLOBALS()                                                  \
    int tests_run = 0;                                                         \
    int tests_failed = 0

#define CHECK(cond) do {                                                       \
    tests_run++;                                                               \
    if (!(cond)) {                                                             \
        tests_failed++;                                                        \
        fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
    }                                                                          \
} while (0)

#define TEST_REPORT(suite_name) do {                                           \
    printf("[%s] Ran %d checks; %d failed.\n",                                 \
           (suite_name), tests_run, tests_failed);                             \
    return tests_failed == 0 ? 0 : 1;                                          \
} while (0)

#endif /* TEST_HARNESS_H */
