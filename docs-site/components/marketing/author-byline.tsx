import Link from 'next/link';
import { cn } from '@/lib/cn';
import {
  Coffee,
  Github,
  Globe,
  Linkedin,
} from 'lucide-react';
import type { LucideIcon } from 'lucide-react';

type Social = {
  label: string;
  href: string;
  icon: LucideIcon;
  external?: boolean;
};

// Canonical profile links — kept here as a single source of truth.
export const author = {
  name: 'Sitharaj Seenivasan',
  short: 'Sitharaj',
  site: 'https://sitharaj.in',
  github: 'https://github.com/sitharaj88',
  linkedin: 'https://www.linkedin.com/in/sitharaj08',
  buymeacoffee: 'https://www.buymeacoffee.com/sitharaj88',
} as const;

const socials: Social[] = [
  { label: 'GitHub', href: author.github, icon: Github, external: true },
  { label: 'LinkedIn', href: author.linkedin, icon: Linkedin, external: true },
  { label: 'Website', href: author.site, icon: Globe, external: true },
  { label: 'Buy me a coffee', href: author.buymeacoffee, icon: Coffee, external: true },
];

/**
 * Inline byline with author name + social icons. Used in the page
 * hero ("Made by Sitharaj") and in the footer.
 */
export function AuthorByline({
  className,
  variant = 'compact',
}: {
  className?: string;
  variant?: 'compact' | 'full' | 'icons';
}) {
  if (variant === 'icons') {
    // Bare icon row — used in the footer brand cluster.
    return (
      <ul className={cn('flex items-center gap-1.5', className)}>
        {socials.map((s) => (
          <li key={s.label}>
            <Link
              href={s.href}
              target="_blank"
              rel="noreferrer noopener"
              aria-label={s.label}
              className="inline-flex size-9 items-center justify-center rounded-lg border border-fd-border bg-fd-card text-fd-muted-foreground transition hover:-translate-y-0.5 hover:border-fd-primary/40 hover:text-fd-primary"
            >
              <s.icon className="size-4" strokeWidth={2.2} />
            </Link>
          </li>
        ))}
      </ul>
    );
  }
  if (variant === 'compact') {
    return (
      <div
        className={cn(
          'inline-flex flex-wrap items-center justify-center gap-2 text-xs text-fd-muted-foreground',
          className,
        )}
      >
        <span>Crafted by</span>
        <Link
          href={author.site}
          target="_blank"
          rel="noreferrer noopener"
          className="font-semibold text-fd-foreground hover:text-fd-primary"
        >
          {author.name}
        </Link>
        <span aria-hidden className="text-fd-muted-foreground/40">·</span>
        <ul className="inline-flex items-center gap-2">
          {socials.map((s) => (
            <li key={s.label}>
              <Link
                href={s.href}
                target="_blank"
                rel="noreferrer noopener"
                aria-label={s.label}
                className="inline-flex size-7 items-center justify-center rounded-md text-fd-muted-foreground transition hover:bg-fd-muted hover:text-fd-foreground"
              >
                <s.icon className="size-3.5" strokeWidth={2.2} />
              </Link>
            </li>
          ))}
        </ul>
      </div>
    );
  }
  // Full variant — icon cards with labels, for the footer.
  return (
    <ul
      className={cn(
        'grid grid-cols-2 gap-2 text-sm sm:grid-cols-4',
        className,
      )}
    >
      {socials.map((s) => (
        <li key={s.label}>
          <Link
            href={s.href}
            target="_blank"
            rel="noreferrer noopener"
            className="group flex items-center gap-2 rounded-lg border border-fd-border bg-fd-card px-3 py-2 text-fd-foreground transition hover:border-fd-primary/40 hover:text-fd-primary"
          >
            <s.icon
              className="size-4 text-fd-muted-foreground transition group-hover:text-fd-primary"
              strokeWidth={2.2}
            />
            <span className="font-medium">{s.label}</span>
          </Link>
        </li>
      ))}
    </ul>
  );
}
