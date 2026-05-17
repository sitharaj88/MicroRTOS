import { docs, meta } from '@/.source';
import { loader } from 'fumadocs-core/source';
import { createMDXSource } from 'fumadocs-mdx';
import { icons } from 'lucide-react';
import { createElement } from 'react';

// Central content source used by all docs routes. Fumadocs uses this to
// build the sidebar tree, breadcrumb data, and search index from the
// MDX tree under content/docs.
export const source = loader({
  baseUrl: '/docs',
  source: createMDXSource(docs, meta),
  icon: (icon) => {
    if (!icon) return undefined;
    const Lucide = (icons as Record<string, React.ComponentType<{ className?: string }>>)[icon];
    return Lucide ? createElement(Lucide) : undefined;
  },
});
