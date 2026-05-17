import Link from 'next/link';
import { cn } from '@/lib/cn';
import type { ComponentProps, ReactNode } from 'react';

const sizes = {
  sm: 'h-9 px-4 text-sm',
  md: 'h-11 px-5 text-sm',
  lg: 'h-12 px-6 text-base',
} as const;

const variants = {
  primary:
    'bg-fd-primary text-fd-primary-foreground hover:opacity-90 active:opacity-80 shadow-lg shadow-fd-primary/20',
  secondary:
    'bg-fd-card text-fd-foreground border border-fd-border hover:bg-fd-muted',
  ghost:
    'text-fd-foreground hover:bg-fd-muted',
  outline:
    'border border-fd-border bg-transparent text-fd-foreground hover:bg-fd-card',
} as const;

export type ButtonVariant = keyof typeof variants;
export type ButtonSize = keyof typeof sizes;

const base =
  'inline-flex items-center justify-center gap-2 rounded-lg font-semibold transition-all duration-150 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-fd-ring focus-visible:ring-offset-2 focus-visible:ring-offset-fd-background disabled:opacity-50 disabled:pointer-events-none';

type ButtonOwnProps = {
  variant?: ButtonVariant;
  size?: ButtonSize;
  iconLeft?: ReactNode;
  iconRight?: ReactNode;
};

export function Button({
  variant = 'primary',
  size = 'md',
  iconLeft,
  iconRight,
  className,
  children,
  ...props
}: ComponentProps<'button'> & ButtonOwnProps) {
  return (
    <button
      className={cn(base, sizes[size], variants[variant], className)}
      {...props}
    >
      {iconLeft}
      {children}
      {iconRight}
    </button>
  );
}

export function ButtonLink({
  variant = 'primary',
  size = 'md',
  iconLeft,
  iconRight,
  className,
  children,
  href,
  ...props
}: Omit<ComponentProps<typeof Link>, 'href'> & ButtonOwnProps & { href: string }) {
  return (
    <Link
      href={href}
      className={cn(base, sizes[size], variants[variant], className)}
      {...props}
    >
      {iconLeft}
      {children}
      {iconRight}
    </Link>
  );
}
