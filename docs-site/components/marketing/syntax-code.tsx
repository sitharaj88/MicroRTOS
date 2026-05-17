import { cn } from '@/lib/cn';
import { Terminal } from 'lucide-react';
import type { ReactNode } from 'react';

/**
 * Lightweight, server-rendered "syntax-highlighted" code card.
 *
 * We don't run Shiki here (kept off marketing surfaces to avoid bundle
 * cost). Instead, we accept pre-tokenized children — caller assembles
 * the content with our exported color helpers.
 */
export function SyntaxCode({
  title,
  language,
  children,
  className,
  showLineNumbers = true,
  variant = 'editor',
  badge,
}: {
  title?: string;
  language?: string;
  children: ReactNode;
  className?: string;
  showLineNumbers?: boolean;
  variant?: 'editor' | 'terminal';
  badge?: string;
}) {
  // `children` is expected to be the entire code body as a string or
  // pre-tokenized JSX. For line numbers we wrap each line if it's a
  // plain string.
  const renderBody = () => {
    if (typeof children !== 'string') return children;
    const lines = children.split('\n');
    return lines.map((line, i) => (
      <div key={i} className="flex">
        {showLineNumbers ? (
          <span className="select-none pr-3 text-right text-[10px] text-fd-muted-foreground/40 sm:pr-6 sm:text-xs">
            {String(i + 1).padStart(2, ' ')}
          </span>
        ) : null}
        <span className="flex-1 whitespace-pre">{line || ' '}</span>
      </div>
    ));
  };

  return (
    <div
      className={cn(
        // min-w-0 lets the card shrink inside flex/grid columns without
        // its overflow-x-auto child stretching the parent. Crucial for
        // mobile widths where the code lines are wider than the screen.
        'group/code relative min-w-0 overflow-hidden rounded-2xl border border-fd-border bg-fd-card/95 shadow-2xl shadow-black/10 backdrop-blur dark:shadow-black/40',
        className,
      )}
    >
      {/* Gradient hairline top */}
      <div
        aria-hidden
        className="pointer-events-none absolute inset-x-0 top-0 h-px bg-gradient-to-r from-transparent via-fd-primary/50 to-transparent"
      />
      <div className="flex items-center justify-between gap-3 border-b border-fd-border bg-fd-muted/40 px-3 py-2 sm:px-4 sm:py-2.5">
        <div className="flex min-w-0 items-center gap-2 sm:gap-3">
          <div className="flex gap-1 sm:gap-1.5">
            <span className="size-2.5 rounded-full bg-red-400/80 sm:size-3" aria-hidden />
            <span className="size-2.5 rounded-full bg-amber-400/80 sm:size-3" aria-hidden />
            <span className="size-2.5 rounded-full bg-emerald-400/80 sm:size-3" aria-hidden />
          </div>
          {title ? (
            <span className="flex min-w-0 items-center gap-1.5 truncate font-mono text-[10px] text-fd-muted-foreground sm:text-xs">
              {variant === 'terminal' ? <Terminal className="size-3 shrink-0 sm:size-3.5" /> : null}
              <span className="truncate">{title}</span>
            </span>
          ) : null}
        </div>
        <div className="flex shrink-0 items-center gap-2">
          {badge ? (
            <span className="hidden rounded-md border border-fd-border bg-fd-card px-2 py-0.5 font-mono text-[10px] uppercase tracking-wider text-fd-muted-foreground sm:inline">
              {badge}
            </span>
          ) : null}
          {language ? (
            <span className="font-mono text-[10px] uppercase tracking-wider text-fd-muted-foreground">
              {language}
            </span>
          ) : null}
        </div>
      </div>
      <pre className="overflow-x-auto px-3 py-4 text-[11px] leading-relaxed sm:px-5 sm:py-5 sm:text-[13px]">
        <code className="font-mono text-fd-foreground">{renderBody()}</code>
      </pre>
    </div>
  );
}

/* Tiny color helpers (used inline in `children`) */
export const tok = {
  keyword: 'text-violet-500 dark:text-violet-400',
  type: 'text-cyan-600 dark:text-cyan-400',
  string: 'text-emerald-600 dark:text-emerald-400',
  number: 'text-amber-600 dark:text-amber-400',
  fn: 'text-fd-primary',
  comment: 'text-fd-muted-foreground italic',
  ident: 'text-fd-foreground',
  punct: 'text-fd-muted-foreground',
} as const;
