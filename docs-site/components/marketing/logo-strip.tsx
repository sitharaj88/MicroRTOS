import { cn } from '@/lib/cn';

/**
 * "Validated on" small board-name strip. Pure typography — no logos
 * (those would be more legal hassle than they're worth for a small
 * project). Looks intentional, not empty.
 */
export function LogoStrip({ className }: { className?: string }) {
  const items = [
    { name: 'Arduino Uno', tag: 'ATmega328P', validated: true },
    { name: 'Arduino Nano', tag: 'ATmega328P' },
    { name: 'Arduino Mega', tag: 'ATmega2560' },
    { name: 'Arduino Due', tag: 'SAM3X8E' },
    { name: 'Arduino Zero', tag: 'SAMD21' },
    { name: 'MKR series', tag: 'SAMD21' },
  ];
  return (
    <div className={cn('flex flex-col items-center gap-4', className)}>
      <span className="text-xs font-bold uppercase tracking-widest text-fd-muted-foreground">
        Runs on
      </span>
      <ul className="flex flex-wrap items-center justify-center gap-x-4 gap-y-2.5 text-xs sm:gap-x-6 sm:gap-y-3 sm:text-sm">
        {items.map((it) => (
          <li
            key={it.name}
            className="group flex items-center gap-1.5 text-fd-muted-foreground transition hover:text-fd-foreground"
          >
            <span className="font-medium">{it.name}</span>
            <span className="font-mono text-[10px] uppercase tracking-wider text-fd-muted-foreground/60">
              {it.tag}
            </span>
            {it.validated ? (
              <span
                className="ml-1 rounded-full bg-emerald-500/15 px-1.5 py-0.5 text-[10px] font-semibold text-emerald-600 ring-1 ring-emerald-500/20 dark:text-emerald-400"
                title="Hardware-validated"
              >
                LIVE
              </span>
            ) : null}
          </li>
        ))}
      </ul>
    </div>
  );
}
