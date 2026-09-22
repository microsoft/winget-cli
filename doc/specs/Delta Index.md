---
author: JohnMcPMS, GitHub Copilot <Copilot>
created on: 2026-09-01
last updated: 2026-09-22
---

# Delta Index

## Abstract

The pre-indexed package source ships a complete SQLite index inside an MSIX package on every
update cycle, so every client downloads the entire index even though consecutive publishes
differ by a tiny fraction of their content. This spec proposes a **delta index**: a small
supplemental SQLite database holding only the rows that changed since a designated **baseline**
index, which the client combines with the baseline at query time using `ATTACH` and TEMP VIEWs.
The merge is transparent to the existing V2 read path, so search, correlation, and every other
consumer of the index operate without modification.

## Inspiration

Source update is the most frequent network operation winget performs and the one with the worst
ratio of bytes transferred to information gained. The index grows monotonically with the size of
the community repository, so the cost of this operation increases for every user over time,
while the amount of *new* data in each publish stays roughly constant.

The drivers:

- **Egress cost.** The full index is served to every client on every update cycle. The bytes are
  dominated by data the client already has.
- **User-visible time and bandwidth.** Users on metered or slow connections pay the full index
  cost repeatedly.
- **The trend is adverse.** As the community repository grows, so does the fixed per-update cost.
  Nothing in the current design amortizes it.

