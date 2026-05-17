import { RootProvider } from 'fumadocs-ui/provider';
import type { Metadata } from 'next';
import type { ReactNode } from 'react';
import './global.css';

const siteUrl = process.env.NEXT_PUBLIC_SITE_URL ?? 'https://sitharaj88.github.io/rtos';

export const metadata: Metadata = {
  metadataBase: new URL(siteUrl),
  title: {
    default: 'MicroRTOS — A modern RTOS for AVR & ARM Cortex-M',
    template: '%s · MicroRTOS',
  },
  description:
    'MicroRTOS is a small, MISRA-aligned real-time operating system for AVR and ARM Cortex-M, with priority + EDF scheduling, mutexes with priority inheritance, queues, software timers, and a tickless mode. Hardware-validated on Arduino Uno.',
  applicationName: 'MicroRTOS',
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
  ],
  openGraph: {
    type: 'website',
    title: 'MicroRTOS',
    description:
      'A small, MISRA-aligned real-time operating system for AVR and ARM Cortex-M.',
    url: siteUrl,
    siteName: 'MicroRTOS',
  },
  twitter: {
    card: 'summary_large_image',
    title: 'MicroRTOS',
    description: 'A small, MISRA-aligned RTOS for AVR and ARM Cortex-M.',
  },
};

export default function RootLayout({ children }: { children: ReactNode }) {
  return (
    <html lang="en" suppressHydrationWarning>
      <body className="flex min-h-screen flex-col">
        <RootProvider>{children}</RootProvider>
      </body>
    </html>
  );
}
