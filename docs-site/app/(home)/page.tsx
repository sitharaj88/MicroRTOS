import Link from 'next/link';
import {
  ArrowRight,
  BookOpen,
  Cpu,
  Gauge,
  Github,
  Layers,
  Lock,
  Rocket,
  ShieldCheck,
  SignalHigh,
  Sparkles,
  Star,
  Timer,
  Workflow,
  Zap,
} from 'lucide-react';
import { Section, SectionHeader } from '@/components/ui/section';
import { Container } from '@/components/ui/container';
import { Pill } from '@/components/ui/pill';
import { ButtonLink } from '@/components/ui/button';
import { CodeCard } from '@/components/ui/code';
import { BentoGrid, BentoCard } from '@/components/marketing/bento';
import { TerminalDemo } from '@/components/marketing/terminal-demo';
import { ComparisonTable } from '@/components/marketing/comparison';
import { StatGrid, Eyebrow } from '@/components/marketing/stat-grid';

const blinkExample = `#include "micrortos.h"

static mr_tcb_t  task_tcb;
static uint8_t   task_stack[128];

static void blinker(void *arg) {
    (void)arg;
    while (1) {
        PORTB ^= (1 << PB5);                   /* toggle pin 13 */
        mr_task_delay(MR_MS_TO_TICKS(500));
    }
}

int main(void) {
    DDRB |= (1 << PB5);
    mr_kernel_init();
    mr_task_create(&task_tcb, "blink", blinker, NULL, 2,
                   task_stack, sizeof(task_stack));
    mr_kernel_start();              /* never returns */
}`;

const faq = [
  {
    q: 'Is MicroRTOS a fork of FreeRTOS?',
    a: 'No. It is an independent kernel with a different code structure and API. The two share a feature surface (priority scheduling, mutexes with PI, queues, timers, etc.) because those are the well-known primitives, but every line is written from scratch.',
  },
  {
    q: 'Why "Micro"? How small can it really get?',
    a: 'The smallest useful program is around 3.5 KB of flash and 518 B of SRAM on an ATmega328P — kernel plus one task plus a single rtos_task_delay loop. Every advanced feature is gated by an MR_USE_* flag; disabled features cost zero bytes thanks to link-time dead-code elimination.',
  },
  {
    q: 'Which boards are supported today?',
    a: 'AVR ports run on ATmega328P (Uno, Nano), ATmega2560 (Mega), and ATmega32U4 (Leonardo). The ARM Cortex-M port targets SAM3X8E (Due) and SAMD21 (Zero, MKR). The build system has check targets for both so you can verify a port without flashing.',
  },
  {
    q: 'Can I use the dynamic malloc / heap from libc?',
    a: 'Yes — the kernel itself never calls malloc, but your application can. We recommend a memory pool instead (deterministic O(1), no fragmentation, no surprises) which the kernel ships with.',
  },
  {
    q: 'Is it production-ready?',
    a: 'The kernel passes 79 host unit tests, has been syntax-checked under every advanced flag on both AVR and ARM, and example 10 has been flashed and verified blinking on an Arduino Uno. It is small, auditable, and MISRA-aligned — but you should still review and test for your specific safety requirements.',
  },
  {
    q: 'How do I add a new port?',
    a: 'Implement ~12 functions defined in mr_port.h. The Porting guide walks through the AVR port as a worked example; expect ~500 lines for a new MCU and ~1–2 days of bring-up work.',
  },
];