A proof of concept validated the core mechanism — generation of a delta from a baseline, and
transparent merged reads via SQLite views — and a measurement tool walking the git history of the
community repository was used to characterize how delta size grows relative to the full index
over time. Those measurements inform the baseline refresh cadence discussed under
[Baseline selection policy](#baseline-selection-policy).

> [!NOTE]
> **Implementation status.** The index-side work described here — the V2.1 schema, delta
> generation inside `PrepareForPackaging`, and the merged read path — is implemented. The
> service-side and client-side flows are not yet: there is no `delta.msix` acquisition, no
> baseline download, no telemetry event, and no feature toggle.

## Solution Design

### Terminology

| Term | Meaning |
|---|---|
| **Full index** | The complete V2 index as published in the pre-indexed package source (`index.db` inside `source2.msix`). |
| **Baseline** | A full index that has been *explicitly designated* as the reference point for one or more deltas, which stamps it with a baseline GUID. Structurally identical to a full index; "baseline" is a role conferred by designation, not a distinct format. |
| **Baseline GUID** | A unique identifier stamped into an index when it is designated a baseline. Deltas record the GUID of the baseline they were built against; the client uses GUID equality to guarantee it is pairing the right two files. |
| **Delta** | A SQLite database holding only the changes relative to a specific baseline. Not usable on its own. |
| **Baseline roll** | Publishing a new baseline, which resets every delta client's delta to empty and forces a one-time full download. |

Three flows make up the design:

1. **[Generation](#1-generation-flow)** — the service produces a delta alongside the full index.
2. **[Retrieval](#2-retrieval-flow)** — the client acquires the delta, and the baseline when needed.
3. **[Merge](#3-merge-mechanism)** — the client combines them at open time via `ATTACH` + TEMP VIEWs.

---

### 1. Generation Flow

Delta generation is an **additional step layered onto the existing index creation flow**, not a
separate pipeline or a separate index type. A standard V2 index must still support a plain
`PrepareForPackaging` call with no delta involvement; the delta path is opt-in and is engaged
only when the delta properties are set on the index.

#### Publisher sequence

After performing all the normal work for a publish cycle (add/update/remove manifests against
the working index), the caller performs four additional steps:

```
  ┌─────────────────────────────────────────────────────────────────┐
  │ 1. Existing flow: mutate the working index                      │
  │    AddManifest / UpdateManifest / RemoveManifest                │
  └────────────────────────────┬────────────────────────────────────┘
                               │
  ┌────────────────────────────▼────────────────────────────────────┐
  │ 2. Decide the baseline                                          │
  │                                                                 │
  │    Is it time to roll the baseline?                             │
  │      YES → prepare this publish as a full index, then call      │
  │            MarkAsBaseline on it → stamps a new baseline GUID    │
  │      NO  → retrieve the current baseline index                  │
  └────────────────────────────┬────────────────────────────────────┘
                               │
  ┌────────────────────────────▼────────────────────────────────────┐
  │ 3. Set the delta properties, then PrepareForPackaging:          │
  │      - DeltaBaselineIndexPath (must be designated, and must     │
  │          share this index's database identifier)                │
  │      - DeltaOutputPath (must not already exist)                 │
  │      - DeltaBaselineRelativeSourcePath (the relative path       |
  |          to the baseline file in storage)                       │
  │      - DeltaBaselinePackageVersion (the package version of      |
  |          the baseline to make some client checks more efficient)│
  │    → emits the prepared index AND the delta, the latter         │
  │      recording the baseline's GUID                              │
  └────────────────────────────┬────────────────────────────────────┘
                               │
  ┌────────────────────────────▼────────────────────────────────────┐
  │ 4. Package the delta into an MSIX with a DISTINCT identity      │
  │    from the baseline package                                    │
  └────────────────────────────┬────────────────────────────────────┘
                               │
  ┌────────────────────────────▼────────────────────────────────────┐
  │ 5. Publish                                                      │
  │      delta   → fixed location:   <root>/delta.msix              │
  │      baseline→ versioned location,                              |
  |                consistent with DeltaBaselineRelativeSourcePath  │
  │                ONLY when the baseline was rolled; an existing   │
  │                baseline is already published                    │
  │      full    → fixed location:   <root>/source2.msix (unchanged)│
  └─────────────────────────────────────────────────────────────────┘
```

#### Step 2 — baseline selection and designation

Whether to roll the baseline is a **service policy decision**, not a client or index concern.
The index code is told what the baseline is; it does not choose.

A baseline roll is expensive: every delta client pays a full-index download at their next
update. A baseline that is too old is also expensive: the delta grows monotonically until it
approaches the size of the full index. The optimum sits between those, and is discussed under
[Baseline selection policy](#baseline-selection-policy).

##### Designation is an explicit act

Being a baseline is **not** an implicit property of any prepared index. The service calls a
dedicated function — `MarkAsBaseline` — on a prepared full index to confer the role. That
function does exactly one thing: it generates a **baseline GUID** and stores it in the index
metadata under `baselineIdentifier`.

It refuses two kinds of index:

1. **A delta.** A delta describes change rather than holding a whole index, so it cannot serve
   as the baseline for another one. Both forms are refused — a delta opened on its own, which
   carries the identifier of the baseline it was built against, and the merged form, whose
   tables are views over a union of two databases.
2. **An index that has not been prepared.** The merged views are defined over the V2 tables, and
   an unprepared index still holds the V1.7 tables that `PrepareForPackaging` reads from. A
   baseline in that state is one no delta could be generated from or attached to.

An index without a baseline GUID cannot be used as a baseline, and delta creation rejects it.

This matters for correctness, not just tidiness. Without designation, any prepared index
silently qualifies as a baseline, and nothing tying a delta to a baseline is stronger than a
coincidence of ordering — which two independently produced indexes can satisfy while containing
entirely different data. Explicit designation plus a GUID makes the pairing verifiable rather
than presumed.

##### The change window is a sequence, not a timestamp

Deltas need to know which packages changed after the baseline was produced. That boundary is a
**monotonic change sequence** rather than a time.

Every write to the package update tracking table takes the next sequence number. Preparing a
V2.1 index records the highest sequence issued at that point into its own metadata, under
`deltaBaselineSequence`, and generation asks the tracking table for everything strictly after
the sequence recorded in the *baseline*. The window is therefore defined by two recorded
integers rather than by comparing clocks.

This is recorded during `PrepareForPackaging` for **every** V2.1 index, not only for ones that
are later designated. The index does not know at prepare time whether it will become a baseline,
and the value is a single metadata row, so recording it unconditionally costs nothing and avoids
a designation that arrives too late to be accurate.

Sequences are used rather than the existing write timestamps for two reasons:

- **A timestamp is not a boundary.** Multiple packages written in the same clock tick cannot be
  ordered against a cutoff that falls among them, so a time-based window either repeats work or
  silently drops it. A sequence has no ties by construction.
- **Clocks are not guaranteed to advance.** A system clock that steps backwards between publish
  cycles makes a later index appear earlier, which a comparison cannot detect. A sequence is
  derived from the table's own contents.

The write timestamp column is unchanged and still drives the existing intermediate-file output
path, which has its own base-time property. Sequences are additive, and exist only where
removals are recorded — that is, V2.1 and above.

#### Step 3 — delta creation inputs

Delta generation is engaged by setting two properties on the working index before
`PrepareForPackaging`:

| Property | Purpose |
|---|---|
| `DeltaBaselineIndexPath` | The baseline index file. Read-only source for the "before" state; every changed package is compared against it. Must carry a baseline GUID, and must belong to the same database lineage as the index being prepared. |
| `DeltaOutputPath` | Where to write the delta. Must not already exist. |
| `DeltaBaselineRelativeSourcePath` | The source base path relative location of the baseline file. This allows versioned baselines to exist and be independently controlled by the service. |
| `DeltaBaselinePackageVersion` | The baseline package version. Having this value isn't strictly necessary, but it will make some of the client checks more efficient. |

Setting none of these properties leaves `PrepareForPackaging` behaving exactly as it does for a V2.0 index.
All must be set for generation to run.

#### Step 3 — the baseline must be an ancestor, not merely a baseline

Two checks establish that the baseline is a legitimate predecessor of the index being prepared,
and both must pass before any processing begins:

- **Same database lineage.** The standard `databaseIdentifier` metadata value, which every index
  carries and which survives across prepares, must be identical in both. This rejects a baseline
  produced from an entirely different source, which a GUID check alone would not catch — the
  baseline GUID proves the file *is* a designated baseline, not that it is *this* index's
  ancestor.
- **The sequence does not go backwards.** The baseline's recorded `deltaBaselineSequence` must
  not exceed the index's current sequence. A baseline from the future describes changes that
  this index has not made.

Both failures raise `APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED`.

#### Step 3 — what delta creation does

Generation must run **inside** `PrepareForPackaging`, after the V2 tables have been built but
**before** the V1.7 tables and the package update tracking table are dropped. That is the only
moment where the finished V2 data and the change-tracking data coexist.

```
  a. Record this index's own change sequence into its metadata
  b. Open the baseline read-only; validate its GUID, lineage, and sequence
  c. Create the delta at a temporary path, with its schema
  d. Ask the update tracking table for everything after the baseline's
     sequence — changed packages and vacated rowids, reported separately
  e. For each changed package:
       - copy the current row, then for each 1:N and system-reference table,
         diff the current string set against the baseline's string set and
         record ONLY the differences: added values with is_removed = 0,
         values the baseline had but no longer apply with is_removed = 1.
         Unchanged associations are not written.
  f. For each vacated rowid:
       - record a package tombstone (is_removed = 1), carrying the identifier
         the baseline holds at that rowid. No per-association tombstones are
         written; the views suppress the baseline's association rows by
         reference to this one.
  g. Drop the generation-only indexes and vacuum
  h. Rename the temporary file into place
```

If the change set is empty, such as providing the baseline to itself, an empty delta database is produced (full schema with only metadata rows) and the publish proceeds as normal.

Three properties of the schema make the merge cheap and are worth stating explicitly because
they constrain generation:

- **Package rowids are stable across builds.** A package's rowid in the V2 `packages` table is
  pinned to its rowid in the V1 `ids` table, which is monotonic and never reused. The
  consequence is that the delta and the baseline occupy naturally disjoint-or-identical rowid
  space: an updated package has the *same* rowid in both, a new package has a rowid higher
  than anything in the baseline. No offset arithmetic is needed anywhere in the merge.
- **Value tables only ever gain rows.** `delta_tags2` / `delta_commands2` contain *only strings
  that do not already exist in the baseline*, assigned rowids starting above the baseline's
  maximum. A mapping to an already-existing string references the baseline's rowid directly.
  This is what allows the value-table views to be an unconditional `UNION ALL` with no
  filtering.
- **Association tables carry only the diff.** For the map and system-reference tables the delta
  records added and removed `(value, package)` pairs and nothing else. Unchanged associations
  live solely in the baseline. This keeps the delta small — a version bump typically changes no
  associations at all — but it means the merge must suppress baseline rows at row granularity
  rather than by package. See [§3](#3-merge-mechanism).

#### Schema version bump: V2.0 → V2.1

Removals require a change to the package update tracking table. Previously a removed package's
tracking row was deleted, which made the removal invisible to anything reading the tracking
table afterwards. It now records the removal instead, so the delta builder sees the complete
change set.

Three columns are added, and they are present **only** when the interface records removals — a
V2.0 index creates the table in exactly its original shape:

```sql
is_removed    INTEGER NOT NULL DEFAULT 0,   -- the row is a tombstone
package_rowid INTEGER NOT NULL,             -- the rowid the package occupies, or vacated
change_seq    INTEGER NOT NULL              -- monotonic; the delta's change window boundary
```

Two indexes come with them. One over `change_seq`, which serves both the range scan that reports
changes and the maximum that allocates the next sequence. One partial unique index over
`package_rowid WHERE is_removed = 0`, enforcing that a rowid has at most one live row; tombstones
are excluded from it, because a rowid vacated by one package can be taken by another and the two
would otherwise collide.

> [!NOTE]
> The live-row constraint is deliberately on the **rowid** rather than the identifier. A unique
> index cannot use `LIKE`, and no available collation matches it: `NOCASE` is ASCII-only, while
> `LIKE` here is the ICU implementation registered at connection open, so `NOCASE` would disagree
> with every other accessor on non-ASCII identifiers. An ICU collation cannot be used either, as
> it would bake the ICU version into a published index file.

This forces a V2.0 → V2.1 minor version bump. A delta can only be built against a baseline that
was itself built with removal tracking, which `MarkAsBaseline` guarantees by existing only on the
V2.1 interface — the base implementation throws `ERROR_NOT_SUPPORTED`.

#### Delta database schema

The delta is an ordinary SQLite database created through the same storage base as any index, so
it carries the standard `metadata` table, schema version, and database identifier. On top of that
it has one table per V2 table role, each named with a `delta_` prefix.

**Metadata values:**

| Key | Type | Description |
|-----|------|-------------|
| `deltaBaselineIdentifier` | TEXT | GUID of the baseline this delta was built against. Written into the delta; checked against the baseline's `baselineIdentifier` when the two are opened together. |
| `deltaBaselineRelativeSourcePath` | TEXT | Source base relative path to find the baseline package at. |
| `deltaBaselinePackageVersion` | TEXT | The version of the baseline package to make some client side checks more efficient. |

Two related values live in an ordinary index rather than in the delta:

| Key | Written to | Description |
|-----|-----|-------------|
| `baselineIdentifier` | A designated baseline | GUID minted by `MarkAsBaseline`. Its presence is what makes an index usable as a baseline. |
| `deltaBaselineSequence` | Every prepared V2.1 index | The change sequence reached when the index was prepared. Read from the *baseline* to determine the delta's change window. |

**Packages** — a row carries either the package's current state or the fact that it was removed,
so every column but the identifier is nullable:

```sql
CREATE TABLE delta_packages (
    rowid           INTEGER PRIMARY KEY,   -- stable ids-table rowid
    id              TEXT NOT NULL,
    name            TEXT,
    moniker         TEXT,
    latest_version  TEXT,
    arp_min_version TEXT,
    arp_max_version TEXT,
    hash            BLOB,
    is_removed      INTEGER NOT NULL
);
```

There is deliberately no index on `id`. Nothing looks a delta package up by identifier: the
merge is keyed entirely on `rowid`, and so is generation.

**Value tables** — new strings only, rowids allocated above the baseline's maximum:

```sql
CREATE TABLE delta_tags2 (
    rowid INTEGER PRIMARY KEY,
    tag   TEXT NOT NULL
);
CREATE UNIQUE INDEX delta_tags2_pkindex ON delta_tags2(tag);
-- identical shape for delta_commands2 (command TEXT NOT NULL)
```

The unique index exists **only during generation**, which looks a value up by string to reuse a
rowid it has already allocated for it. It is dropped before the delta is vacuumed, because the
merged view never searches this table by value — it is reached only through the map table, by
rowid. Carrying the index into the published file would be paying bytes for a lookup no client
performs.

There is no removal flag on a value table. A value becomes unreferenced only when every map entry
naming it is removed, which the map table already records.

**Map tables** — the diff of `(value rowid, package rowid)` pairs:

```sql
CREATE TABLE delta_tags2_map (
    tag        INTEGER NOT NULL,   -- rowid into the merged tags2 (baseline or delta)
    package    INTEGER NOT NULL,   -- stable packages.rowid
    is_removed INTEGER NOT NULL,
    PRIMARY KEY (tag, package)
) WITHOUT ROWID;
-- identical shape for delta_commands2_map
```

**System-reference tables** — `(value, package)` directly, with no separate value table:

```sql
CREATE TABLE delta_pfns2 (
    pfn        TEXT NOT NULL,
    package    INTEGER NOT NULL,
    is_removed INTEGER NOT NULL,
    PRIMARY KEY (pfn, package)
) WITHOUT ROWID;
-- identical shape for delta_productcodes2, delta_norm_names2,
-- delta_norm_publishers2, delta_upgradecodes2
```

The `WITHOUT ROWID` primary keys are load-bearing for merge performance — see
[Performance of the row-level anti-join](#performance-of-the-row-level-anti-join).

`is_removed` carries no `DEFAULT`. Generation always states it explicitly, and a default would
only serve to make a row that omitted it look deliberate.

#### Step 4 — packaging identity

The delta package must have a **different MSIX identity than the baseline package**. Both may
be present on a client simultaneously, so they cannot collide.

Baselines use **one identity published at multiple versioned URLs**. Rolling the baseline
publishes a higher package version of the same identity at a new relative path. This is what
makes retention automatic on the client: deploying the new baseline is an *upgrade* of the
existing baseline package, and the platform removes the previous version as part of that
upgrade. No explicit cache eviction logic is required.

#### Step 5 — publishing layout

```
<source root>/
  source2.msix                    ← full index, fixed name, unchanged (existing clients)
  source.msix                     ← V1 index, fixed name, unchanged (legacy clients)
  delta.msix                      ← delta, fixed name          (delta-aware clients)
  <baseline relative path>        ← baseline, versioned path   (delta-aware clients)
```

`source2.msix` continues to be published in full, indefinitely, for clients that do not
understand deltas. Delta publishing is purely additive — a client that has never heard of
`delta.msix` sees no change in behavior.

#### Baseline selection policy

The refresh cadence is derived offline from a cost model rather than fixed in the client or the
index code, and is expressed to the publishing automation as a simple rule — "roll every N
weeks", or "roll when the delta exceeds X% of the full index".

The model minimizes total egress across all download events. For a candidate refresh period `P`:

```
cost_per_download = cycle_avg_delta + weighted_p_baseline × full_avg
```

where `cycle_avg_delta` is the mean delta size across the refresh cycle, `full_avg` is the mean
full index size, and `weighted_p_baseline` is the fraction of download events that land on a
baseline roll and therefore pay the full cost. That fraction depends on how stale clients are
when they update, which is the one input not derivable from index measurements alone — see
[Telemetry](#telemetry).

> [!NOTE]
> All model inputs use *compressed* sizes. The index ships inside an MSIX, so the egress bytes
> are deflate-compressed. This matters disproportionately for the delta, because a small SQLite
> file carries a higher proportion of empty page data and compresses substantially better than
> the full index.

#### Telemetry

Tuning the cadence requires knowing the distribution of client staleness at update time. A
single event emitted on each index download supplies it:

| Field | Type | Nullable | Description |
|-------|------|----------|-------------|
| `SourceId` | string | No | Source being updated, for per-source stratification |
| `PreviousIndexPublishedAt` | datetime | Yes | Timestamp in the previously-held index; null for net-new clients |
| `NewIndexPublishedAt` | datetime | No | Timestamp in the newly-downloaded index |
| `IndexStalenessDays` | float | Yes | Pre-computed difference of the two above |
| `IsNewClient` | bool | No | True when the client held no prior index |
| `UsedDeltaDownload` | bool | No | True if a delta was downloaded rather than a full index |
| `PreviousBaselinePublishedAt` | datetime | Yes | Baseline held before this update |
| `NewBaselinePublishedAt` | datetime | Yes | Baseline in use after this update |
| `BaselineStalenessDays` | float | Yes | Pre-computed difference of the two above |
| `BaselineUpdated` | bool | No | True if a new baseline was downloaded this cycle |
| `DownloadedBytes` | long | No | Actual bytes transferred (baseline + delta, or full) |

Delta clients populate the `Index` fields from the delta's own timestamps, so the same fields
carry the same meaning for both client types and the analysis pipeline does not need to branch.
`ClientVersion` and the event timestamp are supplied by the telemetry infrastructure and are not
part of this event.

Net-new clients always pay the full baseline cost regardless of cadence, so they are modeled as
a distinct bucket rather than folded into the staleness distribution.

---

### 2. Retrieval Flow

#### Client sequence

```
  ┌─────────────────────────────────────────────────────────────────┐
  │ 1. Retrieve <root>/delta.msix                                   │
  │    Exactly the same mechanism used for source2.msix:            │
  │    version probe → download → trust validation → deploy/extract │
  │                                                                 │
  │    Not available / not understood?  →  fall back to source2.msix│
  └────────────────────────────┬────────────────────────────────────┘
                               │
  ┌────────────────────────────▼────────────────────────────────────┐
  │ 2. Read the baseline locator from the delta's metadata          │
  │      - baseline GUID                                            │
  │      - baseline relative path                                   │
  │      - baseline MSIX package version                            │
  └────────────────────────────┬────────────────────────────────────┘
                               │
                    Do we already have that baseline version?
                               │
              ┌────────────────┴─────────────────┐
             YES                                 NO
              │                                  │
              │              ┌───────────────────▼─────────────────┐
              │              │ 3. Retrieve the baseline from       │
              │              │    <root>/<baseline relative path>  │
              │              │    Same download + trust validation │
              │              │    Deploy → upgrades/replaces the   │
              │              │    previous baseline version        │
              │              └───────────────────┬─────────────────┘
              │                                  │
              └────────────────┬─────────────────┘
                               │
  ┌────────────────────────────▼────────────────────────────────────┐
  │ 4. Validate affinity: baseline's GUID == GUID named by the delta│
  └────────────────────────────┬────────────────────────────────────┘
                               │
  ┌────────────────────────────▼────────────────────────────────────┐
  │ 5. Open delta + baseline together (see §3)                      │
  └─────────────────────────────────────────────────────────────────┘
```

#### Step 1 — reuse of the existing acquisition path

The delta is acquired through the **same code path as the full index**: the same version
header probe to decide whether an update is needed, the same download, the same MSIX trust
validation, and the same split between the desktop context (download the `.msix`, validate,
extract `Public\index.db`) and the packaged context (deploy the MSIX so the platform manages
it). The delta is just another package at a fixed name under the source root.

This matters for two reasons. It keeps the amount of new networking and trust code near zero,
and it means the delta inherits the existing signature validation and trust-level semantics
without a parallel implementation that could drift.

#### Step 2 — baseline discovery is delta-driven

The client does **not** independently know where the baseline is or which one it needs. It
learns both from the delta it just downloaded. This is deliberate:

- The service can relocate baselines without a client change, because the path travels inside
  the delta.
- The delta and the baseline it was built from can never be mismatched by a stale client-side
  assumption — the delta names its own baseline.
- Rolling the baseline requires no client-visible signal beyond the next delta naming a
  different version.

The version is the cheap check. If the locally deployed baseline package's version already
equals the version named in the delta, no network activity is needed for the baseline at all —
which is the common case, since the baseline changes far less often than the delta.

#### Step 4 — baseline affinity

The client verifies that the baseline it holds carries **the same baseline GUID the delta
names**. This is an equality check on an identifier stamped at designation time, not a content
comparison.

A GUID rather than a hash of the baseline file, for three reasons:

- **`PrepareForPackaging` is not byte-reproducible.** It ends in a VACUUM and stamps fresh
  timestamps, so regenerating a logically identical baseline yields a different file. A hash
  would force the service to archive the exact published bytes for as long as any delta
  references them, and would break the moment a baseline was repackaged or re-vacuumed.
- **Identity should survive packaging.** The client extracts the index from an MSIX. Hash
  validation makes correctness depend on extraction being byte-faithful; GUID equality depends
  only on the logical content the service designated.
- **Designation is where identity belongs.** The GUID is created by the same explicit act that
  confers the baseline role, so the two cannot drift apart.

**Affinity is not integrity, and does not need to be.** The GUID proves the delta and baseline
were built as a pair; it does not prove the baseline is uncorrupted. Corruption and tampering
are already covered by the MSIX signature validation both packages receive through the existing
acquisition path. A file hash would duplicate that coverage, so it is deliberately not used.

The failure mode matters as much as the check. A signed-but-wrong baseline paired with a delta
produces a merged view that is silently incomplete — packages missing, versions stale — with no
error anywhere. That is the specific outcome the GUID check exists to prevent. On mismatch the
client must refuse the pair, re-acquire the baseline named by the delta, and fall back to the
full index if that fails.

#### Back-compat metadata

Additional metadata is required in one or both packages to keep old and new clients coexisting
safely:

- **A delta-aware client must not misread a baseline as a full index, or vice versa.** They are
  structurally the same format; only the role differs. The baseline GUID stamped by
  `MarkAsBaseline` serves as this positive marker — an index carrying one has been designated,
  an index without one has not.
- **A pre-delta client must never be handed a delta.** This is handled structurally by the
  fixed-name separation — such clients only ever ask for `source2.msix` — but the delta database
  should still be identifiable as non-standalone so that a mistaken direct open fails loudly
  rather than producing an index that appears valid but is missing most of its rows.

#### Fallback behavior

Every failure in the delta path falls back to downloading the full index. The delta is an
optimization, never a correctness dependency.

| Condition | Behavior |
|---|---|
| `delta.msix` not present at the source root | Full index |
| Delta downloaded but metadata unreadable or incomplete | Full index |
| Baseline named by the delta cannot be retrieved | Full index |
| Baseline GUID does not match the delta's | Re-acquire baseline once; then full index |
| Baseline schema version differs from the delta's | Full index |
| Merged open fails for any reason | Full index |
| Source is not a pre-indexed package source (REST, Store) | Not applicable — no change |

---

### 3. Merge Mechanism

#### The approach

1. Both databases are opened in SQLite — the delta as the main connection, the baseline
   `ATTACH`ed to it.
2. A **TEMP VIEW is created for every table**, named identically to the real table it stands in
   for, producing the merged row set.
3. **Existing read code is unchanged.** It queries `packages`, `tags2`, `tags2_map` and so on
   exactly as it does against a normal index, and never learns that it is talking to a view.

```
   ┌──────────────────────────────────────────────────────┐
   │  V2_0::Interface  (search, correlation, all reads)   │
   │  — completely unmodified —                           │
   └───────────────────────┬──────────────────────────────┘
                           │ SELECT ... FROM packages
                           ▼
   ┌──────────────────────────────────────────────────────┐
   │  TEMP VIEW packages                                  │
   │  TEMP VIEW tags2 / tags2_map / commands2 / ...       │
   │  TEMP VIEW pfns2 / productcodes2 / ...               │
   └──────────┬───────────────────────────┬───────────────┘
              │                           │
              ▼                           ▼
   ┌────────────────────┐      ┌─────────────────────────┐
   │  delta.db  (main)  │      │  baseline.db (ATTACH)   │
   │  delta_packages    │      │  packages               │
   │  delta_tags2 ...   │      │  tags2 ...              │
   └────────────────────┘      └─────────────────────────┘
```

#### Why views rather than anything else

The decisive property is that **the entire V2 read surface keeps working untouched**. The
search layer is large, performance-sensitive, and correct; rewriting it to be delta-aware would
be a substantial change with a substantial regression risk, and would have to be maintained in
parallel with the non-delta path forever. A view that shadows the table name means there is
exactly one read path, exercised identically by both configurations.

Secondary benefit: SQLite's query planner still sees the whole composed dataset, so index usage
and join ordering continue to work across the merged rows.

Alternatives considered and rejected:

| Alternative | Why rejected |
|---|---|
| Materialize the merge into a new database at open | Produces a file the size of the full index, on the client, on every update. Defeats the entire purpose. |
| Merge in C++ above SQLite | Requires reworking the search layer, loses the query planner across the merged set, and creates two divergent read paths. |
| Persistent (non-TEMP) views | Not possible. A stored view cannot reference an attached database, because the attachment alias only exists for the session. |

The views must be TEMP for that last reason — the `baseline` schema name only exists after the
`ATTACH` on that specific connection, so the views have to be created per-connection after
attaching.

#### View shapes

Three patterns cover every table, corresponding to the three roles that need change-tracking.

**Packages** — delta rows win; tombstones drop out; untouched baseline rows pass through.

```sql
CREATE TEMP VIEW packages AS
  SELECT rowid, id, name, moniker, latest_version, arp_min_version, arp_max_version, hash
  FROM delta_packages WHERE is_removed = 0
  UNION ALL
  SELECT b.rowid, b.id, b.name, b.moniker, b.latest_version, b.arp_min_version, b.arp_max_version, b.hash
  FROM baseline.packages b
  WHERE NOT EXISTS (SELECT rowid FROM delta_packages d WHERE d.rowid = b.rowid);
```

A baseline package is superseded whenever the delta mentions its **rowid** at all: the delta row
replaces it when the package changed, and stands for its absence when it was removed — the
tombstone suppresses the baseline row here while being filtered out of the first branch by
`is_removed = 0`.

> [!IMPORTANT]
> Suppression is keyed on `rowid`, not on `id`. Identifiers are matched by `LIKE` elsewhere in
> the index, and the `ids` table collapses identifiers differing only by case onto a single row,
> overwriting the stored string with the most recent casing. A baseline and a delta can therefore
> hold different spellings of the same package. The rowid is the identity that is actually
> stable, and matching on it is also a primary-key seek rather than a string comparison.

**Value tables** (`tags2`, `commands2`) — unconditional union, no filtering, because generation
guarantees the delta only ever contains strings the baseline does not have, at rowids above the
baseline's maximum.

```sql
CREATE TEMP VIEW tags2 AS
  SELECT rowid, tag FROM delta_tags2
  UNION ALL
  SELECT rowid, tag FROM baseline.tags2;
```

Nothing is suppressed here, including values no longer being referenced. The map table governs
what is visible, so an unreferenced value simply never appears.

**Map and system-reference tables** — the delta holds **only the diff**: rows added since the
baseline (`is_removed = 0`) and tombstones for rows the baseline had that are now gone
(`is_removed = 1`). Associations that did not change are not represented at all. The baseline
row therefore passes through unless the delta specifically claims that pair.

```sql
CREATE TEMP VIEW tags2_map AS
  SELECT tag, package FROM delta_tags2_map WHERE is_removed = 0
  UNION ALL
  SELECT b.tag, b.package FROM baseline.tags2_map b
  WHERE NOT EXISTS (
      SELECT package FROM delta_tags2_map d
      WHERE d.tag = b.tag AND d.package = b.package)
    AND NOT EXISTS (
      SELECT rowid FROM delta_packages p
      WHERE p.rowid = b.package AND p.is_removed = 1);
```

> [!IMPORTANT]
> The suppression is **row-level** (`tag` *and* `package`), not package-level. A package that
> gains a single tag must keep all of its other tags, which are present only in the baseline and
> are never copied into the delta. Suppressing by package alone would silently drop every
> unchanged association of every updated package, degrading package correlation by
> PFN and product code.

The first `NOT EXISTS` deliberately does **not** test `is_removed`. The delta owns any pair it
names: if it holds the pair as added, the first branch already emits it, and letting the baseline
copy through as well would emit it twice. Testing `d.is_removed = 1` here would duplicate every
association that appears on both sides, which is every
unchanged association of a package that changed for some other reason.

The second `NOT EXISTS` drops associations belonging to deleted packages. Removing a package
does **not** write a tombstone per association — that would be pure waste — so the association
rows are instead suppressed by reference to the package tombstone. Without this, a value-first
lookup (`SELECT package FROM tags2_map WHERE tag = ?`) could return the rowid of a package that
no longer exists in the `packages` view.

The same shape applies to `commands2_map`, and to the system-reference tables (`pfns2`,
`productcodes2`, `norm_names2`, `norm_publishers2`, `upgradecodes2`), which carry
`(value, package)` directly with no separate value table. The two are identical as far as merging
is concerned — one holds the value inline, the other a reference to it — so a single
implementation covers both.

#### Performance of the row-level anti-join

The concern with diff-only is that the baseline branch carries two `NOT EXISTS` predicates
rather than a single package-level `NOT IN`. In practice this is not a meaningful cost:

- **Both probes are exact B-tree seeks into small tables.** The delta map and system-reference
  tables are `PRIMARY KEY (value, package) WITHOUT ROWID`, so the table *is* the index on
  exactly the two columns being matched — the probe lands on at most one row and reads
  `is_removed` from it. `delta_packages` is keyed by `rowid INTEGER PRIMARY KEY`, so the package
  check is a rowid seek. No supplementary indexes are required.
- **Complexity is unchanged.** Per baseline row the cost is `O(log D)` against a small delta,
  the same order as the package-level `NOT IN` it replaces (which SQLite services by
  materializing an ephemeral index and probing it once per row). Diff-only performs two seeks
  where full-set would perform one, against tables that are orders of magnitude smaller than the
  baseline.
- **The predicates are rarely evaluated at scale.** These views are almost never scanned in
  full. Real queries filter first — `WHERE tag = ?`, `WHERE pfn = ?` — and SQLite pushes the
  filter into each `UNION ALL` branch independently, seeking the baseline table by its own key.
  The anti-join predicates then run only against the handful of rows that survived the filter,
  making the practical cost `O(log B + log D)` per lookup rather than anything proportional to
  the baseline's size.

The net effect is a substantially smaller delta for an unmeasurable difference in query cost.

#### Open sequence

```
  1. Open delta.db as the main connection
  2. ATTACH baseline.db AS baseline
  3. Validate the pair:
       - the baseline's baselineIdentifier equals the delta's
         deltaBaselineIdentifier
       - the two schema versions are equal
     On failure, DETACH and throw
  4. Create the TEMP VIEWs, shadowing every real table name
  5. Construct the normal V2 interface
     → its state detection is told that the index is already in
       post-PrepareForPackaging form, and all reads proceed unchanged
```

This is exposed as an additional construction entry point on the existing index type
(`OpenWithBaseline`) rather than a new type. The resulting object is an ordinary index as far
as every consumer is concerned. Underneath, it is a single virtual call — `SetupDeltaReadMode` —
that only the V2.1 interface implements; every other version inherits a base that throws
`ERROR_NOT_SUPPORTED`.

#### Constraints this imposes

- **Read-only.** The merged view is not writable and is not intended to be. Deltas are consumed,
  never mutated, on the client.
- **Both files must remain available** for the lifetime of the connection, since the baseline is
  attached rather than copied.
- **Rowid stability is load-bearing.** If package rowids were not pinned to the `ids` table
  rowid, every view would need offset arithmetic and the map tables would need rewriting on the
  client. The generation-side rowid pinning is what keeps the merge this simple, and it cannot
  be relaxed independently.

---

### API surface changes

**`SQLiteIndex` properties** — new values used to engage the delta path during
`PrepareForPackaging`.

```cpp
enum class Property
{
    PackageUpdateTrackingBaseTime,
    IntermediateFileOutputPath,
    DeltaBaselineIndexPath,             // new
    DeltaOutputPath,                    // new
    DeltaBaselineRelativeSourcePath,    // new
    DeltaBaselinePackageVersion,        // new
};
```

**`ISQLiteIndex`** — two new virtuals, implemented only by V2.1, with base implementations that
throw `ERROR_NOT_SUPPORTED`:

```cpp
virtual void MarkAsBaseline(SQLite::Connection& connection);
virtual void SetupDeltaReadMode(SQLite::Connection& connection, const SQLite::DatabaseSpecifier& baseline);
```

For the WinGetUtil C API:
```c
WINGET_UTIL_API WinGetSQLiteIndexMarkAsBaseline(
    WINGET_SQLITE_INDEX_HANDLE index);
```
with matching additions to the `WinGetSQLiteIndexProperty` enum, and corresponding
`IWinGetFactory` / `IWinGetSQLiteIndex` members in the C# interop layer.

### Client integration

The pre-indexed package source factory gains delta awareness:

- A `delta.msix` location alongside the existing `source2.msix` / `source.msix` candidates.
- Baseline acquisition driven by the delta's metadata, reusing the existing download and trust
  validation helpers. This is what requires the baseline's publish path and package version to be
  recorded in the delta.
- Merged open via `OpenWithBaseline` when a valid pair is available, and the fallback table
  above otherwise.

### Feature gating

Delta index acquisition ships behind an experimental feature toggle so it can be validated in
the field and disabled without a client update if the service side needs to be rolled back:

```json
{
    "$schema": "https://aka.ms/winget-settings.schema.json",
    "experimentalFeatures": {
        "deltaIndex": true
    }
}
```

When disabled, the client never requests `delta.msix` and behaves exactly as it does today. The
toggle is removed once the feature graduates.

### Areas explicitly not impacted

| Area | Impact |
|---|---|
| **Manifest schema** (`schemas/JSON/manifests/`) | None. No manifest field is added, changed, or interpreted differently. The delta operates on the index built *from* manifests, not on manifests themselves. |
| **COM API** (`Microsoft.Management.Deployment`) | None. No IDL change. The merged index is an ordinary index; `PackageManager`, `CatalogPackage`, and every other projection behave identically. |
| **PowerShell cmdlets** | None. `Find-WinGetPackage`, `Install-WinGetPackage`, and the rest consume the same catalogs through the COM API and see no difference. |
| **CLI arguments** | None. No new argument on any command. Source acquisition is not user-parameterized. |
| **Group Policy** | None. Existing source-related policies continue to govern which sources may be used; how a source's index is transferred is not a policy concern. |
| **winget-create** | None. Manifest authoring is unaffected. |
| **winget-cli-restsource** | None. REST sources do not use the pre-indexed package format. |
| **winget-pkgs validation pipeline** | None for validation itself. The repository's manifests and validation rules are unchanged. The index *publishing* automation that consumes this repository is where the generation flow is adopted. |
| **WinGet Configuration / DSC** | None. Configuration flows consume packages through the same catalogs. |

## UI/UX Design

The feature is intentionally invisible. The success criterion is that users notice only that
source update is faster and transfers less data.

`winget source update` output is unchanged in structure:

```
> winget source update
Updating all sources...
Updating source: winget...
Done
```

The progress indicator during acquisition reflects the smaller transfer, which is the only
directly observable difference in the common case. On a baseline roll the transfer is almost the same
size as a full index download today.

When a baseline must also be acquired, both transfers are reported as a single source update —
the two-file nature of the acquisition is not surfaced, because it is not actionable by the
user:

```
> winget source update
Updating all sources...
Updating source: winget...
Done
```

Diagnostic detail goes to the log rather than the console, since it is only useful when
investigating a problem:

```
Source `winget` delta acquired; baseline {GUID} version 2.1.20260901.1 already present
Source `winget` opened as delta + baseline
```

On fallback, the log records the reason and the console remains unchanged — the user gets a
successfully updated source either way:

```
Source `winget` delta baseline GUID mismatch; falling back to full index
```

`winget --info` and `winget source list` are unchanged. No new user-facing string is required
beyond log output, so there is no new localization surface.

> [!NOTE]
> Because the console output is unchanged, there is no impact on scripts that parse source
> update output.

## Capabilities

### Accessibility

No impact. The feature adds no new console output, no new prompts, and no new interactive
elements. Existing source update output — which is already screen-reader compatible — is
unchanged in structure and content. Nothing about the change depends on color, VT sequences, or
cursor positioning, so behavior under `--no-vt` is identical to today.

### Security

The delta and the baseline are both MSIX packages acquired through the **existing** download and
trust validation path, so they inherit the current signature validation and source trust-level
semantics without a parallel implementation. No new trust decision is introduced.

Specific considerations:

- **Package substitution.** The baseline GUID check ensures a delta is only ever applied to the
  baseline it was built against. A correctly signed but mismatched baseline is rejected rather
  than silently producing an incomplete catalog. This is the principal new integrity property
  and the reason affinity is checked explicitly rather than inferred. Generation adds a second
  check on the same theme in the other direction: the baseline must share the index's
  `databaseIdentifier`, so a delta cannot be built against a designated baseline belonging to an
  unrelated source.
- **Baseline location is service-controlled data.** The baseline relative path travels inside
  the delta, which means a compromised delta could name an arbitrary path. The path is resolved
  strictly relative to the source root already configured for that source, and the resulting
  package is subject to the same signature and trust validation as any other. It cannot be used
  to reach a different origin.
- **Attack surface.** Two packages are acquired instead of one, but both through the same
  validated path. The delta database itself is only ever opened read-only, and the baseline is
  attached with its read-only disposition carried in the URI rather than inherited from the
  connection.
- **Downgrade.** A client that cannot validate the pair falls back to the full index, which is
  the current behavior, so failure never results in a less-trusted outcome.

### Reliability

Reliability improves in aggregate: smaller transfers complete more often on unreliable
connections, and a failed delta download costs far less to retry than a failed full index
download.

The design is fail-safe by construction. Every failure mode in the
[fallback table](#fallback-behavior) resolves to downloading the full index, which is exactly
what the client does today. The delta path can fail completely — service outage, malformed
delta, missing baseline, GUID mismatch — without preventing a source update from succeeding.

The risks that require care are correctness rather than availability risks, since a mis-merged
index fails silently rather than loudly. They are covered under [Potential Issues](#potential-issues).

### Compatibility

**No breaking changes.** The design is purely additive at every layer:

- **Existing clients** continue to download `source2.msix` from its fixed location, which
  continues to be published in full indefinitely. A client with no knowledge of `delta.msix`
  observes no change whatsoever.
- **The V2.0 → V2.1 bump is additive.** It adds columns to an internal tracking table that is
  dropped before packaging, so it is not visible in a published index. Published V2.1 indexes
  remain readable by V2.0-aware clients. Creating a V2.0 index still produces exactly the V2.0
  schema; the added columns and the nullability relaxation they require appear only on migration
  to V2.1.
- **Delta-aware clients against a non-delta source** find no `delta.msix` and fall back
  immediately, so a source that has not adopted delta publishing works unchanged.
- **The default index version is unchanged.** Creating an index without naming a version still
  produces V1.7, because the unqualified "latest" resolution is still the V1 map. Only an
  explicit request for the latest V2 now resolves to V2.1 rather than V2.0, and the index
  publishing tooling names its version explicitly.
- **A V2.1 client against an older baseline** is prevented by the schema version equality check
  performed when the two are opened together.
- **Third-party sources** using the pre-indexed format are unaffected unless they choose to
  publish deltas. Nothing requires them to.

The one compatibility constraint the service must honor is that a baseline remains published for
as long as any delta references it. Removing a baseline while deltas still name it forces those
clients to fall back to the full index — degraded but not broken.

### Performance, Power, and Efficiency

This is the motivating capability.

**Network.** The steady-state transfer per source update drops from the full compressed index to
the compressed delta. Delta size is proportional to the number of packages changed since the
baseline rather than the total package count, so the saving grows as the repository grows. The
amortized cost is the delta on most updates plus the full index once per baseline roll, and the
roll cadence is tuned to minimize that total.

**Compression matters disproportionately.** Because the payload ships in an MSIX, the relevant
figure is compressed size. Small SQLite files contain a higher proportion of empty page data and
compress better than large ones, so the delta's advantage in egress bytes is larger than its
advantage in file size.

**Disk.** The client stores a baseline plus a delta rather than a single index. The baseline is
approximately the size of today's index, so steady-state disk usage increases by roughly the
size of the delta. Baseline retention is handled by MSIX upgrade semantics, so old baselines do
not accumulate.

**Query time.** Effectively unchanged. Reads go through TEMP VIEWs whose extra predicates are
exact seeks into tables orders of magnitude smaller than the baseline, and which are evaluated
only against rows surviving the query's own filters — see
[Performance of the row-level anti-join](#performance-of-the-row-level-anti-join). Query plans
are still produced by SQLite across the composed dataset.

**Open time.** One additional `ATTACH` and a fixed number of `CREATE TEMP VIEW` statements per
connection. Both are constant-cost operations independent of index size.

**Power.** Less radio-on time for the dominant transfer, which is the meaningful mobile and
battery consideration.

**Service.** Generation adds a diff pass over changed packages during index publishing, bounded
by the change set rather than the catalog size. The egress reduction is the point of the
exercise.

## Potential Issues

**Silent incorrectness is the primary risk.** A merge defect does not throw — it yields an index
that looks valid but is missing rows. The specific hazard is suppressing baseline associations at
package granularity rather than row granularity, which would drop every unchanged tag, command,
and system-reference string of any updated package and quietly degrade package correlation by
PFN and product code. Testing must assert on the *associations of updated packages*, not merely
on added and removed packages, because the latter passes while the former is broken.

**Baseline lifetime coupling.** The service must keep a baseline published for as long as any
delta references it. Retiring a baseline too early does not break clients — they fall back to the
full index — but it silently eliminates the benefit for everyone still on it.

**Baseline roll is a synchronized cost spike.** Every delta client pays a full download at their
next update after a roll. The aggregate egress on roll day approaches the pre-delta baseline
cost.

**Rowid stability is a hidden invariant.** The merge is only this simple because package rowids
are pinned to the `ids` table rowid. Anything that changes rowid assignment during
`PrepareForPackaging` breaks the merge in a way that is not locally obvious from the code being
changed.

Pinning is **not** conditional on delta generation — every V2 prepare does it, because a delta
may be built against any prepared index and the decision is not known at that point. Two
consequences follow for indexes that have nothing to do with deltas:

- **Published index bytes change.** A package now occupies the rowid its identifier was first
  assigned, rather than one handed out in the order packages happened to be written.
- **Search results that tie now order differently.** Match results are ordered by match quality
  alone, and equally good matches previously emerged in alphabetical order as a side effect of
  how rowids were assigned. They now emerge in the order packages were first added. Nothing
  documented or depended upon changes — the index defines no order among equal matches, and
  deliberately does not — but `winget search` preserves index order for a free-text query, so the
  output is visibly reshuffled. Ordering equally good matches is a presentation decision and
  belongs to the caller; the results table does not even lead with the identifier, so name would
  likely be the better key.

**Rowid reuse complicates removal.** Rowids are recycled, so a rowid vacated by one package can
be taken by another within the same delta window. Generation writes changed packages before
tombstones and skips any rowid already written, and skips tombstones for rowids the baseline
never held. Neither case is rare enough to leave to chance in a repository with steady churn.

**Two-package acquisition has more failure states.** A baseline roll makes source update a
two-download operation, which is more exposed to interruption. The fallback path bounds the
damage but the interaction of partial failures deserves deliberate testing.

**Disk growth on clients.** Steady state now holds a baseline plus a delta. The increase is
modest, but it is an increase, and it lands on machines that may be storage constrained.

**Chained deltas are not addressed.** A delta against a delta would reduce the cost of a
baseline roll considerably, but it multiplies the merge complexity and the affinity checking. It
is deliberately out of scope; see [Future considerations](#future-considerations).

## Deprecation Path

Not applicable. Nothing is being replaced or removed at this time. `source2.msix` continues
to be published indefinitely, and no manifest field, setting,
CLI argument, or API is deprecated by this change.

The only phased element is the experimental feature toggle described under
[Feature gating](#feature-gating), which is removed once the feature graduates — that is a
maturity progression rather than a deprecation, and it removes no user-visible surface.

## Resources

- [SQLite `ATTACH DATABASE`](https://www.sqlite.org/lang_attach.html) — the mechanism underlying
  the merge
- [SQLite `CREATE VIEW`](https://www.sqlite.org/lang_createview.html) — including TEMP view scope
- [SQLite `WITHOUT ROWID` tables](https://www.sqlite.org/withoutrowid.html) — the property that
  makes the anti-join probes exact seeks
- [SQLite query planner](https://www.sqlite.org/optoverview.html) — relevant to how filters are
  pushed into `UNION ALL` branches
- [SQLite in-memory databases](https://www.sqlite.org/inmemorydb.html)
  — for referencing in memory database as part of attach
