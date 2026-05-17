import { cn } from '@/lib/cn';
import { Cpu, Layers, Workflow } from 'lucide-react';

/**
 * Layered architecture diagram. Three stacked "cards" representing the
 * kernel/port/hardware sandwich, with a subtle 3D-feel via offsets and
 * gradient borders.
 */
export function ArchitectureDiagram({ className }: { className?: string }) {
  const layers = [
    {
      label: 'Application',
      sub: '#include "micrortos.h"',
      icon: Workflow,
      pills: ['Tasks', 'Mutex', 'Queue', 'Timer'],
      hue: 'accent',
    },
    {
      label: 'Kernel (portable)',
      sub: 'src/core, src/sync, src/ipc, src/memory, src/time, src/diag',
      icon: Layers,
      pills: [
        'Scheduler',
        'Mutex/Sem/Event',
        'Queue/ZeroCopy',
        'Mempool',
        'Timers',
        'Safety',
      ],
      hue: 'primary',
    },
    {
      label: 'Port layer',
      sub: 'src/port/avr · src/port/arm',
      icon: Cpu,
      pills: ['SAVE_CONTEXT', 'RESTORE_CONTEXT', 'Tick ISR', 'Critical'],
      hue: 'primary',
    },
  ] as const;

  return (
    <div className={cn('relative flex flex-col gap-4', className)}>
      {layers.map((layer, i) => (
        <div
          key={layer.label}
          className={cn(
            'group relative overflow-hidden rounded-2xl border bg-fd-card p-5 shadow-xl transition-transform duration-300 hover:-translate-y-0.5',
            layer.hue === 'accent'
              ? 'border-fd-accent/30 shadow-fd-accent/10'
              : 'border-fd-primary/30 shadow-fd-primary/10',
          )}
          style={{ transform: `translateX(${i * 6}px)` }}
        >
          {/* Gradient sheen on hover */}
          <div
            aria-hidden
            className={cn(
              'pointer-events-none absolute -right-20 -top-20 size-60 rounded-full opacity-0 blur-3xl transition-opacity duration-500 group-hover:opacity-60',
              layer.hue === 'accent' ? 'bg-fd-accent/30' : 'bg-fd-primary/30',
            )}
          />
          <div className="relative flex flex-wrap items-start justify-between gap-3">
            <div className="flex items-start gap-3">
              <div
                className={cn(
                  'flex size-10 items-center justify-center rounded-lg ring-1',
                  layer.hue === 'accent'
                    ? 'bg-fd-accent/10 text-fd-accent ring-fd-accent/20'
                    : 'bg-fd-primary/10 text-fd-primary ring-fd-primary/20',
                )}
              >
                <layer.icon className="size-5" strokeWidth={2} />
              </div>
              <div>
                <div className="text-sm font-bold uppercase tracking-wider text-fd-foreground">
                  {layer.label}
                </div>
                <div className="mt-0.5 font-mono text-xs text-fd-muted-foreground">
                  {layer.sub}
                </div>
              </div>
            </div>
            <div className="flex flex-wrap gap-1.5">
              {layer.pills.map((p) => (
                <span
                  key={p}
                  className={cn(
                    'rounded-md border px-2 py-0.5 font-mono text-[10px] font-medium',
                    layer.hue === 'accent'
                      ? 'border-fd-accent/20 bg-fd-accent/10 text-fd-accent'
                      : 'border-fd-primary/20 bg-fd-primary/10 text-fd-primary',
                  )}
                >
                  {p}
                </span>
              ))}
            </div>
          </div>
        </div>
      ))}

      {/* Connecting line decoration */}
      <div className="-mt-1 ml-7 flex flex-col items-center text-fd-muted-foreground">
        <div className="font-mono text-xs uppercase tracking-widest">
          Hardware
        </div>
        <div className="mt-1 flex gap-2">
          {['AVR', 'Cortex-M3', 'Cortex-M0+'].map((chip) => (
            <span
              key={chip}
              className="rounded-md border border-fd-border bg-fd-muted/30 px-2 py-0.5 font-mono text-[10px] text-fd-muted-foreground"
            >
              {chip}
            </span>
          ))}
        </div>
      </div>
    </div>
  );
}
