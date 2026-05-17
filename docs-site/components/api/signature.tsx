import { cn } from '@/lib/cn';
import { Code2 } from 'lucide-react';
import type { ReactNode } from 'react';
import { Badge, type BadgeVariant } from '@/components/ui/badge';

export type ApiBadge = {
  label: string;
  variant?: BadgeVariant;
};

/**
 * Header block for a single API function: name, signature in a code
 * frame, and a row of badges (ISR-safe? blocking? since-version).
 */
export function Signature({
  name,
  signature,
  badges,
  className,
}: {
  name: string;
  signature: string;
  badges?: ApiBadge[];
  className?: string;
}) {
  return (
    <div
      className={cn(
        'not-prose my-6 overflow-hidden rounded-xl border border-fd-border bg-fd-card',
        className,
      )}
    >
      <div className="flex flex-wrap items-center justify-between gap-3 border-b border-fd-border bg-fd-muted/40 px-4 py-2.5">
        <div className="flex items-center gap-2 text-sm font-semibold text-fd-foreground">
          <Code2 className="size-4 text-fd-primary" />
          <code className="font-mono">{name}</code>
        </div>
        {badges && badges.length > 0 ? (
          <div className="flex flex-wrap gap-1.5">
            {badges.map((b) => (
              <Badge key={b.label} variant={b.variant ?? 'neutral'}>
                {b.label}
              </Badge>
            ))}
          </div>
        ) : null}
      </div>
      <pre className="overflow-x-auto px-4 py-3 text-sm leading-relaxed">
        <code className="font-mono text-fd-foreground">{signature}</code>
      </pre>
    </div>
  );
}

/**
 * Section wrapper used under a Signature to introduce parameters,
 * returns, etc. Keeps the typography consistent.
 */
export function ApiSection({
  title,
  children,
  className,
}: {
  title: ReactNode;
  children: ReactNode;
  className?: string;
}) {
  return (
    <div className={cn('not-prose my-5', className)}>
      <div className="mb-2 text-xs font-bold uppercase tracking-widest text-fd-muted-foreground">
        {title}
      </div>
      <div className="text-sm text-fd-foreground/90">{children}</div>
    </div>
  );
}
