import { cn } from '@/lib/cn';

/**
 * Stylized "Arduino-style" PCB SVG with a pulsing on-board LED.
 *
 * Pure SVG + CSS animation. Used as the hero's secondary visual
 * next to the terminal mock.
 */
export function BoardMock({ className }: { className?: string }) {
  return (
    <div
      className={cn(
        'relative aspect-[5/3] overflow-hidden rounded-2xl border border-fd-border bg-gradient-to-br from-emerald-900/20 to-emerald-950/40 p-6 shadow-2xl shadow-emerald-900/30 backdrop-blur',
        className,
      )}
    >
      {/* PCB grid texture */}
      <svg
        className="absolute inset-0 size-full opacity-20"
        aria-hidden
        xmlns="http://www.w3.org/2000/svg"
      >
        <defs>
          <pattern id="pcb-grid" width="14" height="14" patternUnits="userSpaceOnUse">
            <path
              d="M 14 0 L 0 0 0 14"
              fill="none"
              stroke="currentColor"
              strokeWidth="0.5"
              className="text-emerald-400"
            />
          </pattern>
          <radialGradient id="pcb-glow" cx="50%" cy="50%" r="50%">
            <stop offset="0%" stopColor="hsl(150 60% 35%)" stopOpacity="0.3" />
            <stop offset="100%" stopColor="hsl(150 60% 15%)" stopOpacity="0" />
          </radialGradient>
        </defs>
        <rect width="100%" height="100%" fill="url(#pcb-grid)" />
        <rect width="100%" height="100%" fill="url(#pcb-glow)" />
      </svg>

      {/* Traces */}
      <svg
        className="absolute inset-0 size-full"
        viewBox="0 0 500 300"
        preserveAspectRatio="none"
        aria-hidden
        xmlns="http://www.w3.org/2000/svg"
      >
        <g stroke="hsl(150 70% 40%)" strokeWidth="1.5" fill="none" opacity="0.6">
          <path d="M 50 60 L 200 60 L 200 100 L 350 100" />
          <path d="M 50 120 L 150 120 L 150 180 L 320 180" />
          <path d="M 100 240 L 280 240 L 280 200 L 440 200" />
          <path d="M 380 50 L 380 130 L 460 130" />
          <path d="M 70 200 L 70 260 L 200 260" />
        </g>
        {/* Pads/vias */}
        <g fill="hsl(45 80% 60%)" opacity="0.8">
          <circle cx="50" cy="60" r="2.5" />
          <circle cx="200" cy="100" r="2.5" />
          <circle cx="350" cy="100" r="2.5" />
          <circle cx="380" cy="50" r="2.5" />
          <circle cx="460" cy="130" r="2.5" />
          <circle cx="50" cy="120" r="2.5" />
          <circle cx="320" cy="180" r="2.5" />
          <circle cx="100" cy="240" r="2.5" />
          <circle cx="440" cy="200" r="2.5" />
          <circle cx="200" cy="260" r="2.5" />
        </g>
      </svg>

      {/* MCU chip */}
      <div className="absolute left-1/2 top-1/2 z-10 -translate-x-1/2 -translate-y-1/2">
        <div className="relative">
          <div className="grid h-20 w-32 grid-cols-2 place-content-center rounded-md bg-zinc-900 text-center shadow-xl ring-1 ring-zinc-700">
            <div className="font-mono text-[8px] uppercase tracking-wider text-zinc-400">
              ATmega
            </div>
            <div className="font-mono text-[8px] uppercase tracking-wider text-zinc-400">
              328P
            </div>
            <div className="col-span-2 font-mono text-[10px] font-bold text-zinc-300">
              MICRORTOS
            </div>
          </div>
          {/* Pins along the top and bottom */}
          <div className="absolute -top-1 left-0 right-0 flex justify-between px-2">
            {Array.from({ length: 8 }).map((_, i) => (
              <span
                key={i}
                className="h-1 w-1 rounded-sm bg-zinc-500"
                aria-hidden
              />
            ))}
          </div>
          <div className="absolute -bottom-1 left-0 right-0 flex justify-between px-2">
            {Array.from({ length: 8 }).map((_, i) => (
              <span
                key={i}
                className="h-1 w-1 rounded-sm bg-zinc-500"
                aria-hidden
              />
            ))}
          </div>
        </div>
      </div>

      {/* Pulsing LED (top-right corner) */}
      <div className="absolute right-8 top-8 flex items-center gap-2">
        <div className="relative">
          <span className="absolute inset-0 animate-glow rounded-full bg-amber-400 blur-md" />
          <span className="relative block size-3 rounded-full bg-amber-300 shadow-lg shadow-amber-500/50" />
        </div>
        <span className="font-mono text-[10px] uppercase tracking-wider text-emerald-300/70">
          PIN 13
        </span>
      </div>

      {/* Caption */}
      <div className="absolute bottom-5 left-5 right-5 flex items-center justify-between">
        <span className="font-mono text-[10px] uppercase tracking-wider text-emerald-300/60">
          Arduino Uno · 16 MHz · 32 KB
        </span>
        <span className="rounded-full bg-emerald-400/10 px-2 py-0.5 text-[10px] font-medium text-emerald-300 ring-1 ring-emerald-400/20">
          ● LIVE
        </span>
      </div>
    </div>
  );
}
