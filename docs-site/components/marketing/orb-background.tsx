/**
 * Layered animated gradient orbs for hero sections.
 *
 * Pure CSS — no client JS required. Each orb is a large blurred radial
 * gradient that floats slowly via the `orb-*` keyframes defined in
 * tailwind.config.ts.
 */
export function OrbBackground() {
  return (
    <div className="pointer-events-none absolute inset-0 -z-10 overflow-hidden" aria-hidden>
      {/* Large cobalt orb, top-left */}
      <div className="absolute -left-32 -top-32 size-[36rem] animate-orb-1 rounded-full bg-fd-primary/30 opacity-60 blur-3xl dark:opacity-50" />
      {/* Amber orb, mid-right */}
      <div className="absolute -right-32 top-40 size-[32rem] animate-orb-2 rounded-full bg-fd-accent/25 opacity-50 blur-3xl dark:opacity-40" />
      {/* Cyan accent, bottom */}
      <div className="absolute bottom-0 left-1/3 size-[28rem] animate-orb-1 rounded-full bg-cyan-500/20 opacity-50 blur-3xl dark:opacity-35" />
      {/* Subtle violet, top-right */}
      <div className="absolute right-1/4 top-0 size-[22rem] animate-pulse-soft rounded-full bg-violet-500/15 blur-3xl dark:bg-violet-500/25" />
    </div>
  );
}
