import { cn } from '@/lib/cn';

/**
 * Lightweight, self-contained code block for marketing surfaces.
 * Avoids the heavy MDX pipeline; this is purely visual.
 */
export function CodeBlock({
  language,
  children,
  className,
}: {
  language?: string;
  children: string;
  className?: string;
}) {
  return (
    <div
      className={cn(
        'overflow-hidden rounded-xl border border-fd-border bg-fd-card shadow-lg shadow-black/5 dark:shadow-black/20',
        className,
      )}
    >
      {language ? (
        <div className="flex items-center justify-between border-b border-fd-border px-4 py-2 text-xs text-fd-muted-foreground">
          <span className="font-mono">{language}</span>
          <div className="flex gap-1.5">
            <span className="size-2.5 rounded-full bg-red-400/70" aria-hidden />
            <span className="size-2.5 rounded-full bg-yellow-400/70" aria-hidden />
            <span className="size-2.5 rounded-full bg-green-400/70" aria-hidden />
          </div>
        </div>
      ) : null}
      <pre className="overflow-x-auto px-4 py-4 text-sm leading-relaxed">
        <code className="font-mono text-fd-foreground">{children}</code>
      </pre>
    </div>
  );
}
