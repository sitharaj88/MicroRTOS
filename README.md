<div align="center">
  <h1>MicroRTOS</h1>
  <p><strong>A real-time kernel you can read in an afternoon.</strong></p>
  <p>
    A small, MISRA-aligned real-time operating system for AVR and ARM Cortex-M.
    Priority + EDF scheduling, mutexes with priority inheritance, queues,
    software timers, and a tickless mode — in about 3.5 KB of flash on an
    ATmega328P.
  </p>

  <p>
    <a href="https://sitharaj88.github.io/MicroRTOS/">Documentation</a>
    ·
    <a href="https://sitharaj88.github.io/MicroRTOS/docs/getting-started">Get started</a>
    ·
    <a href="https://sitharaj88.github.io/MicroRTOS/docs/tutorials">Tutorials</a>
    ·
    <a href="https://sitharaj88.github.io/MicroRTOS/docs/api">API reference</a>
  </p>
</div>

---

## Highlights

- **Tiny.** Smallest useful program: ~3.5 KB flash, ~518 B SRAM on ATmega328P.
- **Auditable.** ~5 kLOC, single public header (`micrortos.h`).
- **Hardware-validated.** Example 10 flashed and verified blinking on an
  Arduino Uno.
- **MISRA C:2012 aligned.** `make strict` ships a curated warning set.
- **Pay-as-you-go.** Every advanced feature gated by an `MR_USE_*` flag;
  disabled features cost zero bytes.
- **Two ports today.** AVR (ATmega328P, 2560, 32U4) and ARM Cortex-M
  (SAM3X8E, SAMD21). Adding a new port is ~500 lines.

## Quick start

```bash
git clone https://github.com/sitharaj88/MicroRTOS.git
cd MicroRTOS
make flash EXAMPLE=10
```

Pin 13 LED on your Uno will blink at 1 Hz.

## Documentation

The full docs site lives at **<https://sitharaj88.github.io/MicroRTOS/>** and
covers:

- Getting started (install, build, flash workflow)
- 11 hands-on tutorials (multi-task, mutex, ISR signaling, queues, event
  groups, memory pools, software timers, tickless idle, EDF, async
  coroutines, debugging stack overflow)
- 9 concept pages (architecture, scheduler, tasks, sync, IPC, memory
  pools, timers, tickless, safety)
- 11 API reference modules (kernel, tasks, mutex, semaphore, event, queue,
  mempool, timer, deadline, tickless, notifications)
- Performance breakdown (per-module flash, SRAM, sizing tips)
- Porting guide (writing a new MCU port with bring-up staircase)

## Tests

```bash
make test            # 79 host unit tests (list, mempool, queue)
make check           # AVR syntax check, default config
make check-features  # syntax check with each MR_USE_* flag enabled
make check-arm       # ARM Cortex-M syntax check (if arm-none-eabi-gcc installed)
```

## Author

**Sitharaj Seenivasan**

- Website · <https://sitharaj.in>
- GitHub · [@sitharaj88](https://github.com/sitharaj88)
- LinkedIn · [sitharaj08](https://www.linkedin.com/in/sitharaj08)
- Buy me a coffee · [sitharaj88](https://www.buymeacoffee.com/sitharaj88)

If this project saved you a weekend of yak-shaving, a coffee helps a lot.

## License

[MIT](./LICENSE) © Sitharaj Seenivasan
