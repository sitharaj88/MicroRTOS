import { cn } from '@/lib/cn';
import type { ReactNode } from 'react';

export type ErrorRow = {
  code: string;
  description: ReactNode;
};

/**
 * Compact list of possible return-status codes per function.
 */
export function ErrorTable({
  rows,
  className,
}: {
  rows: ErrorRow[];
  className?: string;
}) {
  return (
    <div
      className={cn(
        'not-prose overflow-hidden rounded-xl border border-fd-border bg-fd-card',
        className,
      )}
    >
      <ul className="divide-y divide-fd-border text-sm">
        {rows.map((r) => (
          <li
            key={r.code}
            className="grid grid-cols-1 gap-1 px-4 py-2.5 sm:grid-cols-[180px_1fr] sm:items-baseline sm:gap-4"
          >
            <code className="font-mono text-xs text-fd-foreground">
              {r.code}
            </code>
            <span className="text-fd-muted-foreground">{r.description}</span>
          </li>
        ))}
      </ul>
    </div>
  );
}
