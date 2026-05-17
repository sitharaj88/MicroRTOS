<div align="center">

<img src="docs-site/app/icon.svg" alt="MicroRTOS logo" width="96" height="96" />

# MicroRTOS

**A real-time kernel you can read in an afternoon.**

A small, MISRA-aligned real-time operating system for **AVR** and
**ARM Cortex-M** — priority + EDF scheduling, mutexes with priority
inheritance, queues, software timers, and a tickless mode — in about
**3.5 KB of flash** on an ATmega328P.

[![License: MIT](https://img.shields.io/badge/License-MIT-3B82F6.svg?style=flat-square)](LICENSE)
[![Language: C](https://img.shields.io/badge/Language-C99-00599C.svg?style=flat-square&logo=c&logoColor=white)](#)
[![MISRA C:2012](https://img.shields.io/badge/MISRA-C%3A2012%20aligned-22C55E.svg?style=flat-square)](docs/MISRA_COMPLIANCE.md)
[![AVR](https://img.shields.io/badge/AVR-ATmega328P%20%C2%B7%202560%20%C2%B7%2032U4-F97316.svg?style=flat-square)](#supported-hardware)
[![ARM Cortex-M](https://img.shields.io/badge/ARM-Cortex--M%20%28SAM3X%2FSAMD21%29-00ADEF.svg?style=flat-square)](#supported-hardware)
[![Flash](https://img.shields.io/badge/Flash-3.5%20KB-blueviolet.svg?style=flat-square)](#footprint)
[![SRAM](https://img.shields.io/badge/SRAM-518%20B-blueviolet.svg?style=flat-square)](#footprint)
[![Tests](https://img.shields.io/badge/Host%20tests-79%2F79-22C55E.svg?style=flat-square)](#testing)
[![Docs](https://img.shields.io/badge/Docs-sitharaj88.github.io%2FMicroRTOS-3B82F6.svg?style=flat-square)](https://sitharaj88.github.io/MicroRTOS/)

<p>
  <a href="https://sitharaj88.github.io/MicroRTOS/"><b>Website</b></a> ·
  <a href="https://sitharaj88.github.io/MicroRTOS/docs/getting-started">Get started</a> ·
  <a href="https://sitharaj88.github.io/MicroRTOS/docs/tutorials">Tutorials</a> ·
  <a href="https://sitharaj88.github.io/MicroRTOS/docs/concepts">Concepts</a> ·
  <a href="https://sitharaj88.github.io/MicroRTOS/docs/api">API</a> ·
  <a href="https://sitharaj88.github.io/MicroRTOS/docs/performance">Performance</a> ·
  <a href="https://sitharaj88.github.io/MicroRTOS/docs/porting">Porting</a>
</p>

</div>

---

## Why MicroRTOS?

Embedded RTOSes have grown in two directions: either *too tiny to ship a
real product* or *too big to read in a lifetime*. MicroRTOS sits in the
middle on purpose.

- **Small enough to audit.** ~5,000 lines, one public header
  (`micrortos.h`). One engineer can review the whole kernel for safety.
- **Real enough to ship.** Priority + EDF scheduling, mutexes with
  priority inheritance, queues with zero-copy IPC, software timers,
  event groups, memory pools, tickless idle, MISRA C:2012 alignment.
- **Hardware-validated.** The blink example flashes and runs on a real
  Arduino Uno — not a simulator.
- **Pay-as-you-go.** Every advanced feature is gated by an `MR_USE_*`
  flag. Disabled features cost **zero bytes** thanks to link-time
  dead-code elimination.

> *"Five thousand lines is something a single engineer can review for
> safety. Five hundred thousand is not."*

---

## Highlights

| | |
|---|---|
| **Flash (minimal app)** | ~3.5 KB on ATmega328P, `-Os` |
| **SRAM (kernel state)** | ~518 B (kernel + one task + 128 B stack) |
| **Public surface** | A single header — `micrortos.h` |
| **Audit surface** | ~5,000 LOC across kernel + ports |
| **Scheduler** | Preemptive priority (up to 32 levels) + optional EDF |
| **Sync primitives** | Mutex (with PI), recursive mutex, counting semaphore, event groups |
| **IPC** | Copy queues + zero-copy buffer protocol with refcounting |
| **Memory** | Fixed-block pools (O(1), no fragmentation). Kernel never calls `malloc`. |
| **Power** | Tickless idle — 40× current-draw reduction measured on Uno |
| **Safety** | MISRA C:2012 aligned, stack overflow detection, runtime assertions, optional WDT |
| **Ports** | AVR (ATmega328P / 2560 / 32U4) · ARM Cortex-M (SAM3X8E / SAMD21) |
| **License** | MIT |

---

## Quick start

```bash
git clone https://github.com/sitharaj88/MicroRTOS.git
cd MicroRTOS
make flash EXAMPLE=10
```

Pin&nbsp;13 LED on your Uno will blink at 1&nbsp;Hz. That's the entire
stack — kernel, scheduler, tick timer, task switch, your code — proven
working end-to-end on real hardware.

### Hello, blink

```c
#include "micrortos.h"
#include <avr/io.h>

static mr_tcb_t  t1;
static uint8_t   t1_stack[128];

static void blinker(void *arg) {
    (void)arg;
    while (1) {
        PORTB ^= (1 << PB5);
        mr_task_delay(MR_MS_TO_TICKS(500));
    }
}

int main(void) {
    DDRB |= (1 << PB5);
    mr_kernel_init();
    mr_task_create(&t1, "blink", blinker, NULL, 2, t1_stack, sizeof(t1_stack));
    mr_kernel_start();   /* never returns */
}
```

---

## What's in the kernel?

```
┌──────────────────────────────────────────────────────────────┐
│                       Your application                        │
│           #include "micrortos.h"   <- one header              │
├──────────────────────────────────────────────────────────────┤
│                          Kernel                               │
│   tasks │ scheduler │ mutex │ semaphore │ event │ queue       │
│   mempool │ software timer │ tickless │ notifications │ safety│
├──────────────────────────────────────────────────────────────┤
│                          Port                                 │
│  context save/restore · tick timer · critical sections  ~500  │
└──────────────────────────────────────────────────────────────┘
```

Three layers. One public header. Zero magic.

---

## Supported hardware

| Family | Chip | Board | Status |
|---|---|---|---|
| AVR | ATmega328P | Arduino Uno / Nano | ✅ flashed & verified blink |
| AVR | ATmega2560 | Arduino Mega | ✅ syntax-checked, port present |
| AVR | ATmega32U4 | Arduino Leonardo | ✅ syntax-checked, port present |
| ARM Cortex-M3 | SAM3X8E | Arduino Due | ✅ syntax-checked |
| ARM Cortex-M0+ | SAMD21 | Arduino Zero / MKR | ✅ syntax-checked |
| *your MCU here* | — | — | ~500 LOC port — see [Porting guide](https://sitharaj88.github.io/MicroRTOS/docs/porting) |

---

## Footprint

|  | Flash (min app) | SRAM (kernel state) | Audit surface |
|---|---:|---:|---:|
| **MicroRTOS** | **3.5 KB** | **~250 B** | **~5 kLOC** |
| FreeRTOS | ~6 KB | ~250 B | ~20 kLOC |
| Zephyr | ~10 KB | ~500 B | ~500 kLOC |

> "A quarter of FreeRTOS, a hundredth of Zephyr."

Full module-by-module breakdown:
<https://sitharaj88.github.io/MicroRTOS/docs/performance>

---

## Examples

The repo ships **12 examples** that double as end-to-end tests:

| # | Example | Demonstrates |
|---:|---|---|
| 00 | `blink_baremetal` | Sanity check — no kernel, just port toggle |
| 01 | `basic_tasks` | Multiple tasks at different priorities |
| 02 | `mutex_demo` | Priority inheritance in action |
| 03 | `producer_consumer` | Queues for back-pressured IPC |
| 04 | `timer_events` | Software timers + event groups |
| 05 | `tickless_low_power` | 40× current-draw reduction |
| 06 | `smp_dual_core` | Multi-core (host build only) |
| 07 | `async_await` | Cooperative coroutines without a full TCB |
| 08 | `edf_deadline` | Earliest-deadline-first scheduling |
| 09 | `blink_busywait` | Diagnostic baseline |
| 10 | `blink_taskdelay` | **Validated on Arduino Uno** |
| 11 | `diag_blink` | Diagnostic blink patterns |

```bash
make flash EXAMPLE=N      # build + flash example N (AVR)
```

---

## Build & test

```bash
make                      # build the kernel library
make examples             # build all examples
make flash EXAMPLE=10     # flash example 10 to Uno

make test                 # 79 host unit tests (list · mempool · queue)
make check                # AVR syntax check, default config
make check-features       # AVR syntax check with every MR_USE_* flag on
make check-arm            # ARM Cortex-M syntax check
make check-arm-features   # ARM, per-feature

make strict               # curated MISRA-aligned warning set
make analyze              # static analysis
make misra-check          # MISRA rule scan
make metrics              # flash / SRAM size report
```

Requires `avr-gcc`, `avr-libc`, and `avrdude` for AVR; `arm-none-eabi-gcc`
for the ARM checks; system `gcc` for the host unit tests.

---

## Testing

- ✅ **79 / 79** host unit tests passing (list, mempool, queue)
- ✅ AVR syntax-checked with **every `MR_USE_*` feature** enabled
- ✅ ARM Cortex-M syntax-checked, per feature
- ✅ Example 10 flashed and **verified blinking** on a real Arduino Uno
- ✅ MISRA C:2012 aligned (curated warning set under `make strict`)

---

## Documentation

The full documentation site lives at
**<https://sitharaj88.github.io/MicroRTOS/>** and covers:

- **Getting started** — install, build, flash workflow
- **11 hands-on tutorials** — multi-task, mutex, ISR signaling, queues,
  event groups, memory pools, software timers, tickless idle, EDF,
  async coroutines, debugging stack overflow
- **9 concept pages** — architecture, scheduler, tasks, sync, IPC,
  memory pools, timers, tickless, safety
- **11 API reference modules** — kernel, tasks, mutex, semaphore,
  event, queue, mempool, timer, deadline, tickless, notifications
- **Performance** — per-module flash, SRAM, sizing tips
- **Porting guide** — writing a new MCU port with a bring-up staircase

---

## Project layout

```
.
├── include/           public umbrella header (micrortos.h)
├── src/
│   ├── kernel/        scheduler, tasks, sync, IPC, mempool, timers
│   ├── port/avr/      AVR port (ATmega328P / 2560 / 32U4)
│   └── port/arm/      ARM Cortex-M port (SAM3X8E / SAMD21)
├── examples/          12 hands-on examples (00–11)
├── tests/             host unit tests
├── docs/              MISRA report, design notes
├── docs-site/         Next.js documentation site
├── Makefile           build + flash + check + test orchestration
└── LICENSE            MIT
```

---

## Roadmap

- [x] Priority scheduler with PI mutexes
- [x] Queues + zero-copy IPC
- [x] Software timers, event groups, memory pools
- [x] Tickless idle (40× current-draw reduction on Uno)
- [x] EDF scheduling with admission control
- [x] AVR + ARM Cortex-M ports
- [x] Hardware-validated on Arduino Uno
- [ ] RISC-V port (RV32IMC)
- [ ] ESP32-S3 port
- [ ] Static-analysis CI gate
- [ ] Long-form porting tutorial with worked SAMD21 walkthrough

Have a port you'd like to see? [Open an issue.](https://github.com/sitharaj88/MicroRTOS/issues)

---

## Contributing

Issues and pull requests are welcome. Before submitting a PR please:

1. Run `make test` — all 79 host tests must pass.
2. Run `make check && make check-features` (and `make check-arm*` if you
   have `arm-none-eabi-gcc`) — every feature flag should still build.
3. Run `make strict` and resolve any new warnings.
4. Keep the diff small and focused.

If you're porting to a new MCU, the [Porting guide](https://sitharaj88.github.io/MicroRTOS/docs/porting) walks
through it step-by-step.

---

## Author

**Sitharaj Seenivasan**

- 🌐 Website · <https://sitharaj.in>
- 💻 GitHub · [@sitharaj88](https://github.com/sitharaj88)
- 💼 LinkedIn · [sitharaj08](https://www.linkedin.com/in/sitharaj08)
- ☕ Buy me a coffee · [sitharaj88](https://www.buymeacoffee.com/sitharaj88)

If MicroRTOS saved you a weekend of yak-shaving, a coffee helps a lot.
If your company ships it in a product, do say hi.

---

## License

[MIT](./LICENSE) © 2026 Sitharaj Seenivasan

> Use it commercially, fork it, embed it, ship it — just keep the
> copyright notice. No warranty.

<div align="center">

If you find MicroRTOS useful, please consider giving it a ⭐ on
[GitHub](https://github.com/sitharaj88/MicroRTOS) — it really helps.

</div>
