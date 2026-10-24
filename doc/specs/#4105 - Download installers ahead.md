---
author: Nils Peper nilspep
created on: 2026-09-26
last updated: 2026-10-04
issue id: 4105
---

# Download Installers Ahead

For [#4105](https://github.com/microsoft/winget-cli/issues/4105)

## Abstract

In multi-package operations, WinGet downloads each installer only after the previous package has been installed, so the connection is idle during every installation. This spec adds an experimental feature, `downloadAhead`, that downloads the installers of upcoming packages in the background while the current package installs. Installation itself is unchanged: packages are still installed one at a time and in their original order.

## Inspiration

- [#4105](https://github.com/microsoft/winget-cli/issues/4105) asks for downloads to run in parallel with installations, in particular for `winget upgrade --all`.
- Review feedback on the parallel installation spec in [#6295](https://github.com/microsoft/winget-cli/pull/6295) pointed out that parallel downloads could saturate the network and delay the first installation, suggested downloading packages one by one while others install, and asked for a limit on any concurrency.
- Measurements (see [Performance](#performance-power-and-efficiency)) showed that the time saved comes from overlapping downloads with the serial installations, not from downloading several files at once.

## Solution Design

### Setting

The behavior is an experimental feature and is disabled by default.

```json
"experimentalFeatures": {
    "downloadAhead": true
},
```

The feature is controlled by the existing experimental features group policy. There is no CLI argument while the feature is experimental.

### Client behavior

`ProcessMultiplePackages`, the workflow task behind the affected commands, starts a download pipeline when the feature is enabled and the list contains at least two packages that can be downloaded ahead.

- One background thread downloads installers in package order, one at a time.
- At most three installers are downloaded ahead of the package being installed. Each one is released when its package is taken for installation.
- Each background download runs in its own sub-context: `CheckForExistingInstaller`, `DownloadInstallerFile`, `VerifyInstallerHash`. Its paths and hashes are not copied into the installation context. The normal `DownloadInstaller` step still runs before each installation, so it finds the cached file, verifies it and downloads it again if it is missing or changed.
- Output of a background download is buffered and written before the output of its package, so it never interleaves with an installer that is running.
- A failed download terminates only its own package, with the same result as a failed foreground download.
- When a package has package dependencies, background downloads pause, after the running one finishes, until the dependencies have been processed. A dependency can therefore never download into the same directory as a background download.
- Cancelling the operation while it waits for a download cancels that download. When the operation ends early, for example after Ctrl+C or a failure in a stop-on-failure list, the running download is cancelled and installers downloaded ahead are not installed. Stopping a background download does not change the result or termination reason recorded for the operation.

Packages keep the normal download flow in these cases:

| Case | Reason |
|---|---|
| Microsoft Store, MSIX (including streaming) | Deployment is platform-managed |
| Authenticated downloads | Authentication may require interaction |
| `--ignore-security-hash` | Cache lookup rejects mismatched hashes |
| `--download-only`, `winget download` | Download-only flow is unchanged |
| Same `Id.Version` more than once in one list | The packages share a download directory |
| Operations with a single package | Nothing to overlap with |

### Flow behavior matrix

| Command | Behavior with the feature enabled |
|---|---|
| `winget install <several packages>` | Downloads ahead |
| `winget upgrade --all`, `winget upgrade <several packages>` | Downloads ahead |
| `winget import` | Downloads ahead |
| `winget install <one package>` | Unchanged; a list of two or more package dependencies downloads ahead |
| `winget download`, `--download-only` | Unchanged |
| `--dependencies-only` | Unchanged |
| Interactive installs (`-i`) | Downloads ahead; installers still run one at a time |
| `--silent`, `--disable-interactivity` | Downloads ahead; background downloads never prompt |
| `--no-vt` | Downloads ahead; the status line and buffered output are plain text |
| `winget configure` (`WinGetPackage` resource) | Unchanged; each resource installs one package |

### COM API and PowerShell

There is no API surface change. COM calls and the PowerShell module use the same workflows. An `InstallPackageAsync` call, and therefore `Install-WinGetPackage` and `Update-WinGetPackage`, benefits only when the package has two or more package dependencies. `-Force` has no effect on the feature.

### Manifest schema and validation

No manifest or schema change, so winget-create, winget-cli-restsource and winget-pkgs need no updates. The winget-pkgs validation pipeline is not affected, because it installs one package at a time.

## UI/UX Design

A new status line is shown once when downloading ahead starts. Buffered download output appears with the package identifier before the package is installed:

```
Downloading upcoming installers in the background while packages install...
(1/3) Found Git [Git.Git] Version 2.55.0.3
...
Successfully installed

[Microsoft.VisualStudioCode]
Successfully verified installer hash
(2/3) Found Microsoft Visual Studio Code [Microsoft.VisualStudioCode] Version 1.139.0
...
```

Diagnostic logs record when downloading ahead starts, each background download with its result, packages that fall back to the foreground, pauses for dependencies and cancellations.

## Capabilities

### Accessibility

No change; the added output uses the existing reporter.

### Security

Installers downloaded ahead go through the same download, hash verification and security checks as foreground downloads. The installation step validates the cached file again immediately before use. Authenticated downloads and hash overrides are excluded.

### Reliability

Installations remain serial and in order, and the Windows Installer mutex and the WinGet install lock are unchanged. Background downloads log from another thread, which relies on serialized `FileLogger` writes ([#6571](https://github.com/microsoft/winget-cli/pull/6571), merged).

### Compatibility

With the feature disabled, the workflow is unchanged. With it enabled, the order of installation and the result codes are unchanged.

### Performance, Power, and Efficiency

Median wall-clock time for one `install` of all packages from a clean state, on Azure VMs with x64 Release builds and about 600 runs in total. Only runs on the same VM are compared. The download ahead column was measured with one download at a time and unbounded look-ahead; the final design with at most three installers ahead was within 2% of it in a separate series on the same VMs.

| Packages | Link | Current flow | Download ahead | Saved |
|---|---|---|---|---|
| 8 mixed (706 MB) | 20 Mbit/s | 507 s | 335 s | −34% |
| 8 mixed (706 MB) | 100 Mbit/s | 287 s | 212 s | −26% |
| 10 small (111 MB) | 20 Mbit/s | 83 s | 57 s | −31% |
| 10 small (111 MB) | 100 Mbit/s | 47 s | 33 s | −29% |
| 3 large (966 MB) | 20 Mbit/s | 581 s | 465 s | −20% |
| 3 large (966 MB) | 100 Mbit/s | 225 s | 171 s | −24% |
| 12 real packages from publisher CDNs (811 MB) | Internet | 270 s | 216 s | −20% |

The gain grows with download time and is smallest when installation dominates, for example −5% for the large set at 1 Gbit/s. With the feature disabled, times were within ±2% of the current flow.

- **Concurrent downloads.** Allowing 2, 4 or 8 concurrent downloads was never faster than one at a time. It delayed the next installation and kept up to all installers on disk, for example 921 MB for the large set.
- **Look-ahead depth.** With a single installer ahead, the connection was idle during long installations, which was up to 19% slower than unbounded look-ahead. Three installers ahead was within 2% of unbounded look-ahead in every scenario.
- **Resource use.** At most one additional connection, and additional disk use bounded by three installers.

## Potential Issues

- Installers downloaded ahead are wasted when an operation stops early.
- A cancellation through the COM API that arrives while an installer is running reaches the background download only after that installation returns. CLI cancellation (Ctrl+C) reaches it immediately.
- Disk usage in `%TEMP%` rises by up to three installers.

## Future considerations

- Promote the feature to a setting, possibly with a configurable depth and a CLI argument.
- Download the dependencies of upcoming packages ahead.
- Run installers that do not use Windows Installer in parallel. This is a separate and riskier change.

## Resources

- [#4105](https://github.com/microsoft/winget-cli/issues/4105): feature request and [measurement summary](https://github.com/microsoft/winget-cli/issues/4105#issuecomment-5909448940)
- [#6295](https://github.com/microsoft/winget-cli/pull/6295): parallel installation spec
- [#6571](https://github.com/microsoft/winget-cli/pull/6571): thread-safe `FileLogger`
- The implementation will document the feature in the experimental features section of `doc/Settings.md`.