export default function HomePage() {
  return (
    <main className="flex flex-1 flex-col">
      {/* =========================== HERO ============================== */}
      <section className="relative isolate overflow-hidden bg-mesh">
        <div className="bg-grid absolute inset-0 -z-10 opacity-60" aria-hidden />
        <Container className="relative py-20 sm:py-28 lg:py-32">
          <div className="flex flex-col items-center text-center">
            <Pill shimmer icon={Star}>
              <span className="font-semibold text-fd-foreground">
                Hardware-validated
              </span>{' '}
              · 79 host tests · MISRA-aligned
            </Pill>

            <h1 className="mt-7 max-w-4xl text-balance text-5xl font-bold tracking-tightest text-fd-foreground sm:text-6xl lg:text-7xl">
              A modern RTOS for{' '}
              <span className="text-gradient-brand">AVR &amp; ARM Cortex-M</span>
            </h1>

            <p className="mt-7 max-w-2xl text-balance text-lg text-fd-muted-foreground sm:text-xl">
              MicroRTOS is a small, MISRA-aligned real-time kernel with
              priority and EDF scheduling, priority-inheritance mutexes,
              queues, software timers, and a tickless mode — in about{' '}
              <span className="font-mono text-fd-foreground">3.5 KB</span> of
              flash on an ATmega328P.
            </p>

            <div className="mt-10 flex flex-wrap items-center justify-center gap-3">
              <ButtonLink
                href="/docs/getting-started"
                size="lg"
                iconRight={<ArrowRight className="size-4" />}
              >
                Get started
              </ButtonLink>
              <ButtonLink
                href="https://github.com/sitharaj88/rtos"
                size="lg"
                variant="secondary"
                iconLeft={<Github className="size-4" />}
              >
                View on GitHub
              </ButtonLink>
            </div>

            <div className="mt-16 w-full max-w-3xl">
              <TerminalDemo />
            </div>
          </div>
        </Container>
      </section>

      {/* =========================== STATS ============================ */}
      <Section variant="subtle" tight>
        <StatGrid
          items={[
            { value: '~3.5 KB', label: 'Flash, minimal app', hint: 'ATmega328P -Os', accent: true },
            { value: '~518 B', label: 'SRAM, minimal app', hint: 'Kernel + one task' },
            { value: '79/79', label: 'Host unit tests', hint: 'list · mempool · queue' },
            { value: '2 ports', label: 'AVR + Cortex-M', hint: 'Yours in ~500 LOC' },
          ]}
        />
      </Section>

      {/* ========================= FEATURES ============================ */}
      <Section>
        <SectionHeader
          eyebrow={<><Sparkles className="size-3.5" /> Features</>}
          title={<>Everything you need to ship deterministic firmware</>}
          description="A complete kernel surface — not a toy. Every feature is pay-as-you-go; disabled features cost zero bytes thanks to link-time dead-code elimination."
        />
        <BentoGrid className="mt-14">
          <BentoCard
            icon={Cpu}
            title="Preemptive priority scheduler"
            description="Up to 32 priorities, optional round-robin within priority, O(1) ready-queue lookup via a priority bitmap."
          />
          <BentoCard
            icon={Timer}
            accent
            title="EDF scheduling"
            description="Earliest Deadline First for hard real-time tasks, with admission control and deadline-miss callbacks. Coexists with fixed priorities."
          />
          <BentoCard
            icon={Lock}
            title="Priority inheritance"
            description="Recursive mutexes with PI to prevent unbounded priority inversion. Holders boost when a higher-priority task blocks on them."
          />
          <BentoCard
            icon={Workflow}
            title="Queues &amp; zero-copy IPC"
            description="Classic copy-in message queues plus a zero-copy buffer protocol with reference counting for high-throughput pipelines."
          />
          <BentoCard
            icon={Layers}
            title="Fixed-block memory pools"
            description="Deterministic O(1) allocation, no fragmentation. The kernel itself never calls malloc."
          />
          <BentoCard
            icon={SignalHigh}
            title="Event groups"
            description="Wait on any-of / all-of 32-bit patterns with timeout. Great for state-machine signaling and barriers."
          />
          <BentoCard
            icon={Zap}
            accent
            title="Tickless idle"
            description="Suppress the periodic tick during long sleeps. 40× current-draw reduction on the Uno in tests."
          />
          <BentoCard
            icon={ShieldCheck}
            title="MISRA + safety"
            description="MISRA C:2012 aligned, stack-overflow detection, runtime assertions, optional hardware + per-task watchdogs."
          />
          <BentoCard
            icon={Gauge}
            title="Cooperative async"
            description="Duff-device coroutine macros — sequential state machines that yield without burning a full TCB."
          />
        </BentoGrid>
      </Section>

      {/* ========================= CODE EXAMPLE ======================== */}
      <Section variant="subtle">
        <div className="grid gap-12 lg:grid-cols-2 lg:items-center">
          <div>
            <Eyebrow>
              <Rocket className="size-3.5" /> Boot to blink in 15 lines
            </Eyebrow>
            <h2 className="mt-4 text-3xl font-bold tracking-tight text-fd-foreground sm:text-4xl">
              Build a task. Delay. That&rsquo;s it.
            </h2>
            <p className="mt-4 text-pretty text-fd-muted-foreground">
              No HAL to learn, no IDF to configure. A TCB, a stack, and a
              function pointer is everything you need. The same API works
              identically on AVR and ARM Cortex-M.
            </p>
            <ul className="mt-7 space-y-3 text-sm">
              {[
                'Single header for the public API (micrortos.h)',
                'Static allocation only — no malloc, no surprises',
                'One Makefile target builds, flashes, and verifies',
                'Bring-up debug staircase ships with the examples',
              ].map((line) => (
                <li key={line} className="flex items-start gap-2.5 text-fd-foreground/90">
                  <ArrowRight className="mt-0.5 size-4 shrink-0 text-fd-primary" />
                  <span>{line}</span>
                </li>
              ))}
            </ul>
            <div className="mt-8">
              <ButtonLink
                href="/docs/getting-started/first-blink"
                variant="ghost"
                size="md"
                iconRight={<ArrowRight className="size-4" />}
              >
                Read the line-by-line walkthrough
              </ButtonLink>
            </div>
          </div>
          <CodeCard
            title="examples/10_blink_taskdelay/main.c"
            language="c"
            className="lg:order-last"
          >
            {blinkExample}
          </CodeCard>
        </div>
      </Section>

      {/* ======================== COMPARISON =========================== */}
      <Section>
        <SectionHeader
          eyebrow="How it compares"
          title="Smaller than FreeRTOS, simpler than Zephyr"
          description="The audit surface is about a quarter of FreeRTOS and a hundredth of Zephyr, with comparable features. You can read the whole kernel in an afternoon."
        />
        <div className="mt-12">
          <ComparisonTable />
        </div>
      </Section>

      {/* ============================ FAQ ============================== */}
      <Section variant="subtle">
        <SectionHeader
          eyebrow="FAQ"
          title="Common questions"
          description="If yours isn't here, open an issue on GitHub — we're trying to keep this page short and useful."
        />
        <div className="mx-auto mt-12 grid max-w-4xl gap-4 sm:grid-cols-2">
          {faq.map((item) => (
            <div
              key={item.q}
              className="rounded-2xl border border-fd-border bg-fd-card p-6"
            >
              <div className="mb-2 text-sm font-semibold text-fd-foreground">
                {item.q}
              </div>
              <p className="text-sm leading-relaxed text-fd-muted-foreground">
                {item.a}
              </p>
            </div>
          ))}
        </div>
      </Section>

      {/* ============================= CTA ============================= */}
      <section className="relative isolate overflow-hidden bg-mesh">
        <div className="bg-grid absolute inset-0 -z-10 opacity-60" aria-hidden />
        <Container className="relative py-20 sm:py-24">
          <div className="mx-auto flex max-w-3xl flex-col items-center text-center">
            <Pill icon={BookOpen}>Hardware-validated · MIT licensed</Pill>
            <h2 className="mt-6 text-balance text-4xl font-bold tracking-tight text-fd-foreground sm:text-5xl">
              Ship deterministic firmware in an afternoon
            </h2>
            <p className="mt-5 text-balance text-lg text-fd-muted-foreground">
              Walk through Get Started, flash a Uno, and you&rsquo;re running a
              real scheduler with priority inheritance and tickless idle.
            </p>
            <div className="mt-9 flex flex-wrap items-center justify-center gap-3">
              <ButtonLink
                href="/docs/getting-started"
                size="lg"
                iconRight={<ArrowRight className="size-4" />}
              >
                Start the tutorial
              </ButtonLink>
              <ButtonLink href="/docs/examples" size="lg" variant="secondary">
                Browse examples
              </ButtonLink>
            </div>
          </div>
        </Container>
      </section>

      {/* =========================== FOOTER ============================ */}
      <footer className="border-t border-fd-border bg-fd-card/30">
        <Container className="flex flex-col gap-8 py-12 lg:flex-row lg:items-start lg:justify-between">
          <div className="max-w-sm">
            <div className="flex items-center gap-2">
              <span
                aria-hidden
                className="inline-block size-5 rounded-md bg-gradient-to-br from-fd-primary to-fd-accent"
              />
              <span className="font-semibold tracking-tight">MicroRTOS</span>
            </div>
            <p className="mt-3 text-sm text-fd-muted-foreground">
              A small, MISRA-aligned real-time operating system for AVR and
              ARM Cortex-M.
            </p>
            <p className="mt-6 text-xs text-fd-muted-foreground">
              MIT License · © {new Date().getFullYear()}
            </p>
          </div>
          <div className="grid grid-cols-3 gap-8 text-sm">
            <div>
              <div className="mb-3 text-xs font-bold uppercase tracking-widest text-fd-muted-foreground">
                Docs
              </div>
              <ul className="space-y-2">
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="/docs/getting-started">Get started</Link></li>
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="/docs/tutorials">Tutorials</Link></li>
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="/docs/concepts">Concepts</Link></li>
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="/docs/api">API reference</Link></li>
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="/docs/performance">Performance</Link></li>
              </ul>
            </div>
            <div>
              <div className="mb-3 text-xs font-bold uppercase tracking-widest text-fd-muted-foreground">
                Resources
              </div>
              <ul className="space-y-2">
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="/docs/examples">Examples</Link></li>
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="/docs/porting">Porting</Link></li>
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="/docs/faq">FAQ</Link></li>
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="/docs/comparison">Comparison</Link></li>
              </ul>
            </div>
            <div>
              <div className="mb-3 text-xs font-bold uppercase tracking-widest text-fd-muted-foreground">
                Project
              </div>
              <ul className="space-y-2">
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="https://github.com/sitharaj88/rtos">GitHub</Link></li>
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="https://github.com/sitharaj88/rtos/issues">Issues</Link></li>
                <li><Link className="text-fd-foreground hover:text-fd-primary" href="https://github.com/sitharaj88/rtos/blob/master/docs/MISRA_COMPLIANCE.md">MISRA report</Link></li>
              </ul>
            </div>
          </div>
        </Container>
      </footer>
    </main>
  );
}
