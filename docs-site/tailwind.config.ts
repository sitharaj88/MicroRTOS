import type { Config } from 'tailwindcss';
import { createPreset } from 'fumadocs-ui/tailwind-plugin';

// MicroRTOS brand palette: deep blue primary, warm orange accent.
// Fumadocs maps these HSL tuples to its theme tokens.
const config: Config = {
  content: [
    './app/**/*.{ts,tsx}',
    './components/**/*.{ts,tsx}',
    './content/**/*.{md,mdx}',
    './node_modules/fumadocs-ui/dist/**/*.js',
  ],
  presets: [
    createPreset({
      preset: {
        light: {
          background: '0 0% 100%',
          foreground: '222 47% 11%',
          muted: '210 40% 96%',
          'muted-foreground': '215 16% 47%',
          popover: '0 0% 100%',
          'popover-foreground': '222 47% 11%',
          card: '0 0% 100%',
          'card-foreground': '222 47% 11%',
          border: '214 32% 91%',
          primary: '217 91% 50%',
          'primary-foreground': '0 0% 100%',
          secondary: '210 40% 96%',
          'secondary-foreground': '222 47% 11%',
          accent: '24 95% 53%',
          'accent-foreground': '0 0% 100%',
          ring: '217 91% 50%',
        },
        dark: {
          background: '222 47% 6%',
          foreground: '210 40% 98%',
          muted: '217 33% 17%',
          'muted-foreground': '215 20% 65%',
          popover: '222 47% 8%',
          'popover-foreground': '210 40% 98%',
          card: '222 47% 8%',
          'card-foreground': '210 40% 98%',
          border: '217 33% 17%',
          primary: '217 91% 60%',
          'primary-foreground': '0 0% 100%',
          secondary: '217 33% 17%',
          'secondary-foreground': '210 40% 98%',
          accent: '24 95% 58%',
          'accent-foreground': '0 0% 100%',
          ring: '217 91% 60%',
        },
      },
    }),
  ],
  theme: {
    extend: {
      fontFamily: {
        mono: ['ui-monospace', 'SFMono-Regular', 'Menlo', 'Consolas', 'monospace'],
      },
    },
  },
};

export default config;
