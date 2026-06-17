---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-17
last updated: 2026-06-17
issue id: 2824
---

# Parallel Installation

For [#2824](https://github.com/microsoft/winget-cli/issues/2824).

## Abstract

Enable parallel package installation in WinGet when multiple packages are being installed or upgraded simultaneously. This applies to `winget install` with multiple packages, `winget upgrade --all`, `winget import`, and `winget configure` flows. Packages are classified by installer type safety: MSIX packages can be installed concurrently (up to a configurable limit), while MSI/EXE packages that rely on the Windows Installer mutex are serialized. A dependency-aware scheduler maximizes throughput while respecting installer constraints.

## Inspiration

When users run operations that install multiple packages — `winget upgrade --all`, `winget import`, or `winget configure` — packages are installed sequentially today. For a workstation with 30 packages to upgrade, this can take 30+ minutes even though most time is spent waiting on downloads and independent installations.

Key observations:
- MSIX packages can be deployed concurrently (the deployment engine handles this natively, with a platform limit of ~6 concurrent operations)
- MSI packages use the global `_MSIExecute` mutex — only one MSI can install at a time system-wide
- EXE installers vary: some use MSI internally (same mutex), some are fully standalone
- Downloads are always safe to parallelize regardless of installer type
- Configuration flows (`winget configure`) with multiple `Microsoft.WinGet/Package` resources are inherently independent unless dependency-linked
- The `winget import` flow processes a package list sequentially today with no architectural reason to do so

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
│ │ - Implicit constraints (MSI mutex)      │ │
│ └─────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────┐ │
│ │ Installer Type Classifier               │ │
│ │ - MSIX → concurrent pool               │ │
│ │ - MSI → serialized queue               │ │
│ │ - EXE (MSI-backed) → serialized queue  │ │
│ │ - EXE (standalone) → concurrent pool   │ │
│ └─────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────┐ │
│ │ Execution Engine                        │ │
│ │ - MSIX pool (max N concurrent)          │ │
│ │ - Serialized queue (1 at a time)        │ │
│ │ - Download pool (always parallel)       │ │
│ └─────────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
```

### Execution Model

The scheduler maintains three execution lanes:

1. **Download Lane** — All package downloads run concurrently (up to `maxConcurrentDownloads`, default 5). Downloads complete into a staging area before installation begins.
2. **MSIX Installation Lane** — MSIX packages are deployed concurrently (up to `maxConcurrentMsix`, default 6, matching the platform limit). No mutex contention.
3. **Serialized Installation Lane** — MSI and MSI-backed EXE packages run one at a time. This respects the global `_MSIExecute` mutex and avoids the system-wide conflict where Windows Update can interrupt an MSI installation.

**Phased execution for a multi-package operation:**

```
Phase 1: Download all packages (parallel, up to maxConcurrentDownloads)
Phase 2: Install (concurrent where safe)
  ├── MSIX pool: up to 6 concurrent deployments
  └── Serialized queue: MSI/EXE one at a time
Phase 3: Post-install verification (parallel)
```

> [!IMPORTANT]
> Downloads and MSIX installations can overlap — an MSIX install can begin as soon as its download completes, even while other downloads are still in progress. The serialized queue also starts as soon as its first package finishes downloading. The phases are logical, not strictly sequential.

### Installer Type Classification

The scheduler determines which lane a package uses:

| Installer Type | Lane | Rationale |
|---------------|------|-----------|
| `msix` / `appx` | MSIX pool | Platform supports concurrent deployment |
| `msi` | Serialized | Global `_MSIExecute` mutex |
| `wix` | Serialized | Produces MSI (same mutex) |
| `exe` (default) | Serialized | Conservative — may use MSI internally |
| `exe` with `SafeForParallel: true` | MSIX pool | Manifest-declared safe (see below) |
| `inno` | MSIX pool | Inno Setup is self-contained, no MSI mutex |
| `nullsoft` | MSIX pool | NSIS is self-contained, no MSI mutex |
| `burn` | Serialized | Burn bundles often chain MSI packages |
| `portable` | MSIX pool | Just a file copy — always safe |
| `zip` | MSIX pool | Extraction — always safe |

### Manifest Extension: SafeForParallel

For EXE installers that are known to be safe for concurrent execution (no shared mutex, no shared state), add an optional field:

```yaml
Installers:
  - Architecture: x64
    InstallerType: exe
    InstallerUrl: https://example.com/app-setup.exe
    SafeForParallel: true  # Declares this installer is safe for concurrent execution
```

This is opt-in and conservative. Without the flag, EXE installers default to the serialized queue.

> [!NOTE]
> `SafeForParallel` is a statement about the installer's behavior, not a guarantee. If a manifest incorrectly declares an MSI-wrapping EXE as safe, the worst case is a failed installation (the MSI mutex will cause one to fail), not data corruption. WinGet will retry the failed package in the serialized queue.

### MSI Mutex Detection and Retry

Even in the serialized queue, Windows Update or another system process may hold the `_MSIExecute` mutex:

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
  [Serialized]  ██████░░░░ 3/5  (current: Microsoft.DotNet.SDK.8)
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
  [Parallel]    ██████████████░░ 14/16 (MSIX: 8, Inno: 4, NSIS: 2)
  [Serialized]  ████████░░░░░░░░ 4/9  (current: Git.Git)
  [Completed]   ████████████████ 16

All 25 packages installed successfully in 4m 32s (vs. ~18m sequential).
```

#### `winget configure`

Configuration files with multiple `Microsoft.WinGet/Package` resources are parallelized by the WinGet configuration engine:

```yaml
resources:
  - resource: Microsoft.WinGet/Package
    directives:
      description: Install Git
    settings:
      id: Git.Git       # → serialized (exe, burn-based)
  - resource: Microsoft.WinGet/Package
    directives:
      description: Install VS Code
    settings:
      id: Microsoft.VisualStudioCode  # → serialized (inno? → actually parallel)
  - resource: Microsoft.WinGet/Package
    directives:
      description: Install PowerToys
    settings:
      id: Microsoft.PowerToys  # → MSIX pool
```

The `Microsoft.WinGet/PackageList` resource (from the DSC spec [#6289](https://github.com/microsoft/winget-cli/issues/6289)) natively supports parallel installation — it classifies and schedules all packages in the list.

For standalone `Microsoft.WinGet/Package` resources in a configuration file, WinGet coordinates with the DSC runtime's own parallelism model. Resources without `dependsOn` relationships are candidates for concurrent execution.

#### `winget install` with multiple packages

```
> winget install Git.Git Microsoft.PowerToys Mozilla.Firefox Python.Python.3.12
Installing 4 packages...

Downloading:  [████████████████████] 4/4 complete
Installing:
  [Parallel]    Mozilla.Firefox (inno), Microsoft.PowerToys (msix)
  [Serialized]  Git.Git (exe/burn)
  [Queue]       Python.Python.3.12 (exe) — waiting for serialized slot

All 4 packages installed successfully in 1m 45s.
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
  ○ Python.Python.3.12 (queued - serialized)
  ○ Node.js (queued - serialized)
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
- If an EXE/MSI installation fails with an error indicating mutex contention, and `SafeForParallel` was set, the package is automatically retried in the serialized queue
- This self-healing behavior means incorrect `SafeForParallel` declarations don't permanently fail

### Concurrency Limits

| Setting | Default | Min | Max | Description |
|---------|---------|-----|-----|-------------|
| `maxConcurrentDownloads` | 5 | 1 | 20 | Parallel downloads |
| `maxConcurrentMsix` | 6 | 1 | 6 | Concurrent MSIX deployments (platform max) |
| `maxConcurrentInstallers` | 3 | 1 | 10 | Concurrent non-MSI parallel installers (Inno, NSIS, portable) |

> [!WARNING]
> The MSIX deployment platform limit of 6 is a Windows constraint. Setting `maxConcurrentMsix` above 6 has no effect — deployments will queue at the platform level.

### CLI Arguments

| Argument | Commands | Description |
|----------|----------|-------------|
| `--sequential` | install, upgrade, import | Force sequential installation (disable parallelism) |
| `--fail-fast` | install, upgrade, import | Abort all on first failure |
| `--max-concurrent` | install, upgrade, import | Override total concurrent operations limit |

### Settings

```json
{
  "installBehavior": {
    "parallelInstallation": {
      "enabled": true,
      "maxConcurrentDownloads": 5,
      "maxConcurrentMsix": 6,
      "maxConcurrentInstallers": 3,
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
| `maxConcurrentInstallers` | `--max-concurrent` | `MaxConcurrentInstallers` | GPO sets max; arg can go lower |

### Group Policy

| Policy | Type | Default | Description |
|--------|------|---------|-------------|
| `ParallelInstallation` | Enum | Enabled | `Enabled` / `Disabled` / `DownloadOnly` |
| `MaxConcurrentDownloads` | Int | 5 | Maximum parallel downloads |
| `MaxConcurrentInstallers` | Int | 3 | Maximum concurrent non-MSI installers |
| `AllowParallelMsi` | Bool | False | **Future/experimental** — attempt concurrent MSI (very risky) |

`DownloadOnly` allows parallel downloads but forces sequential installation — a conservative middle ground for enterprises concerned about installer interactions.

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
    InstallExecutionLane ExecutionLane { get; };  // Parallel, Serialized, Retry
    TimeSpan WaitDuration { get; };              // Time spent waiting for slot
}

enum InstallExecutionLane { Parallel, Serialized, Retry, Skipped };
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

The `-Sequential` switch maps to `--sequential`. `-MaxConcurrent` maps to `--max-concurrent`.

### Cross-Repository Impact

- **winget-cli** — Scheduler, execution engine, progress reporting, settings, GPO, COM API
- **winget-pkgs** — `SafeForParallel` field validation; community can annotate known-safe installers
- **winget-create** — Support `SafeForParallel` field in manifest authoring
- **winget-cli-restsource** — Schema update for `SafeForParallel` field
- **winget-dsc** — `Microsoft.WinGet/PackageList` uses parallel scheduling internally

### Schema Version

Requires manifest schema version 1.29.0 for the `SafeForParallel` field. Parallel installation itself does not require schema changes — it works with existing manifests using installer type classification.

### Validation Pipeline Impact

- `SafeForParallel` is a new optional boolean field on installer entries
- Validation: if `InstallerType` is `msi` or `burn` and `SafeForParallel: true`, reject as invalid (these types are inherently unsafe for parallel execution)
- No other pipeline changes — parallel installation is a client behavior

## UI/UX Design

### Default multi-package output (VT enabled):

```
> winget upgrade --all

Upgrading 8 packages...

⬇ Downloading    ████████████████████  8/8 complete (47.2 MB)

⚙ Installing
  ✓ Microsoft.PowerToys 0.82.0            [msix, 3.2s]
  ✓ Microsoft.WindowsTerminal 1.21.0      [msix, 4.1s]
  ✓ Mozilla.Firefox 128.0                 [inno, 12.3s]
  ● Git.Git 2.45.1                        [exe, installing...]
  ○ Python.Python.3.12.4                  [exe, queued]
  ○ Node.js 20.15.0                       [msi, queued]
  ✓ 7zip.7zip 24.07                       [msi, 8.1s]
  ✓ VideoLAN.VLC 3.0.21                   [nsis, 6.4s]

✓ 8/8 packages upgraded successfully in 1m 12s
  (estimated sequential time: 4m 30s — 3.8× faster)
```

### Non-interactive / `--disable-interactivity`:

```
Downloading 8 packages...
Downloaded: Microsoft.PowerToys (12.3 MB)
Downloaded: Git.Git (54.1 MB)
...
Installing: Microsoft.PowerToys [msix, parallel]
Installing: Mozilla.Firefox [inno, parallel]
Installed: Microsoft.PowerToys 0.82.0 (3.2s)
Installing: Git.Git [exe, serialized]
...
Complete: 8/8 succeeded in 1m 12s
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
- MSI serialization protects system integrity (the `_MSIExecute` mutex exists for safety — we respect it)
- `SafeForParallel` cannot be set on MSI/Burn types (enforced at validation)
- GPO `ParallelInstallation: Disabled` gives enterprises an opt-out if they discover interaction issues

### Reliability

- **MSI mutex respect** — Never attempts concurrent MSI installation; avoids the system-instability risk
- **Retry on contention** — Automatic retry when mutex is held by external process (Windows Update)
- **Continue-on-failure** — One package failing doesn't abort others (unless `--fail-fast`)
- **Self-healing misclassification** — If `SafeForParallel` is wrong and causes failure, retry in serialized queue
- **Conservative defaults** — EXE installers default to serialized unless explicitly declared safe
- **MSIX platform limit respected** — Never exceeds the 6-concurrent deployment platform constraint

### Compatibility

- **Fully backwards compatible** — Default behavior is parallel, but `--sequential` restores old behavior
- Older manifests without `SafeForParallel` work unchanged — classification uses `InstallerType` field
- Single-package operations (`winget install Foo`) are unaffected (nothing to parallelize)
- COM API: `AllowParallelInstallation` defaults to `true` for batch operations — existing single-package callers see no change
- GPO default is `Enabled` — no enterprise action needed unless they want to restrict

### Performance, Power, and Efficiency

- **Primary benefit** — Multi-package operations complete significantly faster (3-5× for typical workloads)
- **Download parallelism** — Network bandwidth utilized more efficiently
- **MSIX concurrency** — Platform-native concurrent deployment
- **CPU/disk** — Multiple installers may compete for disk I/O. The `maxConcurrentInstallers` limit prevents excessive resource contention.
- **Memory** — Each concurrent installer spawns a separate process. With default limits (3 concurrent + 6 MSIX), worst case is ~9 simultaneous installer processes.

## Potential Issues

1. **Installer interaction** — Two "standalone" EXE installers might modify shared system state (PATH, registry keys) simultaneously. Mitigation: conservative defaults (serialized unless declared safe), retry logic.
2. **Resource contention** — Many concurrent installers may saturate disk I/O, causing all of them to slow down. Mitigation: configurable limits, and downloads are staged (pre-downloaded before install begins).
3. **Progress reporting complexity** — Displaying multiple concurrent operations cleanly is a UX challenge, especially in constrained terminal widths. Mitigation: graceful degradation for narrow terminals and non-VT environments.
4. **Windows Update conflicts** — Windows Update may acquire the MSI mutex at any time. The retry logic handles this, but users may still see intermittent failures during Patch Tuesday.
5. **`SafeForParallel` accuracy** — Community may incorrectly mark installers as safe. Mitigation: auto-retry in serialized queue, and validation blocks MSI/Burn from being marked safe.
6. **System instability from too many concurrent installers** — Resource-constrained devices (2GB RAM, slow disk) may struggle with 6 MSIX + 3 EXE concurrent. Mitigation: configurable limits, and enterprises can reduce via GPO.
7. **Reboot requirements** — If an installer requires reboot, subsequent installers may fail or produce unpredictable results. Mitigation: detect pending-reboot state, queue remaining packages with a warning.

## Future Considerations

- **Intelligent scheduling** — Learn from historical install times to optimize queue ordering (start longest installs first)
- **Pre-download during interactive prompts** — Begin downloading the next package while waiting for user confirmation on the current one
- **AllowParallelMsi** (experimental) — For known-safe MSI packages that don't actually use shared MSI state, consider allowing parallel MSI with very careful validation
- **Distributed installation** — For fleet management, schedule package downloads during off-peak hours and install during maintenance windows
- **Resume on reboot** — If a reboot interrupts a batch, resume remaining packages after restart

## Resources

- Issue: https://github.com/microsoft/winget-cli/issues/2824
- Parallel downloads: https://github.com/microsoft/winget-cli/issues/225
- Concurrent downloading: https://github.com/microsoft/winget-cli/issues/6274
- MSIX deployment concurrency: https://learn.microsoft.com/windows/msix/desktop/managing-your-msix-deployment-overview
- Windows Installer mutex: https://learn.microsoft.com/windows/win32/msi/mutex-objects
- Side-by-side packages (related): https://github.com/microsoft/winget-cli/issues/2129
