import { defineConfig, defineDocs } from 'fumadocs-mdx/config';

// All MDX content lives under content/docs. Fumadocs MDX will walk this
// tree, build the page index, and expose a typed `docs` source object.
export const { docs, meta } = defineDocs({
  dir: 'content/docs',
});

export default defineConfig({
  mdxOptions: {
    // Add any rehype/remark plugins here if needed later.
  },
});
