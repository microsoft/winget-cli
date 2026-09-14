---
author: JohnMcPMS, GitHub Copilot <Copilot>
created on: 2026-09-01
last updated: 2026-09-01
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
  │ 3. Call delta creation, supplying:                              │
  │      - the baseline index file (must be designated)             │
  │      - the relative path the baseline will be published at      │
  │      - the MSIX package version of the baseline                 │
  │    → emits delta.db, recording the baseline's GUID              │
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
  │      baseline→ versioned location (the relative path from #3)   │
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
function:

1. Validates the index is a legitimate baseline candidate: post-`PrepareForPackaging`, schema
   V2.1 or higher, and not itself a delta.
2. Generates and stores a **baseline GUID** in the index metadata.
3. Records the designation timestamp used as the change-window origin for deltas built against
   it.

An index without a baseline GUID cannot be used as a baseline, and delta creation rejects it.

This matters for correctness, not just tidiness. Without designation, any prepared index
silently qualifies as a baseline, and the only thing tying a delta to a baseline is a timestamp
comparison — which two independently produced indexes can satisfy by coincidence while
containing entirely different data. Explicit designation plus a GUID makes the pairing
verifiable rather than presumed.

Recording the change-window origin at designation time also resolves a race: using "now" at
generation time would miss package updates that land between the moment the baseline was
prepared and the moment it was published.

#### Step 3 — delta creation inputs

Delta creation takes three inputs beyond the working index:

| Input | Purpose |
|---|---|
| **Baseline index file** | Read-only source for the "before" state. Every changed package is compared against this. Must carry a baseline GUID; generation fails if it does not. The GUID is copied into the delta. |
| **Baseline relative path** | Where the baseline package will live, relative to the source root. Recorded in the delta so the client can locate the baseline. Deliberately **independent** of the version so that the service retains freedom in how it lays out storage. |
| **Baseline MSIX package version** | The identity version of the baseline package. Recorded in the delta so the client can determine whether the baseline it already has is the one this delta needs, without a network round trip. |

The path and version are independent inputs. The path is not derived from the version.

#### Step 3 — what delta creation does

Generation must run **inside** `PrepareForPackaging`, after the V2 tables have been built but
**before** the V1.7 tables and the package update tracking table are dropped. That is the only
moment where the finished V2 data and the change-tracking data coexist.

```
  a. Open the baseline read-only; read and validate its baseline GUID
  b. Create delta.db and its schema
  c. Ask the update tracking table for everything that changed since the
     baseline's designation timestamp — adds, updates, AND removes
  d. For each changed package:
       - removed  → record a package tombstone (is_removed = 1). No per-
                    association tombstones are written; the views suppress the
                    baseline's association rows by reference to this one.
       - added or
         updated → copy the current row, then for each 1:N and system-reference
                   table, diff the current string set against the baseline's
                   string set and record ONLY the differences: added values with
                   is_removed = 0, values the baseline had but no longer apply
                   with is_removed = 1. Unchanged associations are not written.
  e. Write metadata: baseline GUID, baseline relative path, baseline MSIX
     version, minimum baseline schema version, timestamps
  f. Vacuum
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
table afterwards. It now sets `is_removed = 1` instead, so the delta builder sees the complete
change set.

```sql
ALTER TABLE package_update_tracking ADD COLUMN is_removed INTEGER NOT NULL DEFAULT 0;
```

This forces a V2.0 → V2.1 minor version bump. A delta can only be built against a baseline that
was itself built with removal tracking, so `MarkAsBaseline` enforces V2.1 or higher.

#### Delta database schema

The delta carries a `metadata` table using the existing named-value mechanism, plus one table
per V2 table role.

**Metadata values:**

| Key | Type | Description |
|-----|------|-------------|
| `BaselineGuid` | TEXT | GUID of the baseline this delta was built against |
| `BaselineRelativePath` | TEXT | Publish location of the baseline, relative to the source root |
| `BaselinePackageVersion` | TEXT | MSIX package version of the baseline package |
| `MinimumBaselineSchemaVersion` | TEXT | Lowest baseline schema version this delta can be applied to |
| `DeltaTimestamp` | INTEGER | Unix epoch when this delta was generated |

**Packages** — tombstones carry only `rowid`, `id`, and `is_removed`, so the data columns are
nullable:

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
    is_removed      INTEGER NOT NULL DEFAULT 0
);
CREATE UNIQUE INDEX delta_packages_id ON delta_packages(id);
```

**Value tables** — new strings only, rowids allocated above the baseline's maximum:

```sql
CREATE TABLE delta_tags2 (
    rowid INTEGER PRIMARY KEY,
    tag   TEXT NOT NULL
);
CREATE UNIQUE INDEX delta_tags2_value ON delta_tags2(tag);
-- identical shape for delta_commands2 (command TEXT NOT NULL)
```

**Map tables** — the diff of `(value rowid, package rowid)` pairs:

```sql
CREATE TABLE delta_tags2_map (
    tag        INTEGER NOT NULL,   -- rowid into the merged tags2 (baseline or delta)
    package    INTEGER NOT NULL,   -- stable packages.rowid
    is_removed INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (tag, package)
) WITHOUT ROWID;
-- identical shape for delta_commands2_map
```

**System-reference tables** — `(value, package)` directly, with no separate value table:

```sql
CREATE TABLE delta_pfns2 (
    pfn        TEXT NOT NULL,
    package    INTEGER NOT NULL,
    is_removed INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (pfn, package)
) WITHOUT ROWID;
-- identical shape for delta_productcodes2, delta_norm_names2,
-- delta_norm_publishers2, delta_upgradecodes2
```

The `WITHOUT ROWID` primary keys are load-bearing for merge performance — see
[Performance of the row-level anti-join](#performance-of-the-row-level-anti-join).

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
- **Schema floor.** Deltas require a V2.1+ baseline. The delta records
  `MinimumBaselineSchemaVersion` so a client can reject an unusable combination cleanly.

> [!NOTE]
> The exact placement of the role and standalone markers — SQLite metadata table, MSIX manifest
> extension, or both — is not yet settled and should be resolved during implementation.

#### Fallback behavior

Every failure in the delta path falls back to downloading the full index. The delta is an
optimization, never a correctness dependency.

| Condition | Behavior |
|---|---|
| `delta.msix` not present at the source root | Full index |
| Delta downloaded but metadata unreadable or incomplete | Full index |
| Baseline named by the delta cannot be retrieved | Full index |
| Baseline GUID does not match the delta's | Re-acquire baseline once; then full index |
| Baseline schema version below the delta's floor | Full index |
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
  SELECT p.rowid, p.id, p.name, p.moniker, p.latest_version, p.arp_min_version, p.arp_max_version, p.hash
  FROM baseline.packages p
  WHERE p.id NOT IN (SELECT id FROM delta_packages);
```

The `NOT IN` is what makes an updated package resolve to its delta row rather than appearing
twice, and what makes a removed package disappear entirely — a tombstone is present in
`delta_packages`, so it suppresses the baseline row, but is itself filtered by `is_removed = 0`.

**Value tables** (`tags2`, `commands2`) — unconditional union, no filtering, because generation
guarantees the delta only ever contains strings the baseline does not have, at rowids above the
baseline's maximum.

```sql
CREATE TEMP VIEW tags2 AS
  SELECT rowid, tag FROM delta_tags2
  UNION ALL
  SELECT rowid, tag FROM baseline.tags2;
```

**Map and system-reference tables** — the delta holds **only the diff**: rows added since the
baseline (`is_removed = 0`) and tombstones for rows the baseline had that are now gone
(`is_removed = 1`). Associations that did not change are not represented at all. The baseline
row therefore passes through unless a tombstone specifically cancels it.

```sql
CREATE TEMP VIEW tags2_map AS
  SELECT tag, package FROM delta_tags2_map WHERE is_removed = 0
  UNION ALL
  SELECT b.tag, b.package FROM baseline.tags2_map b
  WHERE NOT EXISTS (
      SELECT 1 FROM delta_tags2_map d
      WHERE d.tag = b.tag AND d.package = b.package AND d.is_removed = 1)
    AND NOT EXISTS (
      SELECT 1 FROM delta_packages p
      WHERE p.rowid = b.package AND p.is_removed = 1);
```

> [!IMPORTANT]
> The suppression is **row-level** (`tag` *and* `package`), not package-level. A package that
> gains a single tag must keep all of its other tags, which are present only in the baseline and
> are never copied into the delta. Suppressing by package alone would silently drop every
> unchanged association of every updated package, degrading package correlation by
> PFN and product code.

The second `NOT EXISTS` drops associations belonging to deleted packages. Removing a package
does **not** write a tombstone per association — that would be pure waste — so the association
rows are instead suppressed by reference to the package tombstone. Without this, a value-first
lookup (`SELECT package FROM tags2_map WHERE tag = ?`) could return the rowid of a package that
no longer exists in the `packages` view.

The same shape applies to `commands2_map`, and to the system-reference tables (`pfns2`,
`productcodes2`, `norm_names2`, `norm_publishers2`, `upgradecodes2`), which carry
`(value, package)` directly with no separate value table.

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
  1. Open delta.db read-only as the main connection
  2. ATTACH baseline.db AS baseline
  3. Validate the baseline's GUID matches the one recorded in the delta metadata
  4. Create the TEMP VIEWs, shadowing every real table name
  5. Construct the normal V2 interface
     → its state detection finds `packages`, concludes the index is in
       post-PrepareForPackaging state, and all reads proceed unchanged
```

This is exposed as an additional construction entry point on the existing index type
(`OpenWithBaseline`) rather than a new type. The resulting object is an ordinary index as far
as every consumer is concerned.

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
`PrepareForPackaging`:

```cpp
enum class Property
{
    PackageUpdateTrackingBaseTime,
    IntermediateFileOutputPath,
    DeltaBaselineIndexPath,             // new
    DeltaBaselineRelativeStoragePath,   // new
    DeltaBaselinePackageVersion,        // new
    DeltaOutputPath,                    // new
};
```

**WinGetUtil C API** — the index creation tooling is C#, so the properties and the new entry
points are projected through `WinGetUtil.dll`:

```c
WINGET_UTIL_API WinGetSQLiteIndexMarkAsBaseline(
    WINGET_SQLITE_INDEX_HANDLE index);
```

with matching additions to the `WinGetSQLiteIndexProperty` enum, and corresponding `IWinGetFactory`
/ `IWinGetSQLiteIndex` members in the C# interop layer.

### Client integration

The pre-indexed package source factory gains delta awareness:

- A `delta.msix` location alongside the existing `source2.msix` / `source.msix` candidates.
- Baseline acquisition driven by the delta's metadata, reusing the existing download and trust
  validation helpers.
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
  and the reason affinity is checked explicitly rather than inferred.
- **Baseline location is service-controlled data.** The baseline relative path travels inside
  the delta, which means a compromised delta could name an arbitrary path. The path is resolved
  strictly relative to the source root already configured for that source, and the resulting
  package is subject to the same signature and trust validation as any other. It cannot be used
  to reach a different origin.
- **Attack surface.** Two packages are acquired instead of one, but both through the same
  validated path. The delta database itself is only ever opened read-only.
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
- **The V2.0 → V2.1 bump is additive.** It adds an `is_removed` column with a default to an
  internal tracking table that is dropped before packaging, so it is not visible in a published
  index. Published V2.1 indexes remain readable by V2.0-aware clients.
- **Delta-aware clients against a non-delta source** find no `delta.msix` and fall back
  immediately, so a source that has not adopted delta publishing works unchanged.
- **A V2.1 client against an older baseline** is prevented by the `MinimumBaselineSchemaVersion`
  floor recorded in the delta.
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
changed. This deserves an explicit comment at the assignment site and a test that fails loudly.

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
