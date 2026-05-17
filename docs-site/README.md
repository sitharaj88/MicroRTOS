# MicroRTOS docs site

Next.js 15 + Fumadocs + Tailwind, deploys to GitHub Pages as a static
export.

## Develop

```bash
cd docs-site
pnpm install
pnpm dev      # http://localhost:3000
```

The dev server hot-reloads MDX edits. Layout/theme changes need a
browser refresh.

## Build locally

```bash
NEXT_PUBLIC_BASE_PATH=/MicroRTOS pnpm build
```

Produces `out/` — a self-contained static site. Open `out/index.html`
in a browser, or serve it:

```bash
pnpm exec serve out
```

Without `NEXT_PUBLIC_BASE_PATH`, the build assumes the site lives at the
domain root (suitable for `pnpm dev` and Vercel-style deploys). On
GitHub Pages the site lives at `https://<user>.github.io/<repo>/`, so
the base path must match the repo name.

## Deploy

The `.github/workflows/docs.yml` workflow is manual-only — open the
**Actions** tab on GitHub, pick **Docs site**, and click
**Run workflow** on the `main` branch. The job:

1. Installs deps with pnpm.
2. Builds with `NEXT_PUBLIC_BASE_PATH=/MicroRTOS`.
3. Adds `.nojekyll` so GitHub Pages doesn't drop `_next/` paths.
4. Uploads `out/` as a Pages artifact and deploys.

Repo settings → Pages → Source must be set to **GitHub Actions** for
this to work.

## Content layout

```
content/docs/
├── index.mdx                 # /docs (overview)
├── meta.json                 # top-level nav order
├── getting-started/
│   ├── meta.json
│   ├── index.mdx
│   ├── install.mdx
│   ├── build.mdx
│   ├── flash.mdx
│   └── first-blink.mdx
├── concepts/
│   ├── architecture.mdx
│   ├── scheduler.mdx
│   └── ... (one page per topic)
├── api/                       # function-level reference
├── examples.mdx               # all 11 examples in one page
├── performance.mdx
└── porting.mdx
```

`meta.json` files control sidebar order; entries starting with `---`
become headings.

## Adding a page

1. Drop an MDX file under `content/docs/<section>/`.
2. Add its slug to the section's `meta.json` `pages` array.
3. Front-matter:

```mdx
---
title: My new page
description: One-line summary that shows up in search and OG tags.
icon: Sparkles   # optional, lucide-react icon name
---
```

`pnpm dev` will pick it up automatically.

## Useful components in MDX

```mdx
<Callout type="info">...</Callout>
<Steps><Step>...</Step></Steps>
<Cards><Card title="..." href="..." description="..." /></Cards>
<Tabs><Tab title="...">...</Tab></Tabs>
<TypeTable type={{ field: { type: '...', description: '...' } }} />
```

These are wired through `components/mdx.tsx`.

## Theme

`tailwind.config.ts` defines the brand palette (blue primary, orange
accent). Both light and dark are themed. Override at the CSS layer in
`app/global.css` if you want richer gradients.
