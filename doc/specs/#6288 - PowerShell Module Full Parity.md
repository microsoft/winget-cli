---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-17
last updated: 2026-06-23
issue id: 6288
---

# PowerShell Module Full Parity with WinGet CLI

For [#6288](https://github.com/microsoft/winget-cli/issues/6288).

## Abstract

WinGet ships two PowerShell modules — `Microsoft.WinGet.Client` (package management) and
`Microsoft.WinGet.Configuration` (configuration operations). The modules already cover the large
majority of the CLI surface. This spec establishes an accurate baseline of what exists today and
scopes the remaining work required to reach full parity.

After auditing the shipping source (`src/PowerShell/`) and the published modules, the true parity
work falls into three categories:

1. **Genuine command gaps** — capabilities with no cmdlet equivalent today: **pin management** and
   **package list import/export**.
2. **Reliability bugs** — the module already supports elevated and SYSTEM contexts through dedicated
   activation paths; where these fail it is a defect to fix, not a missing feature
   ([#5991](https://github.com/microsoft/winget-cli/issues/5991),
   [#6042](https://github.com/microsoft/winget-cli/issues/6042)).
3. **Enhancement backlog** — smaller, issue-tracked improvements to existing cmdlets (richer detail
   output, update-all, multiple IDs, cancellation, `-WhatIf`/`-Debug`).

## Inspiration

IT decision makers evaluating WinGet for large-scale deployment cite incomplete PowerShell support
as a blocker. Enterprise automation operates in PowerShell-only contexts (Intune, SCCM, Azure
Automation, DSC) where shelling out to `winget.exe` and parsing text is unreliable. Closing the
remaining gaps and hardening elevated/SYSTEM execution lets these workflows stay native.

## Current State — What Already Exists

This inventory is the corrected baseline. Earlier drafts of this spec proposed cmdlets that already
ship; those are recorded here as **existing** so the parity gap is not overstated.

### Microsoft.WinGet.Client (package management)

| CLI command | Existing cmdlet | Notes |
|-------------|-----------------|-------|
| `winget install` | `Install-WinGetPackage` | Supports `-Scope` ([#4787](https://github.com/microsoft/winget-cli/issues/4787)) |
| `winget upgrade` | `Update-WinGetPackage` | Supports `-Scope` |
| `winget uninstall` | `Uninstall-WinGetPackage` | |
| `winget search` | `Find-WinGetPackage` | |
| `winget list` | `Get-WinGetPackage` | |
| `winget download` | `Export-WinGetPackage` | Downloads a package and its dependencies; `-Scope` supported |
| `winget repair` | `Repair-WinGetPackage` | |
| `winget source add/remove/reset/list` | `Add/Remove/Reset/Get-WinGetSource` | |
| `winget settings` (user file) | `Get/Set/Test-WinGetUserSetting` | Read, write, and validate the user settings JSON |
| `winget settings` (admin) | `Get-WinGetSetting`, `Enable/Disable-WinGetSetting` | Admin/toggle settings |
| `winget --version` | `Get-WinGetVersion` | |
| (bootstrap) | `Assert/Repair-WinGetPackageManager` | Verify/repair the WinGet installation itself |

### Microsoft.WinGet.Configuration (configuration operations)

| CLI command | Existing cmdlet | Notes |
|-------------|-----------------|-------|
| `winget configure` / `apply` | `Invoke-WinGetConfiguration`, `Start/Complete-WinGetConfiguration` | Synchronous and async apply |
| `winget configure test` | `Test-WinGetConfiguration` | Tests system state against the set |
| `winget configure show` | `Get-WinGetConfigurationDetails` | |
| `winget configure list` | `Get-WinGetConfiguration` | History via `-...FromHistory` parameter sets |
| `winget configure abort` | `Stop-WinGetConfiguration` | |
| `winget configure delete` | `Remove-WinGetConfigurationHistory` | |
| (open/confirm/convert) | `Get/Confirm-WinGetConfiguration`, `ConvertTo-WinGetConfigurationYaml` | |

> Note: `Microsoft.WinGet.Configuration` communicates with the **`Microsoft.Management.Configuration`**
> WinRT API (and its processor), **not** `Microsoft.Management.Deployment`. The two modules do not
> share a COM surface.

## Solution Design

### Gap 1 — Pin Management (Microsoft.WinGet.Client)

There is no PowerShell equivalent for `winget pin add/remove/list/reset` today. This is the largest
genuine parity gap.

| CLI command | Proposed cmdlet | Verb justification |
|-------------|-----------------|--------------------|
| `winget pin add` | `Add-WinGetPin` | `Add` = create resource |
| `winget pin list` | `Get-WinGetPin` | `Get` = list/retrieve |
| `winget pin remove` | `Remove-WinGetPin` | `Remove` = delete resource |
| `winget pin reset` | `Reset-WinGetPin` | `Reset` = restore defaults |

Pins are modeled with a **`PinType`** rather than a boolean, so the output can describe pinning,
blocking, and gating pins as the feature evolves (per review feedback):

```powershell
# Pinning pin (excluded from `upgrade --all`, still upgradeable explicitly)
Add-WinGetPin -Id "Microsoft.VisualStudioCode"

# Blocking pin (no upgrades at all)
Add-WinGetPin -Id "Microsoft.VisualStudioCode" -Blocking

# Gating pin (pin to a version range, e.g. 1.2.*)
Add-WinGetPin -Id "Microsoft.VisualStudioCode" -Version "1.2.*"

Get-WinGetPin [-Id <String>]      # returns objects with Id, Version, Source, PinType
Remove-WinGetPin -Id "Microsoft.VisualStudioCode"
Reset-WinGetPin [-Force]
```

`Get-WinGetPin` output object:

| Property | Description |
|----------|-------------|
| `Id` | Package identifier |
| `Source` | Source the pin applies to |
| `Version` | Pinned version or range (gating pins) |
| `PinType` | `Pinning`, `Blocking`, or `Gating` |

Automation can branch on `PinType` directly:

```powershell
if (Get-WinGetPin -Id "Microsoft.Edge" | Where-Object PinType -eq 'Blocking') {
    Write-Output "Edge is blocked from upgrade - compliant"
}
```

### Gap 2 — Package List Import / Export (Microsoft.WinGet.Client)

`winget export` (serialize installed packages to a JSON list) and `winget import` (install from such
a list) have no cmdlet equivalent ([#5041](https://github.com/microsoft/winget-cli/issues/5041)).
This is distinct from `Export-WinGetPackage`, which downloads installers.

To avoid colliding with the existing `Export-WinGetPackage` (download) noun, the list operations use
a dedicated noun:

```powershell
# Serialize installed packages to a WinGet import/export JSON file
Export-WinGetPackageList -OutputFile "C:\state\packages.json"
    [-Source <String>] [-IncludeVersions]

# Install everything described by an export file
Import-WinGetPackageList -File "C:\state\packages.json"
    [-IgnoreUnavailable] [-IgnoreVersions] [-AcceptPackageAgreements] [-AcceptSourceAgreements]
```

`Import-WinGetPackageList` returns one result object per package describing install status, so callers
can detect partial failures in automation.

> Naming is provisional. If the team prefers, these can ship as parameters on existing cmdlets;
> the requirement is that the installed-list import/export workflow becomes available without
> shelling out to `winget.exe`.

### Reliability — Elevated and SYSTEM Context

The module already activates the COM API through a custom path for elevated callers and an in-proc
path for SYSTEM; this is not a missing feature. The parity work here is to **fix the defects** so
these paths are dependable for Intune/SCCM:

- SYSTEM context regression where `Get-WinGetPackage` exits immediately
  ([#5991](https://github.com/microsoft/winget-cli/issues/5991),
  [#4820](https://github.com/microsoft/winget-cli/issues/4820)).
- Elevated activation failing to see the packaged COM registration
  ([#6042](https://github.com/microsoft/winget-cli/issues/6042),
  [#5369](https://github.com/microsoft/winget-cli/issues/5369),
  [#5635](https://github.com/microsoft/winget-cli/issues/5635)).

Note that successful *package operation* in a session-less context also depends on installer
behavior, which WinGet does not own; per-installer results cannot be guaranteed.

### Enhancement Backlog (existing cmdlets)

These are issue-tracked refinements, not new commands. They are listed for completeness and can be
scheduled independently of the gaps above:

- Richer package detail output / `list --details`
  ([#6055](https://github.com/microsoft/winget-cli/issues/6055),
  [#6144](https://github.com/microsoft/winget-cli/issues/6144),
  [#5129](https://github.com/microsoft/winget-cli/issues/5129))
- Update-all for `Update-WinGetPackage` ([#5495](https://github.com/microsoft/winget-cli/issues/5495))
- Multiple package IDs in a single call ([#5094](https://github.com/microsoft/winget-cli/issues/5094))
- `Ctrl+C` cancellation of download/install ([#4961](https://github.com/microsoft/winget-cli/issues/4961))
- `-WhatIf` honored by `Update-WinGetPackage` ([#4622](https://github.com/microsoft/winget-cli/issues/4622))
- `-Debug` support ([#4697](https://github.com/microsoft/winget-cli/issues/4697))

### Architecture

Both modules use the existing WinRT COM APIs rather than wrapping `winget.exe`. They target
**different** APIs:

```
┌──────────────────────────────────┐  ┌──────────────────────────────────┐
│ Microsoft.WinGet.Client          │  │ Microsoft.WinGet.Configuration   │
│ (package management cmdlets)     │  │ (configuration cmdlets)          │
└──────────────┬───────────────────┘  └──────────────┬───────────────────┘
               │                                      │
               ▼                                      ▼
┌──────────────────────────────────┐  ┌──────────────────────────────────┐
│ Microsoft.Management.Deployment  │  │ Microsoft.Management.Configuration│
│ (package manager WinRT COM API)  │  │ (configuration WinRT COM API)    │
└──────────────────────────────────┘  └──────────────────────────────────┘
```

Using the COM APIs gives error propagation as PowerShell `ErrorRecord`s (with `HRESULT`), structured
output objects, `StopProcessing()` cancellation, `WriteProgress`, and pipeline support.

> Detailed COM interface design (IIDs, async signatures) is intentionally out of scope for this spec.
> Pin management requires a small addition to the `Microsoft.Management.Deployment` surface; the
> concrete interface shape is an implementation decision made during development. This spec defines
> the cmdlet inputs and outputs only.

### Settings Changes

No new settings required. Existing settings apply identically to CLI and module operations.

### Validation Pipeline Impact

No impact on `winget-pkgs` validation. Validation already uses the COM API for non-interactive
operation.

## UI/UX Design

### Pin management

```powershell
PS> Add-WinGetPin -Id "Git.Git" -Version "2.45.*"
PS> Get-WinGetPin

Id       Version  Source  PinType
--       -------  ------  -------
Git.Git  2.45.*   winget  Gating
```

### Import / export workflow

```powershell
# Capture a machine's installed packages, then reproduce it elsewhere
Export-WinGetPackageList -OutputFile "\\share\golden.json" -IncludeVersions
Import-WinGetPackageList -File "\\share\golden.json" -IgnoreUnavailable
```

### -Force behavior

New cmdlets support `-Force` to suppress confirmation prompts, consistent with existing cmdlets.

## Capabilities

### Accessibility

No direct impact — modules inherit the accessibility of the PowerShell host; all output is text-based
and screen-reader compatible.

### Security

- Elevated/SYSTEM hardening must not bypass UAC for operations that require elevation in user context.
- Pin and import/export operations respect Group Policy; policy-managed state yields a terminating
  error when a caller attempts to modify it.
- Credential/token parameters on source cmdlets remain `SecureString` where applicable.

### Reliability

- Eliminates `Start-Process winget.exe` text parsing for the newly covered commands.
- COM `HRESULT`s map to PowerShell `ErrorRecord`s.
- `StopProcessing()` cancellation prevents orphaned installer processes.

### Compatibility

- No breaking changes to existing cmdlets; new cmdlets follow established naming patterns.
- Minimum PowerShell version remains 7.2+; Windows PowerShell 5.1 is not supported, consistent with
  the current modules.

### Performance, Power, and Efficiency

- COM calls avoid per-operation `winget.exe` startup.
- Pipelining enables batch operations without repeated activation.

## Potential Issues

1. **Elevated/SYSTEM defects are environmental** — reproducing and fixing
   [#5991](https://github.com/microsoft/winget-cli/issues/5991)/[#6042](https://github.com/microsoft/winget-cli/issues/6042)
   requires testing across Intune/SCCM/session-less hosts; installer behavior in those contexts is
   outside WinGet's control.
2. **Pin COM surface** — pin management needs an additive `Microsoft.Management.Deployment` interface
   (new IIDs) to avoid breaking existing COM consumers.
3. **Import/export noun choice** — `Export-WinGetPackage` already means *download*. The list
   operations must use a distinct noun (or parameters) to avoid ambiguity.
4. **Backwards compatibility** — new cmdlets must not destabilize existing functionality via assembly
   loading changes.

## Future Considerations

- Full parity enables DSC resources to delegate directly to PowerShell cmdlets.
- Richer Intune remediation and proactive remediation scripts.
- Azure Automation runbooks managing packages across fleets.
- Potential `WinGet:` PowerShell drive provider (e.g. `Get-ChildItem WinGet:\installed\`).

## Resources

- Client module: https://www.powershellgallery.com/packages/Microsoft.WinGet.Client
- Configuration module: https://www.powershellgallery.com/packages/Microsoft.WinGet.Configuration
- Package manager COM API source: `src/Microsoft.Management.Deployment/`
- Configuration COM API source: `src/Microsoft.Management.Configuration/`
- Client module source: `src/PowerShell/Microsoft.WinGet.Client/`
- Configuration module source: `src/PowerShell/Microsoft.WinGet.Configuration/`
- Pin management: [#6288](https://github.com/microsoft/winget-cli/issues/6288)
- Import/Export package list: [#5041](https://github.com/microsoft/winget-cli/issues/5041)
- SYSTEM context: [#5991](https://github.com/microsoft/winget-cli/issues/5991)
- Elevated COM: [#6042](https://github.com/microsoft/winget-cli/issues/6042)
- Scope parameter (already shipped): [#4787](https://github.com/microsoft/winget-cli/issues/4787)
- Configuration module improvements PR: [#6190](https://github.com/microsoft/winget-cli/pull/6190)
