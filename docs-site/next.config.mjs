import { createMDX } from 'fumadocs-mdx/next';

const withMDX = createMDX();

// MicroRTOS docs site — static export for GitHub Pages.
//
// On GitHub Pages a project site lives at https://<user>.github.io/<repo>/.
// Set NEXT_PUBLIC_BASE_PATH=/MicroRTOS when building for production;
// locally we run at the root.
const basePath = process.env.NEXT_PUBLIC_BASE_PATH ?? '';

/** @type {import('next').NextConfig} */
const config = {
  output: 'export',
  reactStrictMode: true,
  basePath,
  assetPrefix: basePath || undefined,
  trailingSlash: true,
  images: {
    unoptimized: true,
  },
};

export default withMDX(config);
