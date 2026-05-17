import { cn } from '@/lib/cn';
import type { LucideIcon } from 'lucide-react';
import type { ReactNode } from 'react';

export function BentoGrid({
  children,
  className,
}: {
  children: ReactNode;
  className?: string;
}) {
  return (
    <div
      className={cn(
        'grid grid-cols-1 gap-4 sm:grid-cols-2 lg:grid-cols-3',
        className,
      )}
    >
      {children}
    </div>
  );
}

export function BentoCard({
  icon: Icon,
  title,
  description,
  className,
  accent = false,
  span = 1,
  visual,
}: {
  icon: LucideIcon;
  title: string;
  description: string;
  className?: string;
  accent?: boolean;
  span?: 1 | 2;
  visual?: ReactNode;
}) {
  return (
    <div
      className={cn(
        'group relative overflow-hidden rounded-2xl border border-fd-border bg-fd-card p-6 transition-all duration-200',
        'hover:-translate-y-0.5 hover:border-fd-primary/40 hover:shadow-xl hover:shadow-fd-primary/5',
        span === 2 && 'sm:col-span-2',
        className,
      )}
    >
      {/* glow halo on hover */}
      <div
        aria-hidden
        className={cn(
          'pointer-events-none absolute -right-16 -top-16 size-40 rounded-full opacity-0 blur-3xl transition-opacity duration-500 group-hover:opacity-70',
          accent ? 'bg-fd-accent/30' : 'bg-fd-primary/30',
        )}
      />
      <div className="relative flex h-full flex-col">
        <div
          className={cn(
            'mb-5 inline-flex size-11 items-center justify-center rounded-xl ring-1',
            accent
              ? 'bg-fd-accent/10 text-fd-accent ring-fd-accent/20'
              : 'bg-fd-primary/10 text-fd-primary ring-fd-primary/20',
          )}
        >
          <Icon className="size-5" strokeWidth={2} />
        </div>
        <h3 className="mb-2 text-base font-semibold tracking-tight text-fd-foreground">
          {title}
        </h3>
        <p className="text-sm leading-relaxed text-fd-muted-foreground">
          {description}
        </p>
        {visual ? <div className="mt-5">{visual}</div> : null}
      </div>
    </div>
  );
}
