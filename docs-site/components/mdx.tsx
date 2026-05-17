import defaultComponents from 'fumadocs-ui/mdx';
import type { MDXComponents } from 'mdx/types';
import { Callout } from 'fumadocs-ui/components/callout';
import { Card, Cards } from 'fumadocs-ui/components/card';
import { Tab, Tabs } from 'fumadocs-ui/components/tabs';
import { Step, Steps } from 'fumadocs-ui/components/steps';
import { TypeTable } from 'fumadocs-ui/components/type-table';

// MDX surface: the docs theme plus a few first-class doc components
// (Callout, Cards, Steps, Tabs, TypeTable) so authors don't have to
// import them in every page.
export function getMDXComponents(extra?: MDXComponents): MDXComponents {
  return {
    ...defaultComponents,
    Callout,
    Card,
    Cards,
    Tab,
    Tabs,
    Step,
    Steps,
    TypeTable,
    ...extra,
  };
}
