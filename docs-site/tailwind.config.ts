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
        float: {
          '0%, 100%': { transform: 'translateY(0)' },
          '50%': { transform: 'translateY(-8px)' },
        },
        'float-slow': {
          '0%, 100%': { transform: 'translateY(0) translateX(0)' },
          '33%': { transform: 'translateY(-12px) translateX(6px)' },
          '66%': { transform: 'translateY(8px) translateX(-4px)' },
        },
        'orb-1': {
          '0%, 100%': { transform: 'translate(0, 0) scale(1)' },
          '50%': { transform: 'translate(40px, -30px) scale(1.05)' },
        },
        'orb-2': {
          '0%, 100%': { transform: 'translate(0, 0) scale(1)' },
          '50%': { transform: 'translate(-30px, 40px) scale(0.95)' },
        },
        'beam': {
          '0%': { transform: 'translateY(-100%) rotate(0deg)' },
          '100%': { transform: 'translateY(100vh) rotate(0deg)' },
        },
        'pulse-soft': {
          '0%, 100%': { opacity: '0.4' },
          '50%': { opacity: '0.8' },
        },
        'bar-grow': {
          '0%': { transform: 'scaleX(0)', transformOrigin: 'left' },
          '100%': { transform: 'scaleX(1)', transformOrigin: 'left' },
        },
        'gradient-flow': {
          '0%, 100%': { backgroundPosition: '0% 50%' },
          '50%': { backgroundPosition: '100% 50%' },
        },
        'scroll-hint': {
          '0%, 100%': { transform: 'translateY(0)', opacity: '0.5' },
          '50%': { transform: 'translateY(6px)', opacity: '1' },
        },
        'chip-float-a': {
          '0%, 100%': { transform: 'translate(0, 0) rotate(-2deg)' },
          '50%': { transform: 'translate(-4px, -8px) rotate(-2deg)' },
        },
        'chip-float-b': {
          '0%, 100%': { transform: 'translate(0, 0) rotate(1deg)' },
          '50%': { transform: 'translate(6px, -6px) rotate(1deg)' },
        },
        'chip-float-c': {
          '0%, 100%': { transform: 'translate(0, 0) rotate(-1deg)' },
          '50%': { transform: 'translate(-6px, 5px) rotate(-1deg)' },
        },
        'led-pulse': {
          '0%, 100%': { opacity: '0.5', transform: 'scale(0.95)' },
          '50%': { opacity: '1', transform: 'scale(1.05)' },
        },
        'wire-flow': {
          '0%': { 'stroke-dashoffset': '100' },
          '100%': { 'stroke-dashoffset': '0' },
        },
      },
      animation: {
        shimmer: 'shimmer 2.5s linear infinite',
        'fade-in-up': 'fade-in-up 0.6s ease-out',
        glow: 'glow 3s ease-in-out infinite',
        float: 'float 4s ease-in-out infinite',
        'float-slow': 'float-slow 9s ease-in-out infinite',
        'orb-1': 'orb-1 14s ease-in-out infinite',
        'orb-2': 'orb-2 18s ease-in-out infinite',
        'pulse-soft': 'pulse-soft 4s ease-in-out infinite',
        'bar-grow': 'bar-grow 1s ease-out forwards',
        'gradient-flow': 'gradient-flow 6s ease-in-out infinite',
        'scroll-hint': 'scroll-hint 1.8s ease-in-out infinite',
        'chip-float-a': 'chip-float-a 7s ease-in-out infinite',
        'chip-float-b': 'chip-float-b 6s ease-in-out infinite',
        'chip-float-c': 'chip-float-c 8s ease-in-out infinite',
        'led-pulse': 'led-pulse 1.5s ease-in-out infinite',
        'wire-flow': 'wire-flow 2.5s ease-in-out infinite',
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
