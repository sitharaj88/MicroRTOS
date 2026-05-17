import { cn } from '@/lib/cn';
import type { ComponentProps } from 'react';

/**
 * Centered max-width wrapper. The site is consistently aligned to a
 * 1200 px column; pages use `<Container>` to stay on it.
 */
export function Container({ className, ...props }: ComponentProps<'div'>) {
  return (
    <div
      className={cn('mx-auto w-full max-w-6xl px-6', className)}
      {...props}
    />
  );
}
