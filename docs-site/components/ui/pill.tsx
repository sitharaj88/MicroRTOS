import { cn } from '@/lib/cn';
import type { LucideIcon } from 'lucide-react';
import type { ComponentProps } from 'react';

/**
 * Small "tag" with optional icon. Used as section eyebrows and
 * shimmer-style hero badges.
 */
export function Pill({
  icon: Icon,
  shimmer = false,
  className,
  children,
  ...props
}: ComponentProps<'div'> & {
  icon?: LucideIcon;
  shimmer?: boolean;
}) {
  return (
    <div
      className={cn(
        'relative inline-flex items-center gap-2 rounded-full border border-fd-border bg-fd-card/60 px-3 py-1 text-xs font-medium text-fd-muted-foreground backdrop-blur-sm',
        className,
      )}
      {...props}
    >
      {Icon ? <Icon className="size-3.5 text-fd-primary" strokeWidth={2.5} /> : null}
      <span className="relative z-10">{children}</span>
      {shimmer ? (
        <span
          aria-hidden
          className="pointer-events-none absolute inset-0 -z-0 overflow-hidden rounded-full"
        >
          <span className="absolute inset-y-0 -inset-x-2 animate-shimmer bg-gradient-to-r from-transparent via-fd-primary/15 to-transparent" />
        </span>
      ) : null}
    </div>
  );
}
