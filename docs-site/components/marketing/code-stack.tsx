import { cn } from '@/lib/cn';
import type { ReactNode } from 'react';

/**
 * Two stacked code cards offset diagonally — visually communicates
 * "same code, two platforms" or "before/after".
 */
export function CodeStack({
  back,
  front,
  className,
}: {
  back: ReactNode;
  front: ReactNode;
  className?: string;
}) {
  return (
    <div className={cn('relative', className)}>
      <div className="pointer-events-none absolute -inset-6 -z-10 rounded-3xl bg-gradient-to-br from-fd-primary/10 via-transparent to-fd-accent/10 blur-2xl" aria-hidden />
      <div className="relative">
        {/* Back card, offset down-right */}
        <div className="absolute right-0 top-8 w-[92%] translate-x-6 opacity-80 blur-[0.5px] transition-all duration-300 hover:translate-x-4 hover:opacity-90 lg:w-[88%]">
          {back}
        </div>
        {/* Front card */}
        <div className="relative w-[92%] lg:w-[88%]">{front}</div>
      </div>
    </div>
  );
}
