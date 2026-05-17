import { cn } from '@/lib/cn';
import type { LucideIcon } from 'lucide-react';
import type { ReactNode } from 'react';

/**
 * Small "tag" chip that floats around the hero visual.
 * Variants determine the float animation phase.
 */
export function FloatingChip({
  icon: Icon,
  children,
  variant = 'a',
  accent = false,
  className,
}: {
  icon?: LucideIcon;
  children: ReactNode;
  variant?: 'a' | 'b' | 'c';
  accent?: boolean;
  className?: string;
}) {
  const animClass =
    variant === 'a'
      ? 'animate-chip-float-a'
      : variant === 'b'
        ? 'animate-chip-float-b'
        : 'animate-chip-float-c';
  return (
    <div
      className={cn(
        'inline-flex items-center gap-2 rounded-xl border bg-fd-card/95 px-3 py-2 text-xs font-medium shadow-lg backdrop-blur',
        accent
          ? 'border-fd-accent/30 text-fd-accent shadow-fd-accent/15'
          : 'border-fd-primary/30 text-fd-foreground shadow-fd-primary/15',
        animClass,
        className,
      )}
    >
      {Icon ? (
        <Icon
          className={cn('size-3.5', accent ? 'text-fd-accent' : 'text-fd-primary')}
          strokeWidth={2.2}
        />
      ) : null}
      {children}
    </div>
  );
}
