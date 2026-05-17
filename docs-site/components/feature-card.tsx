import { cn } from '@/lib/cn';
import type { LucideIcon } from 'lucide-react';

export function FeatureCard({
  icon: Icon,
  title,
  description,
  accent = false,
}: {
  icon: LucideIcon;
  title: string;
  description: string;
  accent?: boolean;
}) {
  return (
    <div
      className={cn(
        'group relative overflow-hidden rounded-xl border border-fd-border bg-fd-card p-6 transition',
        'hover:border-fd-primary/40 hover:shadow-lg hover:shadow-fd-primary/5',
      )}
    >
      <div
        aria-hidden
        className={cn(
          'absolute -right-12 -top-12 size-32 rounded-full blur-3xl transition-opacity',
          'opacity-0 group-hover:opacity-60',
          accent ? 'bg-fd-accent/30' : 'bg-fd-primary/30',
        )}
      />
      <div className="relative">
        <div
          className={cn(
            'mb-4 inline-flex size-10 items-center justify-center rounded-lg',
            accent
              ? 'bg-fd-accent/10 text-fd-accent'
              : 'bg-fd-primary/10 text-fd-primary',
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
      </div>
    </div>
  );
}
