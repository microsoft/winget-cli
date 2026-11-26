## New Features

### Natural name sorting and relevance groups in package tables

Package name sorting now uses case-insensitive Windows numeric comparison, so otherwise matching application names containing `9.0` appear before `10.0`.

Package tables now use match relevance, then name within each relevance group as their default order. Installed-package and upgrade lists retain their existing name-only default when no query is supplied. Search results and multiple-match candidate tables use relevance followed by name with or without a query.

The `relevance` sort field now compares match type and matched-field importance instead of acting as a no-op, allowing `winget list --sort relevance --sort name` to sort names within relevance groups. Explicit fields and `output.sortOrder` / `output.sortDirection` settings override the defaults; relevance alone retains incoming order within equal-relevance groups.

The `search` and `upgrade` commands now also accept `--sort` (repeatable), `--ascending` / `--asc`, and `--descending` / `--desc`. For example, `winget search Runtime --sort relevance --sort name` and `winget upgrade --sort name --descending` select the displayed order.

These changes affect presentation only. Repository/API result ordering, result limits, package eligibility, and upgrade execution order are unchanged.
