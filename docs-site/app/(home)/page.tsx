import Link from 'next/link';
import {
  ArrowRight,
  BookOpen,
  ChevronRight,
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
import { BentoGrid, BentoCard } from '@/components/marketing/bento';
import { ComparisonTable } from '@/components/marketing/comparison';
import { StatGrid, Eyebrow } from '@/components/marketing/stat-grid';
import { OrbBackground } from '@/components/marketing/orb-background';
import { BoardMock } from '@/components/marketing/board-mock';
import { ArchitectureDiagram } from '@/components/marketing/architecture';
import { FootprintChart } from '@/components/marketing/footprint-chart';
import { SyntaxCode, tok } from '@/components/marketing/syntax-code';
import { CodeStack } from '@/components/marketing/code-stack';
import { LogoStrip } from '@/components/marketing/logo-strip';

const faq = [
  {
    q: 'Is MicroRTOS a fork of FreeRTOS?',
    a: 'No. It is an independent kernel with a different code structure and API. The two share a feature surface (priority scheduling, mutexes with PI, queues, timers, etc.) because those are the well-known primitives, but every line is written from scratch.',
  },
  {
    q: 'Why "Micro"? How small can it really get?',
    a: 'The smallest useful program is around 3.5 KB of flash and 518 B of SRAM on an ATmega328P — kernel plus one task plus a single mr_task_delay loop. Every advanced feature is gated by an MR_USE_* flag; disabled features cost zero bytes thanks to link-time dead-code elimination.',
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
      <section className="relative isolate overflow-hidden">
        <OrbBackground />
        <div className="bg-grid absolute inset-0 -z-10 opacity-60" aria-hidden />

        <Container className="relative pb-16 pt-20 sm:pb-20 sm:pt-28 lg:pb-24 lg:pt-32">
          <div className="flex flex-col items-center text-center">
            <Pill shimmer icon={Star}>
              <span className="font-semibold text-fd-foreground">
                Hardware-validated
              </span>{' '}
              · 79 host tests · MISRA-aligned · MIT
            </Pill>

            <h1 className="mt-7 max-w-4xl text-balance text-5xl font-bold tracking-tightest text-fd-foreground sm:text-6xl lg:text-7xl">
              The kernel{' '}
              <span className="text-gradient-brand">small enough to read</span>,
              real enough to ship.
            </h1>

            <p className="mt-7 max-w-2xl text-balance text-lg text-fd-muted-foreground sm:text-xl">
              MicroRTOS is a 3.5 KB real-time kernel for AVR and ARM Cortex-M
              with priority and EDF scheduling, priority-inheritance mutexes,
              queues, software timers, and a tickless mode — and you can audit
              the whole thing in an afternoon.
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
                href="/docs/tutorials"
                size="lg"
                variant="secondary"
                iconRight={<ChevronRight className="size-4" />}
              >
                Browse tutorials
              </ButtonLink>
              <ButtonLink
                href="https://github.com/sitharaj88/rtos"
                size="lg"
                variant="ghost"
                iconLeft={<Github className="size-4" />}
              >
                GitHub
              </ButtonLink>
            </div>
          </div>

          {/* Hero visual — code + board side by side */}
          <div className="mt-16 grid gap-6 lg:grid-cols-5 lg:items-center">
            <div className="lg:col-span-3 lg:row-start-1">
              <SyntaxCode
                title="examples/10_blink_taskdelay/main.c"
                language="c"
                badge="LIVE on Uno"
                className="lg:max-w-[640px]"
              >
                {[
                  <span key="i1">
                    <span className={tok.keyword}>#include</span>{' '}
                    <span className={tok.string}>&quot;micrortos.h&quot;</span>
                    {'\n'}
                  </span>,
                  '\n',
                  <span key="t1">
                    <span className={tok.keyword}>static</span>{' '}
                    <span className={tok.type}>mr_tcb_t</span> task_tcb;{'\n'}
                  </span>,
                  <span key="t2">
                    <span className={tok.keyword}>static</span>{' '}
                    <span className={tok.type}>uint8_t</span>  task_stack[
                    <span className={tok.number}>128</span>];{'\n'}
                  </span>,
                  '\n',
                  <span key="f1">
                    <span className={tok.keyword}>static void</span>{' '}
                    <span className={tok.fn}>blinker</span>(
                    <span className={tok.type}>void</span> *arg) {'{'}
                    {'\n'}
                  </span>,
                  <span key="f2">
                    {'    '}(<span className={tok.type}>void</span>)arg;{'\n'}
                  </span>,
                  <span key="f3">
                    {'    '}
                    <span className={tok.keyword}>while</span> (
                    <span className={tok.number}>1</span>) {'{'}
                    {'\n'}
                  </span>,
                  <span key="f4">
                    {'        '}PORTB ^= (<span className={tok.number}>1</span>{' '}
                    &lt;&lt; PB5);{'\n'}
                  </span>,
                  <span key="f5">
                    {'        '}
                    <span className={tok.fn}>mr_task_delay</span>(
                    <span className={tok.fn}>MR_MS_TO_TICKS</span>(
                    <span className={tok.number}>500</span>));{'\n'}
                  </span>,
                  <span key="f6">
                    {'    }'}
                    {'\n'}
                  </span>,
                  <span key="f7">{'}'}{'\n'}</span>,
                  '\n',
                  <span key="m1">
                    <span className={tok.keyword}>int</span>{' '}
                    <span className={tok.fn}>main</span>(
                    <span className={tok.type}>void</span>) {'{'}
                    {'\n'}
                  </span>,
                  <span key="m2">
                    {'    '}DDRB |= (<span className={tok.number}>1</span>{' '}
                    &lt;&lt; PB5);{'\n'}
                  </span>,
                  <span key="m3">
                    {'    '}
                    <span className={tok.fn}>mr_kernel_init</span>();{'\n'}
                  </span>,
                  <span key="m4">
                    {'    '}
                    <span className={tok.fn}>mr_task_create</span>(&amp;task_tcb,{' '}
                    <span className={tok.string}>&quot;blink&quot;</span>,
                    blinker, <span className={tok.keyword}>NULL</span>,{' '}
                    <span className={tok.number}>2</span>,{'\n'}
                  </span>,
                  <span key="m5">
                    {'                   '}task_stack,{' '}
                    <span className={tok.keyword}>sizeof</span>(task_stack));
                    {'\n'}
                  </span>,
                  <span key="m6">
                    {'    '}
                    <span className={tok.fn}>mr_kernel_start</span>();{' '}
                    <span className={tok.comment}>/* never returns */</span>
                    {'\n'}
                  </span>,
                  <span key="m7">{'}'}</span>,
                ]}
              </SyntaxCode>
            </div>

            <div className="lg:col-span-2">
              <BoardMock className="animate-float-slow" />
              <div className="mt-4 grid grid-cols-3 gap-3 text-center">
                <div className="rounded-xl border border-fd-border bg-fd-card/60 px-3 py-2 backdrop-blur">
                  <div className="font-mono text-base font-bold text-fd-foreground">
                    3.5 KB
                  </div>
                  <div className="text-[10px] uppercase tracking-wider text-fd-muted-foreground">
                    Flash
                  </div>
                </div>
                <div className="rounded-xl border border-fd-border bg-fd-card/60 px-3 py-2 backdrop-blur">
                  <div className="font-mono text-base font-bold text-fd-foreground">
                    518 B
                  </div>
                  <div className="text-[10px] uppercase tracking-wider text-fd-muted-foreground">
                    SRAM
                  </div>
                </div>
                <div className="rounded-xl border border-fd-border bg-fd-card/60 px-3 py-2 backdrop-blur">
                  <div className="font-mono text-base font-bold text-fd-foreground">
                    1 ms
                  </div>
                  <div className="text-[10px] uppercase tracking-wider text-fd-muted-foreground">
                    Tick
                  </div>
                </div>
              </div>
            </div>
          </div>

          {/* Logo strip */}
          <div className="mt-16">
            <LogoStrip />
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

      {/* ====================== ARCHITECTURE =========================== */}
      <Section>
        <div className="grid gap-14 lg:grid-cols-2 lg:items-center">
          <div>
            <Eyebrow>
              <Layers className="size-3.5" /> How it&rsquo;s built
            </Eyebrow>
            <h2 className="mt-4 text-balance text-3xl font-bold tracking-tight text-fd-foreground sm:text-4xl">
              Three layers. One header.{' '}
              <span className="text-gradient-brand">Zero magic.</span>
            </h2>
            <p className="mt-5 text-pretty text-fd-muted-foreground">
              Your code calls one umbrella header. The portable kernel does
              the work in roughly 5,000 lines you can read in an afternoon.
              A small port layer handles the platform-specific bits.
            </p>
            <ul className="mt-7 space-y-3 text-sm">
              {[
                ['Application', 'Includes micrortos.h. That’s the entire public surface.'],
                ['Kernel', 'Tasks, scheduler, sync, IPC, mempool, timers, safety.'],
                ['Port', 'Context save/restore, tick timer, critical sections. ~500 LOC.'],
              ].map(([k, v]) => (
                <li key={k} className="flex items-start gap-3">
                  <span className="mt-0.5 inline-flex size-5 items-center justify-center rounded-full bg-fd-primary/10 text-[10px] font-bold text-fd-primary">
                    →
                  </span>
                  <span className="text-fd-foreground/90">
                    <span className="font-semibold text-fd-foreground">
                      {k}.
                    </span>{' '}
                    {v}
                  </span>
                </li>
              ))}
            </ul>
            <div className="mt-8">
              <ButtonLink
                href="/docs/concepts/architecture"
                variant="ghost"
                size="md"
                iconRight={<ArrowRight className="size-4" />}
              >
                Read the architecture
              </ButtonLink>
            </div>
          </div>
          <ArchitectureDiagram />
        </div>
      </Section>

      {/* ========================= FEATURES ============================ */}
      <Section variant="subtle">
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

      {/* ========================= FOOTPRINT =========================== */}
      <Section>
        <div className="grid gap-12 lg:grid-cols-2 lg:items-center">
          <div className="order-2 lg:order-1">
            <FootprintChart
              title="Minimum flash (one task)"
              rows={[
                { label: 'MicroRTOS', pct: 35, value: '3.5 KB', variant: 'primary' },
                { label: 'FreeRTOS', pct: 65, value: '~6 KB' },
                { label: 'Zephyr', pct: 100, value: '~10 KB' },
              ]}
            />
            <FootprintChart
              title="Minimum SRAM (kernel state, no stacks)"
              rows={[
                { label: 'MicroRTOS', pct: 50, value: '~250 B', variant: 'primary' },
                { label: 'FreeRTOS', pct: 50, value: '~250 B' },
                { label: 'Zephyr', pct: 100, value: '~500 B' },
              ]}
              className="mt-4"
            />
            <FootprintChart
              title="Audit surface (kLOC to review)"
              rows={[
                { label: 'MicroRTOS', pct: 5, value: '~5 kLOC', variant: 'primary' },
                { label: 'FreeRTOS', pct: 20, value: '~20 kLOC' },
                { label: 'Zephyr', pct: 100, value: '~500 kLOC' },
              ]}
              className="mt-4"
            />
          </div>
          <div className="order-1 lg:order-2">
            <Eyebrow>
              <Gauge className="size-3.5" /> Footprint
            </Eyebrow>
            <h2 className="mt-4 text-balance text-3xl font-bold tracking-tight text-fd-foreground sm:text-4xl">
              A quarter of FreeRTOS,{' '}
              <span className="text-gradient-brand">a hundredth of Zephyr</span>
            </h2>
            <p className="mt-5 text-pretty text-fd-muted-foreground">
              On AVR `-Os` MicroRTOS fits in 3.5 KB of flash for a minimal
              app. It does so by including <em>only</em> a kernel — drivers,
              filesystems, and BSPs are explicitly out of scope. You bring
              what you need; nothing else takes up space.
            </p>
            <p className="mt-4 text-pretty text-fd-muted-foreground">
              The audit surface is the metric we&rsquo;re proudest of. Five
              thousand lines is something a single engineer can review for
              safety. Five hundred thousand is not.
            </p>
            <div className="mt-7 flex flex-wrap gap-3">
              <ButtonLink href="/docs/performance" variant="secondary">
                See detailed footprint
              </ButtonLink>
              <ButtonLink href="/docs/comparison" variant="ghost">
                Compare features
              </ButtonLink>
            </div>
          </div>
        </div>
      </Section>

      {/* ========================== TUTORIALS CTA ====================== */}
      <Section variant="subtle">
        <SectionHeader
          eyebrow={<><BookOpen className="size-3.5" /> Tutorials</>}
          title={<>Hands-on walkthroughs for every concept</>}
          description="Each one picks a realistic embedded scenario and ends with a working program you can flash. Same template across all eleven."
        />
        <div className="mt-12 grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
          {[
            { t: 'Two tasks at different priorities', d: 'Sensor + UI loops running independently.', href: '/docs/tutorials/two-tasks', e: 'Fundamentals' },
            { t: 'Sharing a peripheral with a mutex', d: 'SPI bus + priority inheritance.', href: '/docs/tutorials/mutex-peripheral', e: 'Fundamentals' },
            { t: 'ISR-to-task signaling', d: 'UART RX → semaphore → parser task.', href: '/docs/tutorials/isr-signaling', e: 'Fundamentals' },
            { t: 'Producer / consumer with a queue', d: 'Sensor pushes, writer drains, back-pressure visible.', href: '/docs/tutorials/queue-producer-consumer', e: 'IPC' },
            { t: 'Memory pools', d: 'O(1) allocation, no fragmentation, no malloc.', href: '/docs/tutorials/memory-pools', e: 'IPC' },
            { t: 'Battery-friendly tickless idle', d: 'Stop the tick. 40× current draw reduction.', href: '/docs/tutorials/tickless-low-power', e: 'Power' },
          ].map((t) => (
            <Link
              key={t.href}
              href={t.href}
              className="group flex flex-col gap-2 rounded-2xl border border-fd-border bg-fd-card p-5 transition-all hover:-translate-y-0.5 hover:border-fd-primary/40 hover:shadow-lg hover:shadow-fd-primary/10"
            >
              <span className="text-[10px] font-bold uppercase tracking-widest text-fd-primary">
                {t.e}
              </span>
              <span className="text-base font-semibold tracking-tight text-fd-foreground">
                {t.t}
              </span>
              <span className="text-sm leading-relaxed text-fd-muted-foreground">
                {t.d}
              </span>
              <span className="mt-2 inline-flex items-center gap-1 text-sm font-semibold text-fd-primary transition group-hover:gap-2">
                Read <ChevronRight className="size-4" />
              </span>
            </Link>
          ))}
        </div>
        <div className="mt-10 text-center">
          <ButtonLink
            href="/docs/tutorials"
            variant="ghost"
            iconRight={<ArrowRight className="size-4" />}
          >
            All 11 tutorials
          </ButtonLink>
        </div>
      </Section>

      {/* ======================== COMPARISON =========================== */}
      <Section>
        <SectionHeader
          eyebrow={<><Workflow className="size-3.5" /> Side-by-side</>}
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
        <div className="mx-auto mt-12 grid max-w-5xl gap-4 sm:grid-cols-2">
          {faq.map((item) => (
            <div
              key={item.q}
              className="group rounded-2xl border border-fd-border bg-fd-card p-6 transition hover:border-fd-primary/30"
            >
              <div className="mb-2 flex items-center gap-2 text-sm font-semibold text-fd-foreground">
                <span className="inline-flex size-5 items-center justify-center rounded-full bg-fd-primary/10 text-[10px] font-bold text-fd-primary">
                  ?
                </span>
                {item.q}
              </div>
              <p className="ml-7 text-sm leading-relaxed text-fd-muted-foreground">
                {item.a}
              </p>
            </div>
          ))}
        </div>
        <div className="mt-10 text-center">
          <ButtonLink href="/docs/faq" variant="ghost" iconRight={<ArrowRight className="size-4" />}>
            Read the full FAQ
          </ButtonLink>
        </div>
      </Section>

      {/* ============================= CTA ============================= */}
      <section className="relative isolate overflow-hidden">
        <OrbBackground />
        <div className="bg-grid absolute inset-0 -z-10 opacity-50" aria-hidden />
        <Container className="relative py-20 sm:py-28">
          <div className="mx-auto flex max-w-3xl flex-col items-center text-center">
            <Pill icon={Rocket}>Hardware-validated · MIT licensed</Pill>
            <h2 className="mt-6 text-balance text-4xl font-bold tracking-tight text-fd-foreground sm:text-5xl lg:text-6xl">
              From clone to running task in{' '}
              <span className="text-gradient-brand">five minutes</span>
            </h2>
            <p className="mt-5 text-balance text-lg text-fd-muted-foreground">
              Get the build going on your machine, plug in an Arduino Uno,
              and `make flash EXAMPLE=10` lights an LED that proves the
              whole stack works.
            </p>
            <div className="mt-9 flex flex-wrap items-center justify-center gap-3">
              <ButtonLink
                href="/docs/getting-started"
                size="lg"
                iconRight={<ArrowRight className="size-4" />}
              >
                Start the tutorial
              </ButtonLink>
              <ButtonLink href="/docs/api" size="lg" variant="secondary">
                API reference
              </ButtonLink>
            </div>
          </div>
        </Container>
      </section>

      {/* =========================== FOOTER ============================ */}
      <footer className="border-t border-fd-border bg-fd-card/40">
        <Container className="flex flex-col gap-10 py-14 lg:flex-row lg:items-start lg:justify-between">
          <div className="max-w-sm">
            <div className="flex items-center gap-2">
              <span
                aria-hidden
                className="inline-block size-6 rounded-md bg-gradient-to-br from-fd-primary to-fd-accent shadow-lg shadow-fd-primary/30"
              />
              <span className="text-lg font-semibold tracking-tight">MicroRTOS</span>
            </div>
            <p className="mt-3 text-sm text-fd-muted-foreground">
              A small, MISRA-aligned real-time kernel for AVR and ARM
              Cortex-M. Built to be read, audited, and shipped.
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
