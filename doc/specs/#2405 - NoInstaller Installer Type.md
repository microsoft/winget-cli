---
author: Kaleb Luedtke @Trenly, GitHub Copilot <Copilot>
created on: 2026-06-15
last updated: 2026-06-15
issue id: 2405
---

# NoInstaller Installer Type

For [#2405](https://github.com/microsoft/winget-cli/issues/2405)

## Abstract

This specification describes support for a `NoInstaller` installer type, enabling manifests that declare packages without downloadable installers. This allows WinGet to recognize and correlate already-installed software through Apps & Features (ARP) entries, handle removed or tombstoned packages, and provide actionable user messaging when installers are unavailable. The `NoInstaller` type is intended for system components, pre-installed software, out-of-band distributions, and version tombstones.

## Inspiration

Several scenarios require WinGet to recognize packages that have no installer artifact available:

1. **System Components** — Windows built-in software, drivers, and OEM-installed applications often have no downloadable installer outside their original distribution channel.
2. **Pre-installed Software** — Recognizing applications that ship with the OS or hardware allows WinGet to correlate and manage them without installation.
3. **Out-of-Band Distributions** — Some publishers distribute software through non-WinGet channels (corporate systems, hardware partners, distribution agreements) and want WinGet to recognize these versions for correlation and upgrade tracking.
4. **Version Tombstones** — When a version is removed (CVE, publisher request, identifier change), a `NoInstaller` entry allows the repository to enforce removal tracking, prevent false matches, and communicate to users why installation is unavailable.
5. **Publisher Messaging** — Publishers need a way to inform users that an installer is temporarily or permanently unavailable, with actionable context (links, alternative sources, reasons).

[Issue #823](https://github.com/microsoft/winget-cli/issues/823) covers filtering by installer type; this feature complements that work by introducing a recognized installer type for non-installable packages. Additionally, [Issue #1900](https://github.com/microsoft/winget-cli/issues/1900) discusses version tombstones as a mechanism to handle removed versions, and this feature provides a means to implement that use case.

## Solution Design

### Installer Type Addition

Add `NoInstaller` as a new enum value in `InstallerTypeEnum`:

```cpp
enum class InstallerTypeEnum
{
    Unknown,
    Inno,
    Wix,
    Msi,
    Nullsoft,
    Zip,
    Msix,
    Exe,
    Burn,
    MSStore,
    Portable,
    Font,
    NoInstaller,  // New
};
```

### Manifest Schema Changes

#### Installer Type String Mapping

The string representation for `NoInstaller` in YAML/JSON manifests is `"NoInstaller"` (case-insensitive during parsing).

#### InstallerAvailabilityMessage Field

A new optional field `InstallerAvailabilityMessage` is added at the installer level, with maximum length of 512 characters. This field provides a human-readable explanation when an installer is unavailable.

```yaml
Installers:
  - Architecture: x64
    InstallerType: NoInstaller
    InstallerAvailabilityMessage: "This package is obsolete. Please use Example.Package.2 instead."
```

The field may also be specified at the root manifest level:

```yaml
InstallerAvailabilityMessage: "This version has been removed due to a security vulnerability"
Installers:
  - Architecture: x64
    InstallerType: NoInstaller
```

#### Field Constraints for NoInstaller Type

When `InstallerType` is `NoInstaller`, the following constraint applies:

| Field | Behavior | Rationale |
|-------|----------|-----------|
| **InstallerUrl** | MUST NOT be present | No installer to download |

All other installer fields may be optionally populated as appropriate for the package, including but not limited to:

- **InstallerSha256** — May be used for correlation/deduplication if the package has a known hash
- **ProductCode** — Useful for ARP correlation when available
- **PackageFamilyName** — Can be used for tracking purposes
- **Scope** — May be specified if relevant to the package context
- **InstallerSwitches** — May be retained for documentation or future use
- **InstallerSuccessCodes** — Optional for documentation
- **ExpectedReturnCodes** — Optional for documentation
- **InstallModes** — Optional for documentation
- **InstallerAvailabilityMessage** — Optional; user-facing explanation for unavailability
- **AppsAndFeaturesEntries** — Optional; for ARP correlation when the package has an existing entry
- **Architecture** — Required; must specify which architectures this applies to

### Manifest Validation

**Schema Validation** (winget-cli and winget-pkgs):

1. If `InstallerType` is `NoInstaller`, `InstallerUrl` must not be present. ✓
2. If `InstallerAvailabilityMessage` is present, `InstallerType` must be `NoInstaller`; otherwise, validation error.
3. If `InstallerAvailabilityMessage` is present, it must not be empty.

**Validation Pipeline (winget-pkgs)**:

For community repository submissions with `NoInstaller` installer type:

- A new validation rule `Validation-NoExecutables` (similar to existing validation rules) must be introduced.
- This rule should be auto-waivered, requiring Microsoft review for each `NoInstaller` submission.
- This ensures that community contributions using `NoInstaller` are reviewed to prevent misuse (e.g., publishers blocking inclusion for "corporate reasons").
- Legitimate use cases (system components, OEM software, official tombstones) receive appropriate waiver.

### Manifest Example

```yaml
PackageIdentifier: Example.Package
PackageVersion: 2.0.0
PackageLocale: en-US
Publisher: Example Corporation
PackageName: Example Package
License: Example License
ShortDescription: Example package description
LongDescription: This version is no longer available. Please upgrade to version 3.0.0.

Installers:
  - Architecture: x64
    InstallerType: NoInstaller
    InstallerAvailabilityMessage: "Version 2.0.0 has been removed due to a security vulnerability. Please download version 3.0.0 or later."

ManifestType: installer
ManifestVersion: 1.30.0
```

### CLI Behavior

#### Install Command

When `winget install` encounters a package where the selected installer has type `NoInstaller`:

1. Do not attempt to download or execute any installer.
2. If `InstallerAvailabilityMessage` is provided, display it to the user.
3. If `InstallerAvailabilityMessage` is not provided, display the generic message: `"The installer for this package is no longer available."`.
4. Terminate with exit code `APPINSTALLER_CLI_ERROR_INSTALLER_NOT_AVAILABLE` (0x8A150116).

#### Upgrade Command

When upgrading to a version with `NoInstaller` type:

1. If the latest version is `NoInstaller`, inform the user and terminate with appropriate error message.
2. If an intermediate version is `NoInstaller` but a newer installable version is available, skip the `NoInstaller` version and offer to upgrade to the newer installable version.
3. If all available versions have `NoInstaller` type, inform the user and terminate.

#### Show Command

The `show` command displays information about `NoInstaller` packages:

1. Display package metadata normally.
2. Do NOT display "Installer URL" information.
3. Display "Offline Distribution Supported: false" for `NoInstaller` packages.
4. Display the `InstallerAvailabilityMessage` if present.

#### Search Command

The `search` command includes `NoInstaller` packages in results. Future filtering by installer type ([#823](https://github.com/microsoft/winget-cli/issues/823)) may allow users to exclude non-installable packages, but this is out of scope for this specification.

#### Uninstall Command

The `uninstall` command is supported for `NoInstaller` packages through ARP (Apps & Features) correlation:

1. WinGet uses `ProductCode` and `AppsAndFeaturesEntries` from the manifest to correlate the package to an installed application in the Windows Registry.
2. If a match is found in ARP, the uninstall string from the ARP entry is executed, allowing WinGet to uninstall the package even though the installer was not downloaded by WinGet.
3. If no ARP entry is found, the user is informed that the package cannot be located on the system.
4. This allows users to manage pre-installed software, system components, and out-of-band distributed software through WinGet.

Example:
```
$ winget uninstall ExamplePackage
Found Example Package [Example.Package] v2.0.0
Uninstalling...
Successfully uninstalled
```

### COM API (Microsoft.Management.Deployment)

No changes required to COM API surface for this feature. The existing `PackageInstallerType` enum already maps to the C++ `InstallerTypeEnum` and will automatically include `NoInstaller` through the converter functions.

### PowerShell Module

No changes required to PowerShell cmdlets. The existing cmdlets (`Find-WinGetPackage`, `Show-WinGetPackage`, `Install-WinGetPackage`, `Uninstall-WinGetPackage`, etc.) will handle `NoInstaller` packages naturally:

- `Find-WinGetPackage` returns `NoInstaller` packages in search results.
- `Show-WinGetPackage` displays `NoInstaller` package information.
- `Install-WinGetPackage` terminates with appropriate error when attempting to install a `NoInstaller` package.
- `Uninstall-WinGetPackage` uses ARP correlation to uninstall `NoInstaller` packages that are already installed on the system.
- `Update-WinGetPackage` skips `NoInstaller` versions or terminates if no installable versions exist.
- `Repair-WinGetPackage` is not applicable to `NoInstaller` packages.

### Settings and Group Policy

No new settings or group policies are required for `NoInstaller` support. The feature is opt-in at the manifest level.

### Version Tombstone Scenario

When a version is tombstoned (removed for security, publisher request, or identifier change):

1. The version entry is retained in the repository with `InstallerType: NoInstaller`.
2. The `InstallerAvailabilityMessage` explains the reason: `"This version has been superseded by [PackageIdentifier]"` or `"This version was removed due to [CVE-XXXX]"`.
3. The package entry prevents false matches against other manifests (deduplication).
4. Users attempting to install the removed version receive clear messaging and guidance.
5. The entry allows the index to track the removal without deleting repository history.

### Architecture-Specific Scenarios

When a package has multiple architectures with independent installer availability:

**Scenario 1: Mixed Installability**
```yaml
Installers:
  - Architecture: x64
    InstallerType: Exe
    InstallerUrl: https://example.com/app-x64.exe
  - Architecture: arm64
    InstallerType: NoInstaller
    InstallerAvailabilityMessage: "ARM64 version not yet available"
```

**Behavior:**
- User on x64 system can install normally.
- User on arm64 system receives message that ARM64 is unavailable and cannot install this version.
- For upgrade scenarios: arm64 users cannot upgrade to this version; WinGet informs user that no applicable installer for their architecture is available.

**Scenario 2: Architecture Removed**
```yaml
# Version 1.1 (current)
Installers:
  - Architecture: x86
    InstallerType: Exe
    InstallerUrl: https://example.com/app-x86.exe
  - Architecture: x64
    InstallerType: Exe
    InstallerUrl: https://example.com/app-x64.exe

# Version 2.0 (x86 dropped)
Installers:
  - Architecture: x86
    InstallerType: NoInstaller
    InstallerAvailabilityMessage: "x86 support has been dropped in this version"
  - Architecture: x64
    InstallerType: Exe
    InstallerUrl: https://example.com/app-x64.exe
```

**Behavior:**
- x86 users can upgrade to version 1.1 but not to 2.0; they are informed x86 is no longer available.
- x64 users can upgrade to version 2.0 normally.

### Manifest Versioning

The `NoInstaller` installer type and `InstallerAvailabilityMessage` field are introduced in **Manifest Version 1.30.0**.

Older clients (< 1.30.0) will not recognize `InstallerType: NoInstaller` or `InstallerAvailabilityMessage` fields because they are part of manifest version 1.30.0. The manifest validation schema enforces version requirements.

## UI/UX Design

### Error Messages

**Generic (no message provided):**
```
$ winget install ExamplePackage
Found Example Package [Example.Package]

The installer for this package is no longer available.

Exit Code: 0x8A150116 (APPINSTALLER_CLI_ERROR_INSTALLER_NOT_AVAILABLE)
```

**With InstallerAvailabilityMessage:**
```
$ winget install ExamplePackage
Found Example Package [Example.Package]

This package is obsolete. Please use Example.Package.2 instead.

Exit Code: 0x8A150116 (APPINSTALLER_CLI_ERROR_INSTALLER_NOT_AVAILABLE)
```

### Show Output

```
$ winget show ExamplePackage

AppInstaller Test NoInstaller
Publisher: Microsoft Corporation
License: Test
Description: AppInstaller Test NoInstaller

Version: 1.0.0.0
Locale: en-US
Installer Type: No Installer
Installer Locale: en-US
Installer Sha256:
Installer Product Id:
Installer Release Date:
Offline Distribution Supported: false
Installer Availability Message: This package is obsolete. Please use Example.Package instead.
```

### Search Results

`NoInstaller` packages appear in search results like other packages:
```
$ winget search example
Name                     ID                   Version Available Source
Example Package          Example.Package      2.0.0              winget
```

## Capabilities

### Accessibility

- Terminal output for `NoInstaller` packages follows existing accessibility practices.
- Screen reader users will hear the `InstallerAvailabilityMessage` when present.
- Error messages are clearly distinguished from success messages.
- No new accessibility issues are introduced.

### Security

**Validation-No-Executables Waiver:**
- The `NoInstaller` type cannot be exploited as a distribution vector for malicious code; there is no executable to download or run.
- However, the community repository must gate submissions with a waiver requirement to prevent misuse (e.g., publishers blocking inclusion by marking everything as `NoInstaller`).
- Legitimate use cases (system components, OEM software, security tombstones) are approved through manual review.

### Reliability

- `NoInstaller` packages have no download or execution risk; reliability is not impacted.
- Users are clearly informed when a package cannot be installed, preventing confusion or support tickets.
- Version tombstones can reliably indicate removal reasons, reducing false matches and improving correlation.

### Compatibility

**Breaking Changes:** None. The `NoInstaller` type is additive:
- Older manifests without this type are unaffected.
- Older clients (< 1.30.0) ignore manifest version 1.30.0 files with `NoInstaller` because they enforce schema version compatibility.
- Clients that encounter `NoInstaller` in a compatible schema version handle it gracefully with appropriate error messages.

**Manifest Version Policy:**
- The community repository (winget-pkgs) enforces schema version n or n-1 for new submissions.
- Publishers must use manifest version 1.30.0 (or newer) to use `NoInstaller`.
- Transitional period: Once 1.30.0 is the standard, older publishers are encouraged to upgrade.

**Metadata Compatibility:**
- `ProductCode`, `PackageFamilyName`, and other correlation fields remain compatible with existing upgrade/uninstall tracking.
- `InstallerSha256` (optional) allows deduplication without downloading.

### Performance, Power, and Efficiency

- No download activity for `NoInstaller` packages; user bandwidth and disk I/O are preserved.
- Install operation terminates immediately with an error message, avoiding wasted processing.
- Repo index size is minimally impacted; `NoInstaller` entries use the same schema as other installer types.

## Potential Issues

### Publisher Misuse

**Issue:** Publishers might use `NoInstaller` to block inclusion in the community repository ("We don't want WinGet distribution for corporate reasons").

**Mitigation:**
- `Validation-No-Executables` rule requires waiver for all `NoInstaller` submissions.
- Microsoft reviews each submission to verify legitimate use (system components, OEM, security tombstones).
- Publishers cannot unilaterally block inclusion by marking packages as `NoInstaller`; review is mandatory.

### Version Confusion

**Issue:** Users may be confused why a package version is listed but not installable.

**Mitigation:**
- Prominent messaging in `show` output and install error messages.
- `InstallerAvailabilityMessage` provides actionable context (e.g., "Upgrade to version 3.0.0" or "This package is obsolete. Please use Example.Package.2 instead.").
- Search and upgrade flows clearly indicate unavailability.

### Mixed-Architecture Complexity

**Issue:** A single version may have installable and non-installable architectures, complicating user upgrade paths.

**Mitigation:**
- Architecture selection logic in the manifest comparator already handles mixed compatibility.
- Users on unavailable architectures receive clear guidance and informed when no applicable installer exists for their architecture.
- Future consideration for architecture fallback logic may be addressed in Issue #823 (installer type filtering).

### Repository Inconsistency

**Issue:** If the same package identifier appears in multiple sources with different `NoInstaller` status, conflicts may arise.

**Mitigation:**
- Manifest resolution follows existing source precedence rules (first matching source wins).
- Within a source, a single package identifier has one set of installers; architectural variants are consistent.
- Users can prioritize sources to control behavior.

### Mixed-Locale Complexity

**Issue:** A single version may have different installer availability across different locales (e.g., en-US installer available, ja-JP `NoInstaller`), complicating package correlation and user expectations.

**Mitigation:**
- Manifest validation should flag or warn when a version has inconsistent installer availability across locales.
- Users are informed via `InstallerAvailabilityMessage` when their locale is unavailable.
- Locale-specific installer selection logic ensures users get the correct installer for their system locale.
- Future enhancements to locale fallback logic may improve user experience when preferred locale is unavailable.
- If a user has multiple locales on their system, `NoInstaller` as an installer type should take lower precedence than any supported locale with an available installer.

## Future Considerations

### Issue #823 — Installer Type Filtering

When installer type filtering is implemented ([#823](https://github.com/microsoft/winget-cli/issues/823)), users will be able to filter search results and upgrade candidates by installer type. The default behavior should exclude `NoInstaller` packages from search and upgrade operations to prevent users from inadvertently selecting non-installable versions. Users may explicitly include `NoInstaller` packages if needed through an opt-in flag:

```
$ winget search example                              # Excludes NoInstaller by default
$ winget search example --include-no-installer       # Includes NoInstaller packages
$ winget upgrade                                     # Skips NoInstaller versions by default
$ winget upgrade --include-no-installer              # Considers NoInstaller versions
```

This default behavior improves user experience by focusing on actionable, installable packages while still allowing advanced users to work with `NoInstaller` entries when needed. Implementation deferred to Issue #823.

### Architecture Fallback Logic

In mixed-architecture scenarios where a user's system architecture is unavailable, WinGet may eventually support offering compatible architecture alternatives (e.g., offering x64 to an ARM64 user when ARM64 is `NoInstaller`). This capability would enhance user experience but requires additional architecture compatibility reasoning and is deferred as a future enhancement.

### Portable Repair

The repair feature specification ([#148](https://github.com/microsoft/winget-cli/issues/148)) currently excludes `Portable` packages from repair. A future consideration might extend repair support to `NoInstaller` packages in limited scenarios (e.g., re-registration of ARP entries), but this is out of scope for the current feature.

### Repository Sync and Replication

For REST sources and enterprise mirrors, `NoInstaller` entries should be replicated to allow consistent behavior across deployments. This follows existing REST schema evolution patterns and is out of scope for this specification.

## Resources

- **Issue #2405:** [Manifests without installer info](https://github.com/microsoft/winget-cli/issues/2405)
- **Issue #1900:** [Version Tombstones](https://github.com/microsoft/winget-cli/issues/1900)
- **Issue #823:** [Filtering on installer method / installer type](https://github.com/microsoft/winget-cli/issues/823)
- **Issue #148:** [Repair Support](https://github.com/microsoft/winget-cli/issues/148)
- **PR #6157:** [Add support for Manifests with No Installers](https://github.com/microsoft/winget-cli/pull/6157)
