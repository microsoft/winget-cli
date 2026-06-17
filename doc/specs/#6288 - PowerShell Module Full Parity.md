---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-17
last updated: 2026-06-17
issue id: 6288
---

# PowerShell Module Full Parity with WinGet CLI

For [#6288](https://github.com/microsoft/winget-cli/issues/6288).

## Abstract

The WinGet PowerShell module (`Microsoft.WinGet.Client`) must achieve full functional parity with the WinGet CLI. Several CLI capabilities have no PowerShell equivalent today, blocking adoption by enterprise automation workflows that operate exclusively in PowerShell contexts (Intune, SCCM, Azure Automation, DSC).

## Inspiration

IT decision makers evaluating WinGet for large-scale deployment cite incomplete PowerShell support as a blocking issue. Enterprise automation tooling operates in PowerShell-only contexts where shelling out to `winget.exe` is unreliable — especially in SYSTEM context ([#5991](https://github.com/microsoft/winget-cli/issues/5991)) — or architecturally inappropriate.

Key gaps identified by enterprise customers:

- No PowerShell equivalent for `winget configure` operations (test, validate, show)
- Missing cmdlets for pin management (`winget pin add/remove/list/reset`)
- No cmdlet for `winget download` ([#658](https://github.com/microsoft/winget-cli/issues/658) added CLI support)
- Missing `winget repair` support in the module ([#148](https://github.com/microsoft/winget-cli/issues/148) added CLI support)
- No PowerShell equivalent for `winget settings` management
- Limited source management beyond basic add/remove
- COM API cannot activate in elevated/SYSTEM context ([#6042](https://github.com/microsoft/winget-cli/issues/6042))

## Solution Design

### Phase 1: Core Cmdlet Gaps

Add the following cmdlets to `Microsoft.WinGet.Client`:

| CLI Command | Proposed Cmdlet | Verb-Noun Justification |
|-------------|----------------|------------------------|
| `winget configure test` | `Test-WinGetConfiguration` | `Test` = validate state compliance |
| `winget configure validate` | `Assert-WinGetConfiguration` | `Assert` = validate schema/syntax |
| `winget configure show` | `Get-WinGetConfigurationDetails` | `Get` = retrieve information |
| `winget download` | `Save-WinGetPackage` | `Save` = download without install (PS convention) |
| `winget repair` | `Repair-WinGetPackage` | `Repair` = fix broken install |
| `winget pin add` | `Add-WinGetPin` | `Add` = create resource |
| `winget pin remove` | `Remove-WinGetPin` | `Remove` = delete resource |
| `winget pin list` | `Get-WinGetPin` | `Get` = list/retrieve |
| `winget pin reset` | `Reset-WinGetPin` | `Reset` = restore defaults |
| `winget settings export` | `Export-WinGetSettings` | `Export` = serialize to file |
| `winget settings set` | `Set-WinGetSetting` | `Set` = modify value |

### Phase 2: Enhanced Source Management

Extend `Get-WinGetSource`, `Add-WinGetSource`, `Remove-WinGetSource`:

- Full parameter parity (custom headers, certificates, authentication tokens)
- `Update-WinGetSource` for refresh operations (equivalent to `winget source update`)
- Source group policy awareness — report when sources are policy-managed with a `PolicyManaged` property

### Phase 3: SYSTEM Context Support

Address the fundamental blocker of WinGet operating in SYSTEM context:

- The COM API (`Microsoft.Management.Deployment`) must properly initialize in SYSTEM context
- Package operations must work without a logged-in user session
- Source management must work in SYSTEM context for Intune and SCCM deployments
- Related to [#6042](https://github.com/microsoft/winget-cli/issues/6042) and [#5991](https://github.com/microsoft/winget-cli/issues/5991)

### Architecture

All new cmdlets use the existing COM API rather than wrapping the CLI executable:

```
┌──────────────────────────────────┐
│ PowerShell Cmdlet                │
│ (Microsoft.WinGet.Client)        │
└──────────────┬───────────────────┘
               │ COM Interop
               ▼
┌──────────────────────────────────┐
│ Microsoft.Management.Deployment  │
│ (WinRT COM API)                  │
└──────────────┬───────────────────┘
               │
               ▼
┌──────────────────────────────────┐
│ WindowsPackageManagerServer      │
│ (WinGetDev.exe / winget.exe)     │
└──────────────────────────────────┘
```

Benefits:
- Proper error propagation as PowerShell exceptions (`ErrorRecord` with `HRESULT`)
- Structured output objects (not parsed text)
- Cancellation support via `Ctrl+C` / `StopProcessing()`
- Progress reporting via `WriteProgress`
- Pipeline support for batch operations

### COM API Surface Additions

New interfaces needed in `Microsoft.Management.Deployment`:

```idl
// Pin management
interface IPackagePinManager
{
    IAsyncOperation<IVectorView<PackagePin>> GetPinsAsync();
    IAsyncOperationWithProgress<PinResult, PinProgress> AddPinAsync(AddPinOptions options);
    IAsyncOperationWithProgress<PinResult, PinProgress> RemovePinAsync(RemovePinOptions options);
    IAsyncOperationWithProgress<PinResult, PinProgress> ResetPinsAsync(ResetPinOptions options);
}

// Settings management
interface ISettingsManager
{
    String ExportSettings();
    void SetSetting(String settingPath, String value);
    String GetSetting(String settingPath);
}
```

### PowerShell Cmdlet Details

#### Save-WinGetPackage

```powershell
Save-WinGetPackage
    [-Id] <String>
    [-Version <String>]
    [-Source <String>]
    [-Architecture <Architecture>]
    [-InstallerType <InstallerType>]
    [-Scope <PackageScope>]
    [-OutputDirectory <String>]  # Required
    [-AcceptSourceAgreements]
    [-AcceptPackageAgreements]
    [-Force]
```

Output: `WinGetDownloadResult` object with `InstallerPath`, `InstallerType`, `Sha256`, `Status`.

#### Test-WinGetConfiguration

```powershell
Test-WinGetConfiguration
    [-File] <String>
    [-AcceptConfigurationAgreements]
```

Output: `WinGetConfigurationTestResult` object with per-resource compliance status:

```powershell
$result = Test-WinGetConfiguration -File .\config.dsc.yaml
$result.UnitResults | Format-Table ResourceName, State, InDesiredState
```

#### Add-WinGetPin / Remove-WinGetPin / Get-WinGetPin

```powershell
# Pin to prevent upgrades
Add-WinGetPin -Id "Microsoft.VisualStudioCode" [-Version <String>] [-Blocking]

# List pins
Get-WinGetPin [-Id <String>]

# Remove pin
Remove-WinGetPin -Id "Microsoft.VisualStudioCode"

# Reset all pins
Reset-WinGetPin [-Force]
```

### Settings Changes

No new settings required. Existing settings continue to apply to both CLI and PowerShell module operations identically.

### Validation Pipeline Impact

No impact on `winget-pkgs` validation. Validation already uses the COM API for non-interactive operation.

## UI/UX Design

### Interactive Mode

```powershell
PS> Save-WinGetPackage -Id "Git.Git" -OutputDirectory "C:\Packages"

Id       : Git.Git
Version  : 2.45.1
Path     : C:\Packages\Git-2.45.1-64-bit.exe
Sha256   : abc123...
Status   : Ok
```

### Pipeline Support

```powershell
# Download multiple packages
"Git.Git", "Python.Python.3.12" | ForEach-Object {
    Save-WinGetPackage -Id $_ -OutputDirectory "C:\Packages"
}

# Test configuration and filter failures
$result = Test-WinGetConfiguration -File .\config.dsc.yaml
$result.UnitResults | Where-Object { -not $_.InDesiredState }
```

### Non-Interactive / Automation

```powershell
# Intune remediation script
$pins = Get-WinGetPin
if ($pins | Where-Object { $_.Id -eq "Microsoft.Edge" -and $_.IsBlocking }) {
    Write-Output "Edge is pinned - compliant"
    exit 0
} else {
    Add-WinGetPin -Id "Microsoft.Edge" -Blocking
    exit 1
}
```

### -Force Parameter Behavior

All new cmdlets support `-Force` to suppress confirmation prompts, consistent with existing cmdlets.

## Capabilities

### Accessibility

No direct impact — PowerShell modules inherit the accessibility of the terminal/host application. All output is text-based and works with screen readers through the PowerShell host.

### Security

- SYSTEM context support must not bypass UAC for operations that require elevation in user context
- COM API activation in SYSTEM context validates caller identity
- Settings cmdlets respect Group Policy overrides (throw terminating error when attempting to modify policy-managed settings)
- Token/credential parameters are `SecureString` where applicable

### Reliability

- Eliminates the unreliable pattern of `Start-Process winget.exe` with text parsing
- COM API provides structured error codes via `HRESULT` mapped to PowerShell `ErrorRecord`
- Proper cancellation via `StopProcessing()` prevents orphaned installer processes
- Retry logic for transient COM activation failures

### Compatibility

- No breaking changes to existing cmdlets
- New cmdlets follow established naming patterns in the module
- Minimum PowerShell version remains 7.2+ (consistent with current module)
- Windows PowerShell 5.1 is NOT supported (consistent with current module)

### Performance, Power, and Efficiency

- COM API calls avoid process startup overhead (no `winget.exe` launch per operation)
- Pipelining enables batch operations without repeated COM activation
- `Save-WinGetPackage` supports parallel downloads when used with `ForEach-Object -Parallel`

## Potential Issues

1. **SYSTEM context COM activation** — The WinGet COM server (`WindowsPackageManagerServer`) may require architectural changes to support activation outside a user session. This is the highest-risk item.
2. **Configuration cmdlets depend on DSC runtime** — `Test-WinGetConfiguration` requires the DSC v3 runtime to be available. If unavailable, the cmdlet should throw a clear error with installation guidance.
3. **Backwards compatibility testing** — Enterprises may pin to older module versions. New cmdlets must not destabilize existing functionality via assembly loading changes.
4. **COM interface versioning** — New interfaces must be additive to avoid breaking existing COM consumers. Use new interface IIDs.
5. **Scope parameter interactions** — `Save-WinGetPackage` with `-Scope machine` in a non-elevated session should fail early with a clear error, not after download.

## Future Considerations

- Full parity enables DSC resources to delegate directly to PowerShell cmdlets
- Opens the path for WinGet to be fully operable as a PowerShell-native tool without the CLI
- Enables richer Intune remediation scripts and proactive remediations
- Azure Automation runbooks can manage packages across fleets
- Potential for a `WinGet` PowerShell drive provider (`Get-ChildItem WinGet:\installed\`)

## Resources

- Current module: https://www.powershellgallery.com/packages/Microsoft.WinGet.Client
- COM API source: `src/Microsoft.Management.Deployment/`
- PowerShell module source: `src/PowerShell/Microsoft.WinGet.Client/`
- SYSTEM context issue: https://github.com/microsoft/winget-cli/issues/5991
- Elevated COM issue: https://github.com/microsoft/winget-cli/issues/6042
- Scope parameter issue: https://github.com/microsoft/winget-cli/issues/4787
