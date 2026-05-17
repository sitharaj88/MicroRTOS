import { cn } from '@/lib/cn';
import type { ComponentProps } from 'react';

const variants = {
  primary:
    'bg-fd-primary/10 text-fd-primary border border-fd-primary/20',
  accent:
    'bg-fd-accent/10 text-fd-accent border border-fd-accent/20',
  success:
    'bg-emerald-500/10 text-emerald-600 dark:text-emerald-400 border border-emerald-500/20',
  warning:
    'bg-amber-500/10 text-amber-600 dark:text-amber-400 border border-amber-500/20',
  danger:
    'bg-red-500/10 text-red-600 dark:text-red-400 border border-red-500/20',
  neutral:
    'bg-fd-muted text-fd-muted-foreground border border-fd-border',
} as const;

export type BadgeVariant = keyof typeof variants;

export function Badge({
  variant = 'neutral',
  className,
  ...props
}: ComponentProps<'span'> & { variant?: BadgeVariant }) {
  return (
    <span
      className={cn(
        'inline-flex items-center gap-1 rounded-full px-2.5 py-0.5 text-xs font-medium',
        variants[variant],
        className,
      )}
      {...props}
    />
  );
}
