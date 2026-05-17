import { cn } from '@/lib/cn';
import type { ReactNode } from 'react';

export type Param = {
  name: string;
  type: string;
  description: ReactNode;
  optional?: boolean;
  defaultValue?: string;
};

/**
 * Renders a parameter list as a compact, designed table.
 * Designed to render well on small screens (stacks via CSS grid).
 */
export function ParamTable({
  params,
  className,
}: {
  params: Param[];
  className?: string;
}) {
  if (params.length === 0) return null;
  return (
    <div
      className={cn(
        'not-prose overflow-hidden rounded-xl border border-fd-border bg-fd-card',
        className,
      )}
    >
      <div className="hidden grid-cols-[minmax(0,1fr)_minmax(0,1.2fr)_minmax(0,2fr)] gap-x-4 border-b border-fd-border bg-fd-muted/40 px-4 py-2 text-[10px] font-bold uppercase tracking-widest text-fd-muted-foreground md:grid">
        <div>Name</div>
        <div>Type</div>
        <div>Description</div>
      </div>
      <ul className="divide-y divide-fd-border text-sm">
        {params.map((p) => (
          <li
            key={p.name}
            className="grid grid-cols-1 gap-x-4 gap-y-1 px-4 py-3 md:grid-cols-[minmax(0,1fr)_minmax(0,1.2fr)_minmax(0,2fr)] md:items-baseline"
          >
            <div className="font-mono text-fd-foreground">
              {p.name}
              {p.optional ? (
                <span className="ml-1 text-xs text-fd-muted-foreground">
                  ?
                </span>
              ) : null}
            </div>
            <div className="font-mono text-xs text-fd-primary">
              {p.type}
              {p.defaultValue ? (
                <span className="ml-1 text-fd-muted-foreground">
                  = {p.defaultValue}
                </span>
              ) : null}
            </div>
            <div className="text-fd-muted-foreground">{p.description}</div>
          </li>
        ))}
      </ul>
    </div>
  );
}

/**
 * Single value display, e.g. for return types.
 */
export function Returns({
  type,
  description,
  className,
}: {
  type: string;
  description?: ReactNode;
  className?: string;
}) {
  return (
    <div
      className={cn(
        'not-prose flex flex-col gap-1 rounded-xl border border-fd-border bg-fd-card px-4 py-3 text-sm sm:flex-row sm:items-baseline sm:gap-4',
        className,
      )}
    >
      <span className="font-mono text-fd-primary">{type}</span>
      {description ? (
        <span className="text-fd-muted-foreground">{description}</span>
      ) : null}
    </div>
  );
}
