import { cn } from '@/lib/cn';

/**
 * MicroRTOS logo mark — gradient rounded square with stylized "M" and
 * a small pulse dot. Drawn inline as SVG so it scales perfectly and
 * doesn't need a network request from the nav.
 */
export function LogoMark({
  className,
  size = 24,
}: {
  className?: string;
  size?: number;
}) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 64 64"
      xmlns="http://www.w3.org/2000/svg"
      aria-hidden
      className={cn('inline-block shrink-0', className)}
    >
      <defs>
        <linearGradient id="mr-mark-grad" x1="0%" y1="0%" x2="100%" y2="100%">
          <stop offset="0%" stopColor="#3B82F6" />
          <stop offset="100%" stopColor="#F97316" />
        </linearGradient>
      </defs>
      <rect width="64" height="64" rx="14" fill="url(#mr-mark-grad)" />
      <path
        d="M 14 48 L 14 18 L 22 18 L 32 32 L 42 18 L 50 18 L 50 48 L 44 48 L 44 28 L 34 41 L 30 41 L 20 28 L 20 48 Z"
        fill="#FFFFFF"
      />
      <circle cx="51" cy="14" r="3.5" fill="#FFFFFF" fillOpacity="0.9" />
    </svg>
  );
}

/**
 * Wordmark — logo + brand text inline. Used in the page nav.
 */
export function Wordmark({ className }: { className?: string }) {
  return (
    <span className={cn('inline-flex items-center gap-2', className)}>
      <LogoMark size={22} />
      <span className="text-base font-semibold tracking-tight">MicroRTOS</span>
    </span>
  );
}
