import { cn } from '@/lib/cn';
import { Terminal } from 'lucide-react';
import type { ReactNode } from 'react';

/**
 * Heavier code-card with title bar, optional terminal icon, and a small
 * traffic-light row. Used on marketing surfaces. For docs-content code
 * blocks, Fumadocs' default rehype-pretty-code path is used instead.
 */
export function CodeCard({
  title,
  language,
  children,
  className,
  variant = 'editor',
}: {
  title?: string;
  language?: string;
  children: ReactNode;
  className?: string;
  variant?: 'editor' | 'terminal';
}) {
  return (
    <div
      className={cn(
        'group relative overflow-hidden rounded-2xl border border-fd-border bg-fd-card/80 shadow-2xl shadow-black/10 backdrop-blur dark:shadow-black/40',
        className,
      )}
    >
      <div className="flex items-center justify-between gap-3 border-b border-fd-border bg-fd-muted/40 px-4 py-2.5">
        <div className="flex items-center gap-3">
          <div className="flex gap-1.5">
            <span className="size-3 rounded-full bg-red-400/80" aria-hidden />
            <span className="size-3 rounded-full bg-amber-400/80" aria-hidden />
            <span className="size-3 rounded-full bg-emerald-400/80" aria-hidden />
          </div>
          {title ? (
            <span className="flex items-center gap-1.5 font-mono text-xs text-fd-muted-foreground">
              {variant === 'terminal' ? (
                <Terminal className="size-3.5" />
              ) : null}
              {title}
            </span>
          ) : null}
        </div>
        {language ? (
          <span className="font-mono text-[10px] uppercase tracking-wider text-fd-muted-foreground">
            {language}
          </span>
        ) : null}
      </div>
      <pre className="overflow-x-auto px-5 py-5 text-sm leading-relaxed">
        <code className="font-mono text-fd-foreground">{children}</code>
      </pre>
    </div>
  );
}
