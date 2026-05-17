import Link from 'next/link';
import {
  ArrowRight,
  Cpu,
  Gauge,
  Github,
  Layers,
  Lock,
  ShieldCheck,
  SignalHigh,
  Sparkles,
  Timer,
  Workflow,
  Zap,
} from 'lucide-react';
import { CodeBlock } from '@/components/code-block';
import { FeatureCard } from '@/components/feature-card';
import { Stat } from '@/components/stat';

const blinkExample = `#include "rtos.h"

static rtos_tcb_t  task_tcb;
static uint8_t     task_stack[128];

static void blinker(void *arg) {
    (void)arg;
    while (1) {
        PORTB ^= (1 << PB5);                   /* toggle pin 13 */
        rtos_task_delay(RTOS_MS_TO_TICKS(500));
    }
}

int main(void) {
    DDRB |= (1 << PB5);
    rtos_kernel_init();
    rtos_task_create(&task_tcb, "blink", blinker, NULL, 2,
                     task_stack, sizeof(task_stack));
    rtos_kernel_start();              /* never returns */
}`;

export default function HomePage() {
  return (
    <main className="flex flex-1 flex-col">
      {/* ============================== HERO ============================= */}
      <section className="relative isolate overflow-hidden">
        <div className="hero-glow absolute inset-0 -z-10" aria-hidden />
        <div className="grid-bg absolute inset-0 -z-10 opacity-50" aria-hidden />

        <div className="mx-auto w-full max-w-6xl px-6 py-20 sm:py-28">
          <div className="flex flex-col items-center text-center">
            <div className="mb-6 inline-flex items-center gap-2 rounded-full border border-fd-border bg-fd-card/50 px-3 py-1 text-xs font-medium text-fd-muted-foreground backdrop-blur">
              <span className="inline-block size-1.5 rounded-full bg-green-500" />
              Hardware-validated on Arduino Uno · 79 host tests passing
            </div>
            <h1 className="max-w-3xl bg-gradient-to-br from-fd-foreground via-fd-foreground to-fd-muted-foreground bg-clip-text text-4xl font-bold tracking-tight text-transparent sm:text-6xl">
              A modern RTOS for AVR &amp; ARM Cortex-M
            </h1>
            <p className="mt-6 max-w-2xl text-balance text-lg text-fd-muted-foreground">
              MicroRTOS is a small, MISRA-aligned real-time kernel with priority
              and Earliest-Deadline-First scheduling, mutexes with priority
              inheritance, queues, software timers, and a tickless mode — all in
              about <span className="font-mono text-fd-foreground">3.5 KB</span>{' '}
              of flash on an ATmega328P.
            </p>

            <div className="mt-10 flex flex-wrap items-center justify-center gap-3">
              <Link
                href="/docs/getting-started"
                className="inline-flex items-center gap-2 rounded-lg bg-fd-primary px-5 py-2.5 text-sm font-semibold text-fd-primary-foreground transition hover:opacity-90"
              >
                Get started
                <ArrowRight className="size-4" />
              </Link>
              <Link
                href="https://github.com/sitharaj88/rtos"
                className="inline-flex items-center gap-2 rounded-lg border border-fd-border bg-fd-card px-5 py-2.5 text-sm font-semibold text-fd-foreground transition hover:bg-fd-muted"
              >
                <Github className="size-4" />
                View on GitHub
              </Link>
            </div>

            {/* quick-install one-liner */}
            <div className="mt-10 w-full max-w-2xl">
              <CodeBlock language="bash">{`git clone https://github.com/sitharaj88/rtos
cd rtos
make flash EXAMPLE=10`}</CodeBlock>
            </div>
          </div>
        </div>
      </section>

      {/* ============================== STATS ============================ */}
      <section className="border-t border-fd-border bg-fd-card/30">
        <div className="mx-auto grid w-full max-w-6xl gap-4 px-6 py-12 sm:grid-cols-2 lg:grid-cols-4">
          <Stat value="~3.5 KB" label="Flash, smallest example" hint="ATmega328P release build" />
          <Stat value="~518 B" label="SRAM, smallest example" hint="kernel + one task" />
          <Stat value="79/79" label="Host unit tests" hint="list, mempool, queue" />
          <Stat value="2 ports" label="AVR + ARM Cortex-M" hint="add yours in ~500 lines" />
        </div>
      </section>

      {/* ============================ FEATURES =========================== */}
      <section className="mx-auto w-full max-w-6xl px-6 py-20">
        <div className="mx-auto max-w-2xl text-center">
          <h2 className="text-3xl font-bold tracking-tight sm:text-4xl">
            What&rsquo;s in the box
          </h2>
          <p className="mt-4 text-fd-muted-foreground">
            A complete kernel surface — not a toy. Every feature is pay-as-you-go:
            disabled features cost zero bytes thanks to link-time dead-code
            elimination.
          </p>
        </div>

        <div className="mt-12 grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
          <FeatureCard
            icon={Cpu}
            title="Preemptive scheduler"
            description="32 priority levels, optional round-robin within priority, O(1) ready-queue lookup via priority bitmap."
          />
          <FeatureCard
            icon={Timer}
            title="EDF scheduling"
            description="Earliest-Deadline-First mode for hard real-time tasks, with admission control and deadline-miss callbacks."
            accent
          />
          <FeatureCard
            icon={Lock}
            title="Priority inheritance"
            description="Mutexes with PI to prevent unbounded priority inversion. Recursive locking supported."
          />
          <FeatureCard
            icon={Workflow}
            title="Queues & zero-copy IPC"
            description="Classic message queues plus a zero-copy buffer protocol for high-throughput pipelines."
          />
          <FeatureCard
            icon={Layers}
            title="Fixed-block pools"
            description="Deterministic O(1) allocation. No fragmentation, no surprises in your worst-case timing."
          />
          <FeatureCard
            icon={SignalHigh}
            title="Event groups"
            description="Wait on any-of / all-of bit patterns with timeout. Great for state-machine signaling."
          />
          <FeatureCard
            icon={Zap}
            title="Tickless idle"
            description="Suppress the periodic tick during long sleeps for battery-powered designs."
            accent
          />
          <FeatureCard
            icon={ShieldCheck}
            title="MISRA + safety"
            description="MISRA C:2012 aligned, stack-overflow detection, assertion macros, optional task watchdog."
          />
          <FeatureCard
            icon={Gauge}
            title="Cooperative async"
            description="Duff-device coroutine macros for state machines that yield without a full TCB."
          />
        </div>
      </section>

      {/* ============================= CODE PREVIEW ====================== */}
      <section className="border-t border-fd-border">
        <div className="mx-auto grid w-full max-w-6xl gap-12 px-6 py-20 lg:grid-cols-2 lg:items-center">
          <div>
            <span className="inline-flex items-center gap-2 rounded-full bg-fd-primary/10 px-3 py-1 text-xs font-semibold text-fd-primary">
              <Sparkles className="size-3.5" />
              From boot to blink in 15 lines
            </span>
            <h2 className="mt-4 text-3xl font-bold tracking-tight sm:text-4xl">
              Build a task. Delay. That&rsquo;s it.
            </h2>
            <p className="mt-4 text-fd-muted-foreground">
              No HAL to learn, no IDF to configure. A TCB, a stack, and a function
              pointer is everything you need to launch a task. The same API works
              identically on AVR and ARM Cortex-M.
            </p>
            <ul className="mt-6 space-y-2 text-sm text-fd-muted-foreground">
              <li className="flex items-start gap-2">
                <ArrowRight className="mt-0.5 size-4 shrink-0 text-fd-primary" />
                Single header for the public API (<span className="font-mono">rtos.h</span>)
              </li>
              <li className="flex items-start gap-2">
                <ArrowRight className="mt-0.5 size-4 shrink-0 text-fd-primary" />
                Static allocation only — no malloc, no surprises
              </li>
              <li className="flex items-start gap-2">
                <ArrowRight className="mt-0.5 size-4 shrink-0 text-fd-primary" />
                One Makefile target builds, flashes, and verifies
              </li>
            </ul>
            <div className="mt-8">
              <Link
                href="/docs/getting-started/first-blink"
                className="inline-flex items-center gap-2 text-sm font-semibold text-fd-primary hover:underline"
              >
                Read the walkthrough
                <ArrowRight className="size-4" />
              </Link>
            </div>
          </div>
          <CodeBlock language="c" className="lg:order-last">
            {blinkExample}
          </CodeBlock>
        </div>
      </section>

      {/* ============================= CTA =============================== */}
      <section className="border-t border-fd-border bg-fd-card/30">
        <div className="mx-auto flex w-full max-w-6xl flex-col items-center px-6 py-20 text-center">
          <h2 className="max-w-2xl text-3xl font-bold tracking-tight sm:text-4xl">
            Ship deterministic firmware in an afternoon.
          </h2>
          <p className="mt-4 max-w-xl text-fd-muted-foreground">
            Walk through Get Started, flash an Uno, and you&rsquo;re running a real
            scheduler with priority inheritance and tickless idle.
          </p>
          <div className="mt-8 flex flex-wrap items-center justify-center gap-3">
            <Link
              href="/docs/getting-started"
              className="inline-flex items-center gap-2 rounded-lg bg-fd-primary px-5 py-2.5 text-sm font-semibold text-fd-primary-foreground transition hover:opacity-90"
            >
              Start the tutorial
              <ArrowRight className="size-4" />
            </Link>
            <Link
              href="/docs/examples"
              className="inline-flex items-center gap-2 rounded-lg border border-fd-border bg-fd-card px-5 py-2.5 text-sm font-semibold text-fd-foreground transition hover:bg-fd-muted"
            >
              Browse examples
            </Link>
          </div>
        </div>
      </section>

      {/* ============================ FOOTER ============================= */}
      <footer className="border-t border-fd-border">
        <div className="mx-auto flex w-full max-w-6xl flex-col items-center justify-between gap-4 px-6 py-8 text-sm text-fd-muted-foreground sm:flex-row">
          <div>
            <span className="font-semibold text-fd-foreground">MicroRTOS</span>{' '}
            · MIT License · © {new Date().getFullYear()}
          </div>
          <div className="flex gap-6">
            <Link href="/docs/getting-started" className="hover:text-fd-foreground">
              Docs
            </Link>
            <Link href="/docs/api" className="hover:text-fd-foreground">
              API
            </Link>
            <Link
              href="https://github.com/sitharaj88/rtos"
              className="hover:text-fd-foreground"
            >
              GitHub
            </Link>
          </div>
        </div>
      </footer>
    </main>
  );
}
