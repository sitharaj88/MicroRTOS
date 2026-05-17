import { cn } from '@/lib/cn';

export function Stat({
  value,
  label,
  hint,
  className,
}: {
  value: string;
  label: string;
  hint?: string;
  className?: string;
}) {
  return (
    <div
      className={cn(
        'rounded-xl border border-fd-border bg-fd-card/50 p-5 backdrop-blur-sm',
        className,
      )}
    >
      <div className="font-mono text-3xl font-bold tracking-tight text-fd-foreground sm:text-4xl">
        {value}
      </div>
      <div className="mt-1 text-sm font-medium text-fd-foreground">{label}</div>
      {hint ? (
        <div className="mt-1 text-xs text-fd-muted-foreground">{hint}</div>
      ) : null}
    </div>
  );
}
