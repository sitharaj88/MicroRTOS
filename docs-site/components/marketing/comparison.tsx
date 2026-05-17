import { cn } from '@/lib/cn';
import { Check, Minus, X } from 'lucide-react';

type Cell = 'yes' | 'no' | 'partial' | string;

type Row = {
  feature: string;
  micrortos: Cell;
  freertos: Cell;
  zephyr: Cell;
};

const rows: Row[] = [
  { feature: 'Flash footprint (minimal)', micrortos: '~3.5 KB', freertos: '~5–8 KB', zephyr: '~10 KB+' },
  { feature: 'SRAM footprint (minimal)', micrortos: '~250 B', freertos: '~250 B', zephyr: '~500 B' },
  { feature: 'Audit surface', micrortos: '~5 kLOC', freertos: '~20 kLOC', zephyr: '~500 kLOC' },
  { feature: 'AVR support', micrortos: 'yes', freertos: 'community', zephyr: 'no' },
  { feature: 'ARM Cortex-M', micrortos: 'yes', freertos: 'yes', zephyr: 'yes' },
  { feature: 'EDF scheduler', micrortos: 'yes', freertos: 'no', zephyr: 'no' },
  { feature: 'Priority inheritance', micrortos: 'yes', freertos: 'yes', zephyr: 'yes' },
  { feature: 'Tickless idle', micrortos: 'yes', freertos: 'yes', zephyr: 'yes' },
  { feature: 'Zero-copy IPC', micrortos: 'yes', freertos: 'no', zephyr: 'partial' },
  { feature: 'Single public header', micrortos: 'yes', freertos: 'no', zephyr: 'no' },
  { feature: 'No malloc anywhere', micrortos: 'yes', freertos: 'partial', zephyr: 'no' },
  { feature: 'Build system', micrortos: 'Makefile', freertos: 'Makefile/CMake', zephyr: 'CMake + west' },
  { feature: 'Hardware-validated on Uno', micrortos: 'yes', freertos: 'community', zephyr: 'no' },
];

function CellGlyph({ value }: { value: Cell }) {
  if (value === 'yes') {
    return (
      <span className="inline-flex items-center gap-1.5 text-emerald-600 dark:text-emerald-400">
        <Check className="size-4" strokeWidth={2.5} />
        Yes
      </span>
    );
  }
  if (value === 'no') {
    return (
      <span className="inline-flex items-center gap-1.5 text-fd-muted-foreground">
        <X className="size-4" strokeWidth={2.5} />
        No
      </span>
    );
  }
  if (value === 'partial') {
    return (
      <span className="inline-flex items-center gap-1.5 text-amber-600 dark:text-amber-400">
        <Minus className="size-4" strokeWidth={2.5} />
        Partial
      </span>
    );
  }
  if (value === 'community') {
    return (
      <span className="inline-flex items-center gap-1.5 text-amber-600 dark:text-amber-400">
        <Minus className="size-4" strokeWidth={2.5} />
        Community
      </span>
    );
  }
  return <span className="font-mono text-fd-foreground">{value}</span>;
}

export function ComparisonTable({ className }: { className?: string }) {
  return (
    <div
      className={cn(
        'overflow-hidden rounded-2xl border border-fd-border bg-fd-card',
        className,
      )}
    >
      <div className="grid grid-cols-4 gap-x-4 border-b border-fd-border bg-fd-muted/50 px-5 py-3 text-xs font-semibold uppercase tracking-wider text-fd-muted-foreground">
        <div className="col-span-1">Feature</div>
        <div className="col-span-1 text-fd-primary">MicroRTOS</div>
        <div className="col-span-1">FreeRTOS</div>
        <div className="col-span-1">Zephyr</div>
      </div>
      <ul className="divide-y divide-fd-border">
        {rows.map((r) => (
          <li
            key={r.feature}
            className="grid grid-cols-4 gap-x-4 px-5 py-3 text-sm"
          >
            <div className="text-fd-foreground">{r.feature}</div>
            <div>
              <CellGlyph value={r.micrortos} />
            </div>
            <div className="text-fd-muted-foreground">
              <CellGlyph value={r.freertos} />
            </div>
            <div className="text-fd-muted-foreground">
              <CellGlyph value={r.zephyr} />
            </div>
          </li>
        ))}
      </ul>
    </div>
  );
}
