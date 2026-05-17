# Example 06 - SMP Dual Core

This example slot is reserved for a Symmetric Multi-Processing demo on a
dual-core target such as the Raspberry Pi RP2040 or the Espressif ESP32-S3.

## Status

The SMP core (`src/core/rtos_smp.c`, `include/rtos_smp.h`) is in place but
requires platform glue that is **not yet implemented**:

- `rtos_port_smp_start_core()` — boot the secondary core
- `rtos_port_smp_get_core_id()` — read the current core's identifier
- `rtos_port_smp_send_ipi()` — deliver an inter-processor interrupt

Both the AVR port (single core) and the ARM Cortex-M port (single core
Cortex-M3/M4) skip these intentionally.

## What the example will look like

```c
#include "rtos.h"
#include "rtos_smp.h"

static rtos_tcb_t producer_tcb, consumer_tcb;
static uint8_t producer_stack[256], consumer_stack[256];

int main(void) {
    rtos_kernel_init();
    rtos_smp_init();

    rtos_task_create(&producer_tcb, "Prod", producer_fn, NULL, 2,
                     producer_stack, sizeof(producer_stack));
    rtos_task_create(&consumer_tcb, "Cons", consumer_fn, NULL, 2,
                     consumer_stack, sizeof(consumer_stack));

    /* Pin one task to each core */
    rtos_smp_set_affinity(&producer_tcb, RTOS_AFFINITY_CORE(0));
    rtos_smp_set_affinity(&consumer_tcb, RTOS_AFFINITY_CORE(1));

    rtos_smp_start_cores();
    rtos_kernel_start();
    return 0;
}
```

## How to enable

1. Add an RP2040 or ESP32 port under `src/port/<target>/` implementing the
   three `rtos_port_smp_*` functions.
2. Set `-DRTOS_USE_SMP=1` in your build (or flip the flag in
   `include/rtos_config.h`).
3. Extend the `Makefile` with a build target for your toolchain.
