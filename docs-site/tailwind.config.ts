import type { Config } from 'tailwindcss';
import { createPreset } from 'fumadocs-ui/tailwind-plugin';

// MicroRTOS design system — cobalt primary with warm amber accent,
// strong typographic scale, and a few custom utilities (mesh, grid,
// shimmer) used by the landing surfaces.
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
          primary: '221 83% 53%',
          'primary-foreground': '0 0% 100%',
          secondary: '210 40% 96%',
          'secondary-foreground': '222 47% 11%',
          accent: '32 95% 53%',
          'accent-foreground': '0 0% 100%',
          ring: '221 83% 53%',
        },
        dark: {
          background: '224 71% 4%',
          foreground: '210 40% 98%',
          muted: '215 28% 14%',
          'muted-foreground': '217 11% 65%',
          popover: '224 71% 6%',
          'popover-foreground': '210 40% 98%',
          card: '224 71% 6%',
          'card-foreground': '210 40% 98%',
          border: '215 28% 17%',
          primary: '217 91% 60%',
          'primary-foreground': '0 0% 100%',
          secondary: '215 28% 14%',
          'secondary-foreground': '210 40% 98%',
          accent: '32 95% 58%',
          'accent-foreground': '0 0% 100%',
          ring: '217 91% 60%',
        },
      },
    }),
  ],
  theme: {
    extend: {
      fontFamily: {
        sans: [
          '"Inter Variable"',
          'system-ui',
          '-apple-system',
          'BlinkMacSystemFont',
          'sans-serif',
        ],
        mono: [
          '"JetBrains Mono Variable"',
          'ui-monospace',
          'SFMono-Regular',
          'Menlo',
          'Consolas',
          'monospace',
        ],
        display: [
          '"Inter Variable"',
          'system-ui',
          'sans-serif',
        ],
      },
      letterSpacing: {
        tightest: '-0.04em',
      },
      keyframes: {
        shimmer: {
          '0%': { transform: 'translateX(-100%)' },
          '100%': { transform: 'translateX(100%)' },
        },
        'fade-in-up': {
          '0%': { opacity: '0', transform: 'translateY(8px)' },
          '100%': { opacity: '1', transform: 'translateY(0)' },
        },
        glow: {
          '0%, 100%': { opacity: '0.6' },
          '50%': { opacity: '1' },
        },
      },
      animation: {
        shimmer: 'shimmer 2.5s linear infinite',
        'fade-in-up': 'fade-in-up 0.6s ease-out',
        glow: 'glow 3s ease-in-out infinite',
      },
      backgroundImage: {
        'mesh-light': `
          radial-gradient(at 27% 37%, hsl(221 83% 53% / 0.15) 0px, transparent 50%),
          radial-gradient(at 97% 21%, hsl(32 95% 53% / 0.10) 0px, transparent 50%),
          radial-gradient(at 52% 99%, hsl(196 84% 56% / 0.12) 0px, transparent 50%),
          radial-gradient(at 10% 29%, hsl(280 71% 64% / 0.08) 0px, transparent 50%),
          radial-gradient(at 97% 96%, hsl(210 100% 60% / 0.08) 0px, transparent 50%)
        `,
        'mesh-dark': `
          radial-gradient(at 27% 37%, hsl(221 83% 53% / 0.30) 0px, transparent 50%),
          radial-gradient(at 97% 21%, hsl(32 95% 53% / 0.20) 0px, transparent 50%),
          radial-gradient(at 52% 99%, hsl(196 84% 56% / 0.22) 0px, transparent 50%),
          radial-gradient(at 10% 29%, hsl(280 71% 64% / 0.18) 0px, transparent 50%),
          radial-gradient(at 97% 96%, hsl(210 100% 60% / 0.20) 0px, transparent 50%)
        `,
      },
    },
  },
};

export default config;
