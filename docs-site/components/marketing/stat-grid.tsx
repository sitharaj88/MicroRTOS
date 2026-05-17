import { cn } from '@/lib/cn';
import type { ReactNode } from 'react';

type StatItem = {
  value: string;
  label: string;
  hint?: string;
  accent?: boolean;
};

export function StatGrid({
  items,
  className,
}: {
  items: StatItem[];
  className?: string;
}) {
  return (
    <div
      className={cn(
        'grid gap-px overflow-hidden rounded-2xl border border-fd-border bg-fd-border sm:grid-cols-2 lg:grid-cols-4',
        className,
      )}
    >
      {items.map((s) => (
        <div
          key={s.label}
          className="relative bg-fd-card px-6 py-7 transition hover:bg-fd-card/60"
        >
          {s.accent ? (
            <div
              aria-hidden
              className="absolute inset-x-0 top-0 h-px bg-gradient-to-r from-transparent via-fd-primary to-transparent"
            />
          ) : null}
          <div
            className={cn(
              'font-mono text-3xl font-bold tracking-tight sm:text-4xl',
              s.accent ? 'text-fd-primary' : 'text-fd-foreground',
            )}
          >
            {s.value}
          </div>
          <div className="mt-2 text-sm font-medium text-fd-foreground">
            {s.label}
          </div>
          {s.hint ? (
            <div className="mt-1 text-xs text-fd-muted-foreground">{s.hint}</div>
          ) : null}
        </div>
      ))}
    </div>
  );
}

export function Eyebrow({ children, className }: { children: ReactNode; className?: string }) {
  return (
    <span
      className={cn(
        'inline-flex items-center gap-1.5 rounded-full bg-fd-primary/10 px-3 py-1 text-xs font-semibold uppercase tracking-wider text-fd-primary',
        className,
      )}
    >
      {children}
    </span>
  );
}
