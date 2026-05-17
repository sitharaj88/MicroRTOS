import { cn } from '@/lib/cn';

type Row = {
  label: string;
  /** Bar fill percentage relative to the largest in the set (0..100). */
  pct: number;
  /** Display value shown on the right of the bar. */
  value: string;
  variant?: 'primary' | 'neutral';
};

/**
 * Horizontal bar chart for visual comparisons (footprint vs others).
 * Pure CSS, no chart lib. Bars animate from 0 to their width on mount
 * via the `bar-grow` keyframe.
 */
export function FootprintChart({
  title,
  rows,
  className,
}: {
  title: string;
  rows: Row[];
  className?: string;
}) {
  return (
    <div
      className={cn(
        'rounded-2xl border border-fd-border bg-fd-card p-6',
        className,
      )}
    >
      <div className="mb-5 text-xs font-bold uppercase tracking-widest text-fd-muted-foreground">
        {title}
      </div>
      <ul className="space-y-4">
        {rows.map((r, i) => (
          <li key={r.label} className="grid grid-cols-[140px_1fr_auto] items-center gap-4">
            <span className="text-sm font-medium text-fd-foreground">{r.label}</span>
            <div className="relative h-2.5 overflow-hidden rounded-full bg-fd-muted">
              <div
                className={cn(
                  'h-full origin-left animate-bar-grow rounded-full',
                  r.variant === 'primary'
                    ? 'bg-gradient-to-r from-fd-primary to-fd-accent'
                    : 'bg-fd-muted-foreground/40',
                )}
                style={{
                  width: `${r.pct}%`,
                  animationDelay: `${i * 80}ms`,
                }}
              />
            </div>
            <span
              className={cn(
                'font-mono text-xs tabular-nums',
                r.variant === 'primary'
                  ? 'text-fd-foreground'
                  : 'text-fd-muted-foreground',
              )}
            >
              {r.value}
            </span>
          </li>
        ))}
      </ul>
    </div>
  );
}
