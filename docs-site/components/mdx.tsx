import defaultComponents from 'fumadocs-ui/mdx';
import type { MDXComponents } from 'mdx/types';
import { Callout } from 'fumadocs-ui/components/callout';
import { Card, Cards } from 'fumadocs-ui/components/card';
import { Tab, Tabs } from 'fumadocs-ui/components/tabs';
import { Step, Steps } from 'fumadocs-ui/components/steps';
import { TypeTable } from 'fumadocs-ui/components/type-table';

import { Signature, ApiSection } from '@/components/api/signature';
import { ParamTable, Returns } from '@/components/api/param-table';
import { ErrorTable } from '@/components/api/error-table';
import { Badge } from '@/components/ui/badge';

/**
 * MDX surface: Fumadocs defaults plus a curated set of doc components.
 * MDX authors get all of these as in-scope identifiers without needing
 * explicit imports per page.
 */
export function getMDXComponents(extra?: MDXComponents): MDXComponents {
  return {
    ...defaultComponents,
    // Fumadocs UI
    Callout,
    Card,
    Cards,
    Tab,
    Tabs,
    Step,
    Steps,
    TypeTable,
    // MicroRTOS API-doc primitives
    Signature,
    ApiSection,
    ParamTable,
    Returns,
    ErrorTable,
    Badge,
    ...extra,
  };
}
