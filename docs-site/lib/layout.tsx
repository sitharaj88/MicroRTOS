import type { BaseLayoutProps } from 'fumadocs-ui/layouts/shared';
import type { DocsLayoutProps } from 'fumadocs-ui/layouts/docs';
import { source } from './source';

// Shared layout config: nav links, repo URL, brand mark.
export const baseOptions: BaseLayoutProps = {
  nav: {
    title: (
      <span className="flex items-center gap-2">
        <span
          aria-hidden
          className="inline-block size-5 rounded-md bg-gradient-to-br from-primary to-accent"
        />
        <span className="font-semibold tracking-tight">MicroRTOS</span>
      </span>
    ),
  },
  links: [
    {
      text: 'Docs',
      url: '/docs/getting-started',
      active: 'nested-url',
    },
    {
      text: 'Examples',
      url: '/docs/examples',
      active: 'nested-url',
    },
    {
      text: 'API',
      url: '/docs/api',
      active: 'nested-url',
    },
  ],
  githubUrl: 'https://github.com/sitharaj88/rtos',
};

export const docsOptions: DocsLayoutProps = {
  ...baseOptions,
  tree: source.pageTree,
};
