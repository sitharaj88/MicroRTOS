import { cn } from '@/lib/cn';
import { Activity } from 'lucide-react';

/**
 * Small "live" status indicator with a pulsing dot.
 */
export function LiveTicker({
  text,
  className,
}: {
  text: string;
  className?: string;
}) {
  return (
    <div
      className={cn(
        'inline-flex items-center gap-2 rounded-full border border-emerald-500/30 bg-emerald-500/10 px-3 py-1 text-xs font-medium text-emerald-700 dark:text-emerald-300',
        className,
      )}
    >
      <span className="relative inline-flex">
        <span className="absolute inset-0 animate-led-pulse rounded-full bg-emerald-400 blur-sm" />
        <span className="relative size-1.5 rounded-full bg-emerald-400" />
      </span>
      <Activity className="size-3.5" strokeWidth={2.4} />
      <span>{text}</span>
    </div>
  );
}
