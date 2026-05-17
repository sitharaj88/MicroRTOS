import { RootProvider } from 'fumadocs-ui/provider';
import type { Metadata, Viewport } from 'next';
import type { ReactNode } from 'react';
import './global.css';

// Project is hosted at https://sitharaj88.github.io/MicroRTOS/.
const siteUrl =
  process.env.NEXT_PUBLIC_SITE_URL ?? 'https://sitharaj88.github.io/MicroRTOS';

export const metadata: Metadata = {
  metadataBase: new URL(siteUrl),
  title: {
    default: 'MicroRTOS — A real-time kernel you can read in an afternoon',
    template: '%s · MicroRTOS',
  },
  description:
    'MicroRTOS is a small, MISRA-aligned real-time operating system for AVR and ARM Cortex-M — priority + EDF scheduling, mutexes with priority inheritance, queues, software timers, and a tickless mode. Hardware-validated on Arduino Uno.',
  applicationName: 'MicroRTOS',
  authors: [
    { name: 'Sitharaj Seenivasan', url: 'https://sitharaj.in' },
  ],
  creator: 'Sitharaj Seenivasan',
  publisher: 'Sitharaj Seenivasan',
  keywords: [
    'RTOS',
    'real-time operating system',
    'AVR',
    'ATmega328P',
    'ARM Cortex-M',
    'Arduino',
    'embedded',
    'MISRA',
    'EDF scheduler',
    'priority inheritance',
    'MicroRTOS',
    'Sitharaj Seenivasan',
  ],
  openGraph: {
    type: 'website',
    title: 'MicroRTOS',
    description:
      'A small, MISRA-aligned real-time operating system for AVR and ARM Cortex-M.',
    url: siteUrl,
    siteName: 'MicroRTOS',
    locale: 'en_US',
  },
  twitter: {
    card: 'summary_large_image',
    title: 'MicroRTOS',
    description:
      'A small, MISRA-aligned RTOS for AVR and ARM Cortex-M. By Sitharaj Seenivasan.',
  },
};

// Next.js picks up app/icon.svg automatically as the favicon.
// Theme-color goes in the Viewport export per Next.js 15 convention.
export const viewport: Viewport = {
  themeColor: [
    { media: '(prefers-color-scheme: light)', color: '#FFFFFF' },
    { media: '(prefers-color-scheme: dark)', color: '#0F172A' },
  ],
};

// Static search: the index lives at `${basePath}/api/search` because we
// build with output: 'export' and a project basePath on GitHub Pages.
// The client fetches a plain relative URL, so we must include basePath.
const basePath = process.env.NEXT_PUBLIC_BASE_PATH ?? '';

export default function RootLayout({ children }: { children: ReactNode }) {
  return (
    <html lang="en" suppressHydrationWarning>
      <body className="flex min-h-screen flex-col">
        <RootProvider
          search={{
            options: {
              type: 'static',
              // Next.js export writes the static search index as a single
              // file at out/api/search (no trailing slash, no directory) —
              // that's the URL GitHub Pages serves. We must include the
              // GitHub Pages basePath here since fetch won't auto-prefix.
              api: `${basePath}/api/search`,
            },
          }}
        >
          {children}
        </RootProvider>
      </body>
    </html>
  );
}
