import type { BaseLayoutProps } from 'fumadocs-ui/layouts/shared';
import type { DocsLayoutProps } from 'fumadocs-ui/layouts/docs';
import { source } from './source';
import { Wordmark } from '@/components/ui/logo';

// Shared layout config: nav links, repo URL, brand mark.
export const baseOptions: BaseLayoutProps = {
  nav: {
    title: <Wordmark />,
  },
  links: [
    {
      text: 'Docs',
      url: '/docs/getting-started',
      active: 'nested-url',
    },
    {
      text: 'Tutorials',
      url: '/docs/tutorials',
      active: 'nested-url',
    },
    {
      text: 'API',
      url: '/docs/api',
      active: 'nested-url',
    },
    {
      text: 'Examples',
      url: '/docs/examples',
      active: 'nested-url',
    },
  ],
  githubUrl: 'https://github.com/sitharaj88/MicroRTOS',
};

export const docsOptions: DocsLayoutProps = {
  ...baseOptions,
  tree: source.pageTree,
};
