import { cn } from '@/lib/cn';
import {
  CheckCircle2,
  Cpu,
  Github,
  Scale,
  ShieldCheck,
  TestTube2,
} from 'lucide-react';
import type { LucideIcon } from 'lucide-react';

type Item = {
  icon: LucideIcon;
  label: string;
  value: string;
};

const items: Item[] = [
  { icon: ShieldCheck, label: 'Validated', value: 'Arduino Uno · LIVE' },
  { icon: TestTube2,   label: 'Host tests', value: '79/79 passing' },
  { icon: CheckCircle2, label: 'MISRA',    value: 'C:2012 aligned' },
  { icon: Cpu,         label: 'Targets',   value: 'AVR + Cortex-M' },
  { icon: Scale,       label: 'License',   value: 'MIT' },
];

/**
 * Trust row shown in the hero — small icon-led badges that communicate
 * the social/safety proof in one glance. Replaces a single long pill.
 */
export function TrustRow({ className }: { className?: string }) {
  return (
    <ul
      className={cn(
        'flex flex-wrap items-center justify-center gap-x-5 gap-y-3 text-xs',
        className,
      )}
    >
      {items.map((it) => (
        <li
          key={it.label}
          className="group flex items-center gap-2 text-fd-muted-foreground transition hover:text-fd-foreground"
        >
          <span
            className="inline-flex size-7 items-center justify-center rounded-lg bg-fd-card text-fd-primary ring-1 ring-fd-border transition group-hover:ring-fd-primary/30"
            aria-hidden
          >
            <it.icon className="size-3.5" strokeWidth={2.2} />
          </span>
          <span className="flex items-baseline gap-1">
            <span className="font-semibold text-fd-foreground">{it.value}</span>
            <span className="text-[10px] uppercase tracking-wider text-fd-muted-foreground">
              · {it.label}
            </span>
          </span>
        </li>
      ))}
    </ul>
  );
}
