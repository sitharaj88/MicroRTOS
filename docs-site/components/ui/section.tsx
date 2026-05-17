import { cn } from '@/lib/cn';
import type { ComponentProps, ReactNode } from 'react';
import { Container } from './container';

/**
 * Vertical section wrapper. Variants control the background treatment;
 * `tight` controls vertical padding.
 */
export function Section({
  className,
  variant = 'plain',
  tight = false,
  children,
  ...props
}: ComponentProps<'section'> & {
  variant?: 'plain' | 'subtle' | 'mesh' | 'grid';
  tight?: boolean;
}) {
  return (
    <section
      className={cn(
        'relative isolate',
        variant === 'subtle' && 'border-y border-fd-border bg-fd-card/30',
        variant === 'mesh' && 'overflow-hidden bg-mesh',
        variant === 'grid' && 'overflow-hidden',
        className,
      )}
      {...props}
    >
      {variant === 'grid' ? (
        <div className="bg-grid absolute inset-0 -z-10" aria-hidden />
      ) : null}
      <Container className={cn(tight ? 'py-12' : 'py-20 sm:py-24')}>
        {children}
      </Container>
    </section>
  );
}

export function SectionHeader({
  eyebrow,
  title,
  description,
  align = 'center',
  className,
}: {
  eyebrow?: ReactNode;
  title: ReactNode;
  description?: ReactNode;
  align?: 'center' | 'start';
  className?: string;
}) {
  return (
    <div
      className={cn(
        'flex flex-col gap-4',
        align === 'center' ? 'mx-auto max-w-2xl text-center' : 'max-w-2xl',
        className,
      )}
    >
      {eyebrow ? (
        <div
          className={cn(
            'inline-flex w-fit items-center gap-2 rounded-full bg-fd-primary/10 px-3 py-1 text-xs font-semibold uppercase tracking-wider text-fd-primary',
            align === 'center' && 'mx-auto',
          )}
        >
          {eyebrow}
        </div>
      ) : null}
      <h2 className="text-balance text-3xl font-bold tracking-tight text-fd-foreground sm:text-4xl">
        {title}
      </h2>
      {description ? (
        <p className="text-pretty text-lg text-fd-muted-foreground">{description}</p>
      ) : null}
    </div>
  );
}
