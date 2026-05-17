import Link from 'next/link';

export default function NotFound() {
  return (
    <main className="flex flex-1 flex-col items-center justify-center px-6 py-24 text-center">
      <p className="font-mono text-sm font-semibold text-fd-primary">404</p>
      <h1 className="mt-2 text-3xl font-bold tracking-tight sm:text-4xl">
        Page not found
      </h1>
      <p className="mt-3 max-w-md text-fd-muted-foreground">
        The page you were looking for moved or never existed. Try the docs
        homepage.
      </p>
      <Link
        href="/docs/getting-started"
        className="mt-8 inline-flex items-center gap-2 rounded-lg bg-fd-primary px-5 py-2.5 text-sm font-semibold text-fd-primary-foreground transition hover:opacity-90"
      >
        Go to docs
      </Link>
    </main>
  );
}
