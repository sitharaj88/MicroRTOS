import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';

// Tailwind-aware class merger. Same pattern as shadcn/ui.
export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}
