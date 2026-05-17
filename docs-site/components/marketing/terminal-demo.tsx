import { cn } from '@/lib/cn';
import { Terminal } from 'lucide-react';

type Line =
  | { kind: 'prompt'; cmd: string }
  | { kind: 'output'; text: string; tone?: 'normal' | 'success' | 'dim' };

const lines: Line[] = [
  { kind: 'prompt', cmd: 'git clone https://github.com/sitharaj88/MicroRTOS' },
  { kind: 'output', text: "Cloning into 'MicroRTOS'…", tone: 'dim' },
  { kind: 'output', text: 'Receiving objects: 100% (412/412), done.', tone: 'dim' },
  { kind: 'prompt', cmd: 'cd MicroRTOS && make flash EXAMPLE=10' },
  { kind: 'output', text: 'Compiling: src/core/mr_list.c', tone: 'dim' },
  { kind: 'output', text: 'Compiling: src/core/mr_task.c', tone: 'dim' },
  { kind: 'output', text: 'Creating library: build/libmicrortos.a', tone: 'dim' },
  { kind: 'output', text: 'Example 10 built successfully!', tone: 'normal' },
  { kind: 'output', text: 'Flashing build/examples/example10.hex to /dev/ttyACM0…' },
  { kind: 'output', text: '3,624 bytes of flash verified', tone: 'success' },
  { kind: 'output', text: 'avrdude done.  Thank you.', tone: 'success' },
  { kind: 'output', text: '', tone: 'dim' },
  { kind: 'output', text: '◉  LED on pin 13 is now blinking at 1 Hz', tone: 'success' },
];

/**
 * Marketing terminal mock. Pure CSS, no JS. The blinking caret on the
 * last line uses a single keyframed div.
 */
export function TerminalDemo({ className }: { className?: string }) {
  return (
    <div
      className={cn(
        'overflow-hidden rounded-2xl border border-fd-border bg-fd-card/90 shadow-2xl shadow-black/20 backdrop-blur dark:shadow-black/50',
        className,
      )}
    >
      <div className="flex items-center justify-between gap-3 border-b border-fd-border bg-fd-muted/50 px-4 py-2.5">
        <div className="flex items-center gap-3">
          <div className="flex gap-1.5">
            <span className="size-3 rounded-full bg-red-400/80" aria-hidden />
            <span className="size-3 rounded-full bg-amber-400/80" aria-hidden />
            <span className="size-3 rounded-full bg-emerald-400/80" aria-hidden />
          </div>
          <span className="flex items-center gap-1.5 font-mono text-xs text-fd-muted-foreground">
            <Terminal className="size-3.5" />
            ~/MicroRTOS · zsh
          </span>
        </div>
      </div>
      <div className="px-5 py-5 font-mono text-[13px] leading-relaxed">
        {lines.map((line, i) =>
          line.kind === 'prompt' ? (
            <div key={i} className="text-fd-foreground">
              <span className="text-fd-primary">$</span>{' '}
              <span>{line.cmd}</span>
            </div>
          ) : (
            <div
              key={i}
              className={cn(
                line.tone === 'success' && 'text-emerald-600 dark:text-emerald-400',
                line.tone === 'dim' && 'text-fd-muted-foreground',
                (!line.tone || line.tone === 'normal') && 'text-fd-foreground',
              )}
            >
              {line.text || ' '}
            </div>
          ),
        )}
        <div className="mt-1 text-fd-foreground">
          <span className="text-fd-primary">$</span>{' '}
          <span
            aria-hidden
            className="inline-block h-4 w-1.5 translate-y-0.5 animate-glow bg-fd-foreground"
          />
        </div>
      </div>
    </div>
  );
}
