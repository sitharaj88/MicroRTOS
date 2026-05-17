import { cn } from '@/lib/cn';
import { ChevronDown } from 'lucide-react';

/**
 * Subtle "scroll for more" cue at the bottom of a hero.
 */
export function ScrollHint({ className }: { className?: string }) {
  return (
    <div
      className={cn(
        'flex flex-col items-center gap-1.5 text-fd-muted-foreground/80',
        className,
      )}
      aria-hidden
    >
      <span className="text-[10px] font-semibold uppercase tracking-widest">
        Scroll
      </span>
      <ChevronDown className="size-4 animate-scroll-hint" strokeWidth={2.5} />
    </div>
  );
}
