---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-17
last updated: 2026-06-17
issue id: 2824
---

# Parallel Installation

For [#2824](https://github.com/microsoft/winget-cli/issues/2824).

## Abstract

Enable parallel package installation in WinGet when multiple packages are being installed or upgraded simultaneously. This applies to `winget install` with multiple packages, `winget upgrade --all`, `winget import`, and `winget configure` flows. Only MSIX packages can be installed concurrently (up to 6, the platform limit) because the MSIX deployment engine provides platform-level guarantees for concurrent operation. All Win32 installers (MSI, EXE, Inno, NSIS, Burn, etc.) are serialized — they offer no parallel safety guarantees and may interact with shared system state in unpredictable ways. Downloads are always parallelized regardless of installer type.

## Inspiration

When users run operations that install multiple packages — `winget upgrade --all`, `winget import`, or `winget configure` — packages are installed sequentially today. For a workstation with 30 packages to upgrade, this can take 30+ minutes even though most time is spent waiting on downloads and independent installations.

Key observations:
- MSIX packages can be deployed concurrently (the deployment engine handles this natively, with a platform limit of ~6 concurrent operations)
- MSI packages use the global `_MSIExecute` mutex — only one MSI can install at a time system-wide
- All other Win32 installers (EXE, Inno, NSIS, Burn) offer no parallel guarantees — they may modify shared system state (PATH, registry, shared DLLs, file associations) in ways that conflict when run concurrently
- Downloads are always safe to parallelize regardless of installer type
- Configuration flows (`winget configure`) with multiple `Microsoft.WinGet/Package` resources are inherently independent unless dependency-linked
- The `winget import` flow processes a package list sequentially today with no architectural reason to do so for the download phase and MSIX deployment phase

Related: [#225](https://github.com/microsoft/winget-cli/issues/225) (parallel downloads), [#6274](https://github.com/microsoft/winget-cli/issues/6274) (concurrent downloading).

## Solution Design

### Architecture Overview

```
┌─────────────────────────────────────────────┐
│ Multi-Package Operation                     │
│ (upgrade --all / import / configure)        │
└──────────────────┬──────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────┐
│ Package Scheduler                           │
│ ┌─────────────────────────────────────────┐ │
│ │ Dependency Graph Builder                │ │
│ │ - Explicit dependencies (manifest)      │ │
│ │ - Implicit constraints (installer type) │ │
│ └─────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────┐ │
│ │ Installer Type Classifier               │ │
│ │ - MSIX → concurrent pool               │ │
│ │ - All Win32 → serialized queue         │ │
│ └─────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────┐ │
│ │ Execution Engine                        │ │
│ │ - MSIX pool (max 6 concurrent)          │ │
│ │ - Win32 serialized queue (1 at a time)  │ │
│ │ - Download pool (always parallel)       │ │
│ └─────────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
```

### Execution Model

The scheduler maintains two execution lanes:

1. **Download Lane** — All package downloads run concurrently (up to `maxConcurrentDownloads`, default 5). Downloads complete into a staging area before installation begins.
2. **MSIX Installation Lane** — MSIX packages are deployed concurrently (up to 6, matching the platform limit). The MSIX deployment engine provides platform-level isolation guarantees.
3. **Win32 Serialized Lane** — All non-MSIX packages (MSI, EXE, Inno, NSIS, Burn, Wix) run one at a time. Win32 installers have no platform-level isolation — they may interact with the global `_MSIExecute` mutex, shared registry keys, PATH, file associations, or other system state in ways that are unsafe to overlap.

**Phased execution for a multi-package operation:**

```
Phase 1: Download all packages (parallel, up to maxConcurrentDownloads)
Phase 2: Install
  ├── MSIX pool: up to 6 concurrent deployments
  └── Win32 queue: all non-MSIX installers, one at a time
Phase 3: Post-install verification (parallel)
```

> [!IMPORTANT]
> Downloads and MSIX installations can overlap — an MSIX install can begin as soon as its download completes, even while other downloads are still in progress. The Win32 serialized queue also starts as soon as its first package finishes downloading. The phases are logical, not strictly sequential.

### Installer Type Classification

The scheduler determines which lane a package uses:

| Installer Type | Lane | Rationale |
|---------------|------|-----------|
| `msix` / `appx` | MSIX pool | Platform provides concurrent deployment guarantees |
| `msi` | Win32 serialized | Global `_MSIExecute` mutex |
| `wix` | Win32 serialized | Produces MSI (same mutex) |
| `burn` | Win32 serialized | Burn bundles chain MSI packages |
| `exe` | Win32 serialized | No parallel guarantees; may use MSI internally or modify shared state |
| `inno` | Win32 serialized | No platform-level isolation; modifies registry, PATH, file associations |
| `nullsoft` | Win32 serialized | No platform-level isolation; modifies shared system state |
| `portable` | Win32 serialized | Lightweight, but may still write to shared locations (PATH, shims) |
| `zip` | Win32 serialized | Extraction may target shared directories |

> [!NOTE]
> While some Win32 installer types (Inno, NSIS, portable) may appear safe for concurrency in many cases, they offer no platform-level guarantees. Two Inno installers running simultaneously may both try to modify PATH, write to the same registry keys, or register the same file associations — leading to race conditions, corrupted state, or silent failures. The conservative approach of serializing all Win32 installers ensures system integrity.

### MSI Mutex Detection and Retry

Even in the serialized queue, Windows Update or another system process may hold the `_MSIExecute` mutex when the next MSI package is ready to install:

1. Before starting an MSI installation, check the mutex state
2. If held, wait with exponential backoff (1s, 2s, 4s, ..., max 60s)
3. After max retries (configurable, default 5), report the conflict:
   ```
   ⚠️ Microsoft.Package failed: Windows Installer is busy (another installation is in progress).
       Retry with 'winget install Microsoft.Package' after the current installation completes.
   ```
4. Continue with other packages — one failure does not abort the batch

### Dependency-Aware Ordering

Some packages depend on others (e.g., a VS Code extension requires VS Code). The scheduler builds a dependency graph:

1. **Explicit dependencies** — Manifest `Dependencies` field (PackageDependencies)
2. **Implicit ordering** — Source management resources before package resources (in configure flows)
3. **User-specified ordering** — `winget configure` respects `dependsOn` directives in configuration files

Packages with unsatisfied dependencies wait in a ready queue until their dependencies complete successfully.

### Flows Affected

#### `winget upgrade --all`

```
> winget upgrade --all
Upgrading 12 packages...

Downloading:  [████████████████████] 12/12 complete
Installing:
  [MSIX pool]   ████████░░ 4/5
  [Win32 queue] ██████░░░░ 3/7  (current: Microsoft.DotNet.SDK.8)
  [Completed]   ████       4

Completed: 11/12 succeeded, 1 failed (see below)
Failed:
  Adobe.Acrobat.Reader - Windows Installer busy (retry later)
```

#### `winget import`

```
> winget import -i packages.json
Importing 25 packages...

Downloading:  [████████████████████] 25/25 complete
Installing:
  [MSIX pool]   ████████████░░░░ 6/8
  [Win32 queue] ████████░░░░░░░░ 4/17  (current: Git.Git)
  [Completed]   ████████████     12

All 25 packages installed successfully in 6m 15s (vs. ~18m sequential).
```

#### `winget configure`

Configuration files with multiple `Microsoft.WinGet/Package` resources are parallelized by the WinGet configuration engine:

```yaml
resources:
  - resource: Microsoft.WinGet/Package
    directives:
      description: Install Git
    settings:
      id: Git.Git       # → Win32 serialized (exe)
  - resource: Microsoft.WinGet/Package
    directives:
      description: Install VS Code
    settings:
      id: Microsoft.VisualStudioCode  # → Win32 serialized (inno)
  - resource: Microsoft.WinGet/Package
    directives:
      description: Install PowerToys
    settings:
      id: Microsoft.PowerToys  # → MSIX pool
```

The `Microsoft.WinGet/PackageList` resource (from the DSC spec [#6289](https://github.com/microsoft/winget-cli/issues/6289)) natively supports parallel installation — it schedules MSIX packages concurrently while serializing all Win32 installers.

For standalone `Microsoft.WinGet/Package` resources in a configuration file, WinGet coordinates with the DSC runtime's own parallelism model. Resources without `dependsOn` relationships are candidates for concurrent execution, but the WinGet engine still enforces the MSIX-concurrent / Win32-serialized discipline internally.

#### `winget install` with multiple packages

```
> winget install Git.Git Microsoft.PowerToys Mozilla.Firefox Python.Python.3.12
Installing 4 packages...

Downloading:  [████████████████████] 4/4 complete
Installing:
  [MSIX pool]   Microsoft.PowerToys (deploying...)
  [Win32 queue] Git.Git (running installer...)
  [Queue]       Mozilla.Firefox (waiting for Win32 slot)
  [Queue]       Python.Python.3.12 (waiting for Win32 slot)

All 4 packages installed successfully in 2m 10s.
```

### Progress Reporting

Multi-package progress uses a consolidated display:

```
Downloading (3/12):
  Git.Git             [████████░░░░] 67%  12.3 MB/s
  Microsoft.PowerToys [██████████░░] 83%   8.1 MB/s
  Python.Python.3.12  [██░░░░░░░░░░] 15%  15.0 MB/s

Installing (2 active, 4 queued):
  ✓ Microsoft.VisualStudioCode 1.92.0
  ● Microsoft.PowerToys 0.82.0 (deploying MSIX...)
  ● Git.Git 2.45.1 (running installer...)
  ○ Python.Python.3.12 (queued - Win32)
  ○ Node.js (queued - Win32)
  ○ Docker.DockerDesktop (queued - MSIX)
```

In non-VT mode (`--no-vt`) or non-interactive mode, progress falls back to line-by-line status updates.

### Error Handling and Partial Failure

Multi-package operations use a **continue-on-failure** model:

1. A failed package does not abort other installations
2. Packages that depend on a failed package are marked as `skipped (dependency failed)`
3. Final summary reports succeeded, failed, and skipped counts
4. Exit code is non-zero if any package failed
5. `--fail-fast` argument stops all parallel work on first failure (opt-in for CI scenarios)

**Retry logic for mutex contention:**
- If a Win32 installation fails with an error indicating the `_MSIExecute` mutex is held by another process (e.g., Windows Update), the package is retried after a backoff delay
- This handles the common case of Windows Update blocking MSI installations during Patch Tuesday

### Concurrency Limits

| Setting | Default | Min | Max | Description |
|---------|---------|-----|-----|-------------|
| `maxConcurrentDownloads` | 5 | 1 | 20 | Parallel downloads |
| `maxConcurrentMsix` | 6 | 1 | 6 | Concurrent MSIX deployments (platform max) |

> [!WARNING]
> The MSIX deployment platform limit of 6 is a Windows constraint. Setting `maxConcurrentMsix` above 6 has no effect — deployments will queue at the platform level. All Win32 installers are always serialized — there is no concurrency setting for them because no safe concurrency level exists.

### CLI Arguments

| Argument | Commands | Description |
|----------|----------|-------------|
| `--sequential` | install, upgrade, import | Force sequential installation (disable all parallelism including MSIX) |
| `--fail-fast` | install, upgrade, import | Abort all on first failure |

### Settings

```json
{
  "installBehavior": {
    "parallelInstallation": {
      "enabled": true,
      "maxConcurrentDownloads": 5,
      "maxConcurrentMsix": 6,
      "retryOnMutexContention": true,
      "maxMutexRetries": 5
    }
  }
}
```

| Setting | CLI Argument | GPO Policy | Interaction |
|---------|-------------|------------|-------------|
| `enabled` | `--sequential` (disables) | `ParallelInstallation` | GPO wins; `--sequential` overrides setting |
| `maxConcurrentDownloads` | N/A | `MaxConcurrentDownloads` | GPO wins |
| `maxConcurrentMsix` | N/A | `MaxConcurrentMsix` | GPO wins |

### Group Policy

| Policy | Type | Default | Description |
|--------|------|---------|-------------|
| `ParallelInstallation` | Enum | Enabled | `Enabled` / `Disabled` / `DownloadOnly` |
| `MaxConcurrentDownloads` | Int | 5 | Maximum parallel downloads |
| `MaxConcurrentMsix` | Int | 6 | Maximum concurrent MSIX deployments (capped at platform limit of 6) |

`DownloadOnly` allows parallel downloads but forces sequential installation of everything (including MSIX) — a maximally conservative option for enterprises.

### COM API Surface

```idl
interface IInstallOptions2 : IInstallOptions
{
    Boolean AllowParallelInstallation { get; set; };
    UInt32 MaxConcurrentOperations { get; set; };
}

interface IBatchInstallResult
{
    IVectorView<PackageInstallResult> Results { get; };
    UInt32 SucceededCount { get; };
    UInt32 FailedCount { get; };
    UInt32 SkippedCount { get; };
    TimeSpan TotalDuration { get; };
}

interface IPackageInstallResult2 : IPackageInstallResult
{
    InstallExecutionLane ExecutionLane { get; };  // MsixParallel, Win32Serialized, Skipped
    TimeSpan WaitDuration { get; };              // Time spent waiting for slot
}

enum InstallExecutionLane { MsixParallel, Win32Serialized, Skipped };
```

### PowerShell Cmdlets

```powershell
# Upgrade all with parallel (default)
Update-WinGetPackage -All

# Force sequential
Update-WinGetPackage -All -Sequential

# Import with parallel
Import-WinGetPackage -ImportFile packages.json

# Install multiple
Install-WinGetPackage -Id "Git.Git", "Microsoft.PowerToys", "Mozilla.Firefox"

# Access batch result
$result = Update-WinGetPackage -All
$result.Failed | Format-Table Id, ErrorMessage, ExecutionLane
```

The `-Sequential` switch maps to `--sequential`.

### Cross-Repository Impact

- **winget-cli** — Scheduler, execution engine, progress reporting, settings, GPO, COM API
- **winget-pkgs** — No schema changes required (parallel behavior is purely client-side based on existing `InstallerType` field)
- **winget-create** — No changes required
- **winget-cli-restsource** — No changes required
- **winget-dsc** — `Microsoft.WinGet/PackageList` uses parallel scheduling internally

### Schema Version

No manifest schema changes required. Parallel installation uses the existing `InstallerType` field to classify packages. This is purely a client-side behavioral improvement.

### Validation Pipeline Impact

No pipeline changes. Parallel installation is a client behavior that uses existing manifest data.

## UI/UX Design

### Default multi-package output (VT enabled):

```
> winget upgrade --all

Upgrading 8 packages...

⬇ Downloading    ████████████████████  8/8 complete (47.2 MB)

⚙ Installing
  ✓ Microsoft.PowerToys 0.82.0            [msix, 3.2s]
  ✓ Microsoft.WindowsTerminal 1.21.0      [msix, 4.1s]
  ✓ Mozilla.Firefox 128.0                 [exe, 12.3s]
  ● Git.Git 2.45.1                        [exe, installing...]
  ○ Python.Python.3.12.4                  [exe, queued]
  ○ Node.js 20.15.0                       [msi, queued]
  ✓ 7zip.7zip 24.07                       [msi, 8.1s]
  ✓ VideoLAN.VLC 3.0.21                   [exe, 6.4s]

✓ 8/8 packages upgraded successfully in 1m 48s
  (estimated sequential time: 4m 30s — 2.5× faster)
```

### Non-interactive / `--disable-interactivity`:

```
Downloading 8 packages...
Downloaded: Microsoft.PowerToys (12.3 MB)
Downloaded: Git.Git (54.1 MB)
...
Installing: Microsoft.PowerToys [msix, parallel]
Installing: Microsoft.WindowsTerminal [msix, parallel]
Installed: Microsoft.PowerToys 0.82.0 (3.2s)
Installing: Git.Git [exe, serialized]
Installed: Microsoft.WindowsTerminal 1.21.0 (4.1s)
Installing: Mozilla.Firefox [exe, serialized]
...
Complete: 8/8 succeeded in 1m 48s
```

### `--no-vt` mode:

Simple line-by-line without progress bars or cursor movement.

### Error with partial failure:

```
> winget upgrade --all

Upgrading 8 packages...

⬇ Downloading    ████████████████████  8/8 complete

⚙ Installing
  ✓ Microsoft.PowerToys 0.82.0            [msix, 3.2s]
  ✓ Mozilla.Firefox 128.0                 [inno, 12.3s]
  ✗ Adobe.Acrobat.Reader 24.002           [exe, FAILED: Windows Installer busy]
  ✓ Git.Git 2.45.1                        [exe, 18.4s]
  ...

⚠ 7/8 succeeded, 1 failed:
  Adobe.Acrobat.Reader — Windows Installer mutex held by another process.
  Run 'winget upgrade Adobe.Acrobat.Reader' to retry.
```

## Capabilities

### Accessibility

- Progress display uses text status markers (✓ ● ○ ✗) alongside descriptions for screen readers
- Non-VT fallback ensures full accessibility without virtual terminal sequences
- JSON output mode (`--output json`) provides machine-readable progress events
- Summary line always includes text count (not just visual indicators)

### Security

- No elevation changes — each installer inherits the same security context as sequential installation
- Parallel execution does not bypass SmartScreen or hash verification (each package is validated independently)
- Win32 serialization protects system integrity — no risk of concurrent installers corrupting shared state
- GPO `ParallelInstallation: Disabled` gives enterprises a full opt-out

### Reliability

- **Win32 serialization** — All non-MSIX installers run one at a time; no risk of shared-state corruption from concurrent Win32 installations
- **MSI mutex respect** — Even within serialized execution, retries handle external mutex holders (Windows Update)
- **Retry on contention** — Automatic retry when mutex is held by external process
- **Continue-on-failure** — One package failing doesn't abort others (unless `--fail-fast`)
- **MSIX platform limit respected** — Never exceeds the 6-concurrent deployment platform constraint
- **Conservative by design** — Only MSIX gets concurrency because only MSIX has platform-level isolation guarantees

### Compatibility

- **Fully backwards compatible** — Default behavior includes parallel MSIX, but `--sequential` restores fully sequential behavior
- No manifest schema changes — works with all existing manifests using the `InstallerType` field
- Single-package operations (`winget install Foo`) are unaffected (nothing to parallelize)
- COM API: `AllowParallelInstallation` defaults to `true` for batch operations — existing single-package callers see no change
- GPO default is `Enabled` — no enterprise action needed unless they want to restrict

### Performance, Power, and Efficiency

- **Primary benefit** — Multi-package operations complete faster by overlapping MSIX deployments with each other and with the Win32 serialized queue
- **Download parallelism** — Network bandwidth utilized more efficiently (downloads ahead of install)
- **MSIX concurrency** — Platform-native concurrent deployment, up to 6 simultaneous
- **Realistic speedup** — For a mixed workload (some MSIX, mostly Win32), expect 1.5-2.5× improvement due to download overlap and concurrent MSIX. Pure-Win32 workloads benefit only from parallel downloads.
- **Memory** — Each concurrent MSIX deployment has low overhead. Win32 queue runs one process at a time.

## Potential Issues

1. **Limited speedup for Win32-heavy workloads** — If most packages in an upgrade are Win32 installers, the only parallelism is downloads + any concurrent MSIX. The speed improvement comes from eliminating download wait time and overlapping MSIX with the Win32 queue.
2. **Resource contention from concurrent MSIX** — Multiple simultaneous MSIX deployments may saturate disk I/O on slow storage (eMMC, mechanical drives). Mitigation: `maxConcurrentMsix` is configurable.
3. **Progress reporting complexity** — Displaying multiple concurrent operations (MSIX deploying while Win32 installs serially) is a UX challenge. Mitigation: graceful degradation for narrow terminals and non-VT environments.
4. **Windows Update conflicts** — Windows Update may acquire the MSI mutex at any time, blocking even serialized Win32 installations. Mitigation: retry logic with backoff.
5. **Reboot requirements** — If a Win32 installer requires reboot, subsequent installers may fail or produce unpredictable results. Mitigation: detect pending-reboot state, pause remaining packages with a warning.
6. **MSIX deployment failures under load** — While the platform supports 6 concurrent, some systems may have issues at full concurrency. Mitigation: configurable limit, retry on transient deployment failures.

## Future Considerations

- **Intelligent scheduling** — Learn from historical install times to optimize queue ordering (start longest Win32 installs first while MSIX deploys in parallel)
- **Pre-download during interactive prompts** — Begin downloading the next package while waiting for user confirmation on the current one
- **Win32 installer isolation** — If Windows introduces process-level isolation for Win32 installers (container-based installation), parallel Win32 could become safe in the future
- **Distributed installation** — For fleet management, schedule package downloads during off-peak hours and install during maintenance windows
- **Resume on reboot** — If a reboot interrupts a batch, resume remaining packages after restart

## Resources

- Issue: https://github.com/microsoft/winget-cli/issues/2824
- Parallel downloads: https://github.com/microsoft/winget-cli/issues/225
- Concurrent downloading: https://github.com/microsoft/winget-cli/issues/6274
- MSIX deployment concurrency: https://learn.microsoft.com/windows/msix/desktop/managing-your-msix-deployment-overview
- Windows Installer mutex: https://learn.microsoft.com/windows/win32/msi/mutex-objects
- Side-by-side packages (related): https://github.com/microsoft/winget-cli/issues/2129
