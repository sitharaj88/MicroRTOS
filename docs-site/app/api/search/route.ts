import { source } from '@/lib/source';
import { createFromSource } from 'fumadocs-core/search/server';

// Static search index for the docs site. We ship as a static export
// (output: 'export') so the route is pre-rendered at build time —
// `staticGET` returns the full Orama index as JSON, which the client
// downloads once via the static search client in app/layout.tsx.
export const revalidate = false;
export const dynamic = 'force-static';

export const { staticGET: GET } = createFromSource(source);
