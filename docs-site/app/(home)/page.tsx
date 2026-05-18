import Link from 'next/link';
import {
  ArrowRight,
  BookOpen,
  ChevronRight,
  Cpu,
  Feather,
  Gauge,
  Github,
  Layers,
  Lightbulb,
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
import { TrustRow } from '@/components/marketing/trust-row';
import { FloatingChip } from '@/components/marketing/floating-chip';
import { ScrollHint } from '@/components/marketing/scroll-hint';
import { LiveTicker } from '@/components/marketing/live-ticker';
import { AuthorByline, author } from '@/components/marketing/author-byline';
import { LogoMark } from '@/components/ui/logo';

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

type FooterItem = readonly [label: string, href: string, external?: boolean];

function FooterColumn({
  title,
  items,
}: {
  title: string;
  items: readonly FooterItem[];
}) {
  return (
    <div>
      <div className="mb-3 text-xs font-bold uppercase tracking-widest text-fd-muted-foreground">
        {title}
      </div>
      <ul className="space-y-2.5">
        {items.map(([label, href, external]) => (
          <li key={href}>
            <Link
              href={href}
              className="text-fd-foreground transition-colors hover:text-fd-primary"
              {...(external
                ? { target: '_blank', rel: 'noreferrer noopener' }
                : {})}
            >
              {label}
            </Link>
          </li>
        ))}
      </ul>
    </div>
  );
}

export default function HomePage() {
  return (
    <main className="flex flex-1 flex-col">
      {/* =========================== HERO ============================== */}
      <section className="relative isolate overflow-hidden">
        <OrbBackground />
        <div className="bg-grid absolute inset-0 -z-10 opacity-60" aria-hidden />
        {/* Soft top fade so the hero blends into the page nav */}
        <div
          aria-hidden
          className="pointer-events-none absolute inset-x-0 top-0 -z-10 h-32 bg-gradient-to-b from-fd-background to-transparent"
        />

        <Container className="relative pb-16 pt-12 sm:pb-24 sm:pt-24 lg:pb-28 lg:pt-28">
          <div className="flex flex-col items-center text-center">
            {/* Big brand mark + project name */}
            <div className="mb-6 flex items-center gap-3">
              <LogoMark size={44} className="drop-shadow-[0_4px_24px_rgba(59,130,246,0.4)]" />
              <span className="text-2xl font-bold tracking-tight text-fd-foreground">
                MicroRTOS
              </span>
            </div>

            {/* Compact live status with pulsing dot.
                Shorten the message on phones to keep the pill on one line. */}
            <LiveTicker
              text="Live on Arduino Uno · 1 Hz blink"
              className="hidden sm:inline-flex"
            />
            <LiveTicker
              text="Live on Uno · 1 Hz"
              className="sm:hidden"
            />

            <h1 className="mt-6 max-w-4xl text-balance text-[2.25rem] font-bold leading-[1.1] tracking-tightest text-fd-foreground sm:mt-7 sm:text-5xl md:text-6xl lg:text-7xl lg:leading-[1.05]">
              A real-time kernel{' '}
              <span className="text-gradient-flow">
                you can read in an afternoon
              </span>
            </h1>

            <p className="mt-5 max-w-2xl text-balance text-base text-fd-muted-foreground sm:mt-7 sm:text-lg md:text-xl">
              MicroRTOS is a 3.5 KB real-time kernel for AVR and ARM
              Cortex-M — priority &amp; EDF scheduling, priority-inheritance
              mutexes, queues, timers, and tickless idle.
            </p>

            <div className="mt-7 flex flex-wrap items-center justify-center gap-2 sm:mt-10 sm:gap-3">
              <ButtonLink
                href="/docs/getting-started"
                size="lg"
                iconRight={<ArrowRight className="size-4" />}
                className="relative isolate"
              >
                <span
                  aria-hidden
                  className="absolute -inset-1 -z-10 rounded-xl bg-gradient-to-r from-fd-primary to-fd-accent opacity-50 blur-lg"
                />
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
                href="https://github.com/sitharaj88/MicroRTOS"
                size="lg"
                variant="ghost"
                iconLeft={<Github className="size-4" />}
              >
                GitHub
              </ButtonLink>
            </div>

            {/* Trust signals row */}
            <TrustRow className="mt-9" />

            {/* Author byline */}
            <AuthorByline className="mt-6" />
          </div>

          {/* Hero visual — code + board side by side, integrated with chips */}
          <div className="relative mt-12 sm:mt-16 lg:mt-20">
            {/* Connector wire between code and board (lg only) */}
            <svg
              aria-hidden
              className="pointer-events-none absolute inset-0 hidden h-full w-full text-fd-primary lg:block"
              preserveAspectRatio="none"
              viewBox="0 0 1200 600"
            >
              <defs>
                <linearGradient id="wireGrad" x1="0" y1="0" x2="1" y2="0">
                  <stop offset="0%" stopColor="hsl(var(--fd-primary))" stopOpacity="0" />
                  <stop offset="50%" stopColor="hsl(var(--fd-primary))" stopOpacity="0.7" />
                  <stop offset="100%" stopColor="hsl(var(--fd-accent))" stopOpacity="0" />
                </linearGradient>
              </defs>
              <path
                d="M 640 360 C 720 360, 760 220, 880 220"
                fill="none"
                stroke="url(#wireGrad)"
                strokeWidth="2"
                className="wire-dashed"
              />
            </svg>

            <div className="grid items-start gap-6 sm:gap-8 lg:grid-cols-5">
              <div className="relative min-w-0 lg:col-span-3">
                {/* Floating chips — hidden on phones / small tablets so
                    they don't crowd the visual. Show from md (768px). */}
                <FloatingChip
                  icon={Feather}
                  className="absolute -left-3 -top-5 z-20 hidden md:inline-flex"
                  variant="a"
                >
                  Zero <span className="font-mono">malloc</span>
                </FloatingChip>
                <FloatingChip
                  icon={Lock}
                  accent
                  className="absolute -bottom-4 left-10 z-20 hidden md:inline-flex"
                  variant="c"
                >
                  Priority inheritance
                </FloatingChip>

                <SyntaxCode
                  title="examples/10_blink_taskdelay/main.c"
                  language="c"
                  badge="LIVE on Uno"
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

              <div className="relative min-w-0 lg:col-span-2">
                {/* Floating chip — top right, only from md so it doesn't
                    overflow the container on phones. */}
                <FloatingChip
                  icon={Lightbulb}
                  accent
                  className="absolute -right-2 -top-5 z-20 hidden md:inline-flex"
                  variant="b"
                >
                  Pin 13 blinking · 1 Hz
                </FloatingChip>

                <BoardMock className="animate-float-slow" />

                {/* Live stat chips beneath board */}
                <div className="mt-4 grid grid-cols-3 gap-2 text-center sm:mt-5 sm:gap-3">
                  <div className="group rounded-xl border border-fd-border bg-fd-card/60 px-2 py-2 backdrop-blur transition hover:border-fd-primary/30 sm:px-3 sm:py-2.5">
                    <div className="font-mono text-sm font-bold text-fd-foreground sm:text-base">
                      3.5 KB
                    </div>
                    <div className="text-[10px] uppercase tracking-wider text-fd-muted-foreground">
                      Flash
                    </div>
                  </div>
                  <div className="group rounded-xl border border-fd-border bg-fd-card/60 px-2 py-2 backdrop-blur transition hover:border-fd-primary/30 sm:px-3 sm:py-2.5">
                    <div className="font-mono text-sm font-bold text-fd-foreground sm:text-base">
                      518 B
                    </div>
                    <div className="text-[10px] uppercase tracking-wider text-fd-muted-foreground">
                      SRAM
                    </div>
                  </div>
                  <div className="group rounded-xl border border-fd-border bg-fd-card/60 px-2 py-2 backdrop-blur transition hover:border-fd-primary/30 sm:px-3 sm:py-2.5">
                    <div className="font-mono text-sm font-bold text-fd-foreground sm:text-base">
                      1 ms
                    </div>
                    <div className="text-[10px] uppercase tracking-wider text-fd-muted-foreground">
                      Tick
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>

          {/* Logo strip */}
          <div className="mt-20">
            <LogoStrip />
          </div>

          {/* Scroll hint */}
          <div className="mt-16 flex justify-center">
            <ScrollHint />
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
        <Container className="py-12 sm:py-16">
          {/* Top row: brand cluster + three link columns.
              Mobile  → brand on top, then a 2-col link grid.
              sm      → brand + 3-col link grid stacked.
              lg      → 12-col split (brand 4, links 8 = 3×).
              Equal-weight link columns keep the eye moving horizontally. */}
          <div className="grid gap-10 sm:gap-12 lg:grid-cols-12 lg:gap-10">
            <div className="lg:col-span-4">
              <Link href="/" className="inline-flex items-center gap-2.5">
                <LogoMark size={32} />
                <span className="text-xl font-bold tracking-tight text-fd-foreground">
                  MicroRTOS
                </span>
              </Link>
              <p className="mt-4 max-w-sm text-sm leading-relaxed text-fd-muted-foreground">
                A small, MISRA-aligned real-time kernel for AVR and ARM
                Cortex-M. Built to be read, audited, and shipped.
              </p>
              <p className="mt-4 text-xs text-fd-muted-foreground">
                Crafted by{' '}
                <Link
                  href={author.site}
                  target="_blank"
                  rel="noreferrer noopener"
                  className="font-semibold text-fd-foreground hover:text-fd-primary"
                >
                  {author.name}
                </Link>
              </p>
              <AuthorByline variant="icons" className="mt-5" />
            </div>

            <nav
              aria-label="Footer"
              className="grid grid-cols-2 gap-8 text-sm sm:grid-cols-3 sm:gap-10 lg:col-span-8"
            >
              <FooterColumn
                title="Docs"
                items={[
                  ['Get started', '/docs/getting-started'],
                  ['Tutorials', '/docs/tutorials'],
                  ['Concepts', '/docs/concepts'],
                  ['API reference', '/docs/api'],
                  ['Performance', '/docs/performance'],
                ]}
              />
              <FooterColumn
                title="Resources"
                items={[
                  ['Examples', '/docs/examples'],
                  ['Porting', '/docs/porting'],
                  ['FAQ', '/docs/faq'],
                  ['Comparison', '/docs/comparison'],
                ]}
              />
              <FooterColumn
                title="Project"
                items={[
                  ['GitHub repo', 'https://github.com/sitharaj88/MicroRTOS', true],
                  ['Issues', 'https://github.com/sitharaj88/MicroRTOS/issues', true],
                  ['MISRA report', 'https://github.com/sitharaj88/MicroRTOS/blob/main/docs/MISRA_COMPLIANCE.md', true],
                  ['MIT License', 'https://github.com/sitharaj88/MicroRTOS/blob/main/LICENSE', true],
                ]}
              />
            </nav>
          </div>

          {/* Bottom bar — separator, copyright, tagline.
              Stacks on mobile, sits side-by-side from sm up. */}
          <div className="mt-12 flex flex-col-reverse gap-3 border-t border-fd-border pt-6 text-xs text-fd-muted-foreground sm:mt-14 sm:flex-row sm:items-center sm:justify-between">
            <p>
              <Link
                href="https://github.com/sitharaj88/MicroRTOS/blob/main/LICENSE"
                target="_blank"
                rel="noreferrer noopener"
                className="font-semibold text-fd-foreground hover:text-fd-primary"
              >
                MIT License
              </Link>
              {' · '}© {new Date().getFullYear()} {author.name}
            </p>
            <p className="inline-flex items-center gap-1.5">
              <span aria-hidden className="inline-block size-1.5 rounded-full bg-emerald-500" />
              Hardware-validated on Arduino Uno
            </p>
          </div>
        </Container>
      </footer>
    </main>
  );
}
