---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-17
last updated: 2026-08-06
issue id: 6289
---

# Native DSC v3 Resources in WinGet Client

For [#6289](https://github.com/microsoft/winget-cli/issues/6289).

## Abstract

WinGet already ships native DSC v3 command resources for packages, sources, user settings, and administrator settings. This specification documents those existing resources, adds package installer-type selection, and proposes native `PackageList` and `Pin` resources. Users author and apply these resources through `winget configure`; the internal `winget dscv3` commands remain implementation details of the DSC resource protocol.

## Inspiration

The `Microsoft.WinGet.DSC` PowerShell module provides broad WinGet Configuration coverage, but its class-based resources require the PowerShell adapter when used with DSC v3. Internal performance testing observed approximately 92 seconds per adapted resource invocation compared with approximately 3-5 seconds for native command resources. The exact duration varies by machine and resource, but the adapter overhead makes larger configurations impractical.

The WinGet client already contains these native command resources:

- `Microsoft.WinGet/Package`
- `Microsoft.WinGet/Source`
- `Microsoft.WinGet/UserSettingsFile`
- `Microsoft.WinGet/AdminSettings`

The remaining gaps addressed by this specification are:

1. Selecting a required installer type for a package.
2. Declaring a set of packages in one resource while safely supporting DSC `_purge` semantics.
3. Managing package pins without the PowerShell adapter.
4. Exporting configuration that uses the native resources.

Related issues include [#3401](https://github.com/microsoft/winget-cli/issues/3401) and [#5806](https://github.com/microsoft/winget-cli/issues/5806).

## Solution Design

### Resource Overview

| Resource type | Status | Purpose |
|---|---|---|
| `Microsoft.WinGet/Package` | Existing; enhanced | Manage one package and optionally require an installer type |
| `Microsoft.WinGet/PackageList` | New | Manage a declared set of packages with optional, guarded `_purge` behavior |
| `Microsoft.WinGet/Source` | Existing | Manage one WinGet package source |
| `Microsoft.WinGet/UserSettingsFile` | Existing | Partially update or fully replace the user settings file |
| `Microsoft.WinGet/AdminSettings` | Existing | Manage administrator settings |
| `Microsoft.WinGet/Pin` | New | Manage one package pin |

The existing resource types and property names remain compatible. New properties are additive.

### Architecture

`winget configure` remains the user-facing command for applying and testing configuration files:

```text
Configuration file
        |
        v
winget configure
        |
        v
DSC processor
        |
        v
Microsoft.WinGet/* command resources
        |
        v
WinGet package management operations
```

The resource manifests invoke implementation-facing `winget dscv3` resource handlers. These handlers read resource JSON from standard input and write protocol-compliant JSON to standard output. They are not a replacement for `winget configure`, and this specification does not change the existing `winget dsc` alias.

For example, `winget dscv3 package --manifest` emits the existing
`Microsoft.WinGet/Package` manifest. Its operation definitions use this form:

```json
{
  "type": "Microsoft.WinGet/Package",
  "export": {
    "executable": "winget",
    "args": ["dscv3", "package", "--export"],
    "input": "stdin"
  },
  "get": {
    "executable": "winget",
    "args": ["dscv3", "package", "--get"],
    "input": "stdin"
  },
  "set": {
    "executable": "winget",
    "args": ["dscv3", "package", "--set"],
    "input": "stdin",
    "implementsPretest": true,
    "handlesExist": true,
    "return": "stateAndDiff"
  },
  "test": {
    "executable": "winget",
    "args": ["dscv3", "package", "--test"],
    "input": "stdin",
    "return": "stateAndDiff"
  },
  "schema": {
    "command": {
      "executable": "winget",
      "args": ["dscv3", "package", "--schema"]
    }
  }
}
```

The new resources follow the same established command shape:

```text
winget dscv3 package-list --get|--set|--test|--export|--schema|--manifest
winget dscv3 pin          --get|--set|--test|--export|--schema|--manifest
```

`winget dscv3 --manifest --output <directory>` emits all WinGet resource manifests. The new
resource manifests are named:

```text
microsoft.winget.package-list.dsc.resource.json
microsoft.winget.pin.dsc.resource.json
```

### `Microsoft.WinGet/Package`

`Microsoft.WinGet/Package` continues to manage one package per resource instance.

```yaml
- type: Microsoft.WinGet/Package
  name: Git
  properties:
    id: Git.Git
    source: winget
    installerType: portable
    useLatest: true
    installMode: silent
    acceptAgreements: true
  metadata:
    description: Install Git using the portable installer
```

| Property | Type | Required | Description |
|---|---|---|---|
| `id` | string | Yes | Package identifier |
| `_exist` | boolean | No | Whether the package should exist; defaults to `true` |
| `source` | string | No | Source used to resolve the package |
| `version` | string | No | Specific package version |
| `useLatest` | boolean | No | Require the latest available version; defaults to `false` |
| `installerType` | string | No | Required installer type when specified |
| `installMode` | string | No | `default`, `silent`, or `interactive`; defaults to `silent` |
| `matchOption` | string | No | Package identifier matching behavior |
| `acceptAgreements` | boolean | No | Accept source and package agreements |

`version` and `useLatest: true` are mutually exclusive. The resource returns a validation error if both are specified.

When `installerType` is specified, it is a requirement rather than a preference. WinGet must select an installer of that type that also satisfies the package, version, architecture, scope, locale, and source constraints applicable to the operation. If no installer satisfies all constraints, the resource fails without falling back to another installer type.

The `_exist` property follows the canonical DSC contract:

- `_exist: true` installs or updates the package as needed.
- `_exist: false` uninstalls the matching package if it is installed.

### `Microsoft.WinGet/PackageList`

`Microsoft.WinGet/PackageList` manages multiple package declarations in one resource instance. Each package can specify its own source, version, installer type, installation mode, and agreement behavior.

```yaml
- type: Microsoft.WinGet/PackageList
  name: DeveloperPackages
  properties:
    packages:
      - id: Git.Git
        source: winget
        installerType: portable
        useLatest: true
      - id: Microsoft.VisualStudioCode
        source: winget
        installerType: exe
        useLatest: true
      - id: Python.Python.3.12
        source: winget
        version: 3.12.4
    _purge: false
  metadata:
    description: Developer workstation packages
```

| Property | Type | Required | Description |
|---|---|---|---|
| `packages` | array | Yes | Desired packages |
| `_purge` | boolean | No | Remove eligible WinGet-managed packages not in `packages`; defaults to `false` |
| `acceptAgreements` | boolean | No | Default agreement behavior for entries that omit it |

Each entry in `packages` supports:

| Property | Type | Required | Description |
|---|---|---|---|
| `id` | string | Yes | Package identifier |
| `source` | string | No | Source used to resolve the package |
| `version` | string | No | Specific package version |
| `useLatest` | boolean | No | Require the latest available version |
| `installerType` | string | No | Required installer type when specified |
| `installMode` | string | No | `default`, `silent`, or `interactive` |
| `matchOption` | string | No | Package identifier matching behavior |
| `acceptAgreements` | boolean | No | Overrides the resource-level agreement behavior |

The same `version`, `useLatest`, and `installerType` behavior defined for `Microsoft.WinGet/Package` applies independently to every entry.

#### `_purge` Safety Boundary

`_purge` uses the canonical DSC meaning: entries in the resource's managed collection that are not declared in `packages` should be removed. It is always opt-in and defaults to `false`.

As required by the DSC canonical property contract, `_purge` is write-only. Get, test, set, and
export output must not return it. The resource schema documents that `_purge` affects the
`packages` collection.

The managed collection is intentionally narrower than all software installed on Windows. The resource may purge a package only when WinGet has authoritative management ownership and a stable package identity. At minimum:

1. WinGet tracking data identifies the package as installed and managed by WinGet.
2. The installed package maps to a stable package identifier and source.
3. WinGet has a supported uninstall path for the package.
4. Policy does not protect or prohibit removal of the package.

The resource must not purge:

- Unknown Add or Remove Programs entries.
- Packages inferred only through best-effort catalog correlation.
- Windows components, provisioned applications, drivers, features, or updates.
- Packages without WinGet management ownership.
- Dependencies or other packages for which WinGet cannot establish that removal is safe.

Packages outside this managed collection do not cause the resource to report drift. Diagnostic
logging may report the number of installed packages skipped because they were outside the purge
boundary, but telemetry does not expose their identities.

When `_purge: true`:

1. `winget configure test` reports packages eligible for removal without changing the machine.
2. `winget configure` fails before making purge removals if Group Policy does not permit package-list purge.
3. Eligible removals and requested installations use the same package-operation scheduling model defined by the parallel installation specification in [#6295](https://github.com/microsoft/winget-cli/pull/6295).
4. A failed package operation is reported against that package and causes the resource operation to fail. Other independent operations may complete according to the scheduler's failure behavior.

WinGet must never generate `_purge: true` automatically during configuration export.

### `Microsoft.WinGet/Source`

The existing source resource remains unchanged and continues to use `_exist`.

```yaml
- type: Microsoft.WinGet/Source
  name: CorporateSource
  properties:
    name: corporate
    argument: https://winget.contoso.com/api
    type: Microsoft.Rest
    trustLevel: trusted
    explicit: false
    _exist: true
```

| Property | Type | Required | Description |
|---|---|---|---|
| `name` | string | Yes | Source name |
| `_exist` | boolean | No | Whether the source should exist |
| `argument` | string | No | Source argument, such as its endpoint |
| `type` | string | No | Source type |
| `trustLevel` | string | No | `undefined`, `none`, or `trusted` |
| `explicit` | boolean | No | Whether the source is excluded from operations that do not name it |
| `priority` | number | No | Source priority; available when the existing source-priority experimental feature is enabled |
| `acceptAgreements` | boolean | No | Accept source agreements |

### `Microsoft.WinGet/UserSettingsFile`

The existing user settings resource remains unchanged. The `action` property uses the existing `Partial` and `Full` values.

```yaml
- type: Microsoft.WinGet/UserSettingsFile
  name: WinGetUserSettings
  properties:
    action: Partial
    settings:
      visual:
        progressBar: rainbow
      installBehavior:
        preferences:
          scope: machine
```

| Property | Type | Required | Description |
|---|---|---|---|
| `action` | string | No | `Partial` merges supplied values; `Full` replaces the file |
| `settings` | object | Yes | WinGet user settings content |

### `Microsoft.WinGet/AdminSettings`

The existing administrator settings resource remains unchanged. The resource requires elevation because administrator settings are machine-scoped.

```yaml
- type: Microsoft.WinGet/AdminSettings
  name: WinGetAdminSettings
  properties:
    settings:
      LocalManifestFiles: true
      BypassCertificatePinningForMicrosoftStore: false
  metadata:
    winget:
      securityContext: elevated
```

The resource reports settings blocked by Group Policy and does not override policy.

### `Microsoft.WinGet/Pin`

`Microsoft.WinGet/Pin` manages one pin per resource instance. A configuration declares multiple resource instances to manage multiple pins. This preserves per-pin identity, status, and failure reporting; export emits one resource instance for each pin.

```yaml
- type: Microsoft.WinGet/Pin
  name: ContosoReleasePin
  properties:
    id: Contoso.App
    source: winget
    pinType: gating
    version: 4.2.*
    _exist: true
  metadata:
    description: Keep Contoso App on the approved release line
```

| Property | Type | Required | Description |
|---|---|---|---|
| `id` | string | Yes | Package identifier |
| `source` | string | No | Source used to identify the package |
| `pinType` | string | No | `pinning`, `blocking`, or `gating`; defaults to `pinning` |
| `version` | string | For `gating` | Gated version pattern |
| `_exist` | boolean | No | Whether the pin should exist; defaults to `true` |

The resource validates that `version` is provided only for `gating`. `_exist: false` removes the matching pin.

### Configuration Export

`winget configure export` is enhanced to emit native resources:

- Packages are emitted as individual `Microsoft.WinGet/Package` instances by default.
- An export mode may combine WinGet-managed packages into `Microsoft.WinGet/PackageList`.
- Exported `PackageList` instances omit `_purge` or explicitly set it to `false`.
- Sources, settings, administrator settings, and pins use their corresponding native resource types when requested and available.
- Software outside WinGet's authoritative management boundary is not added to `PackageList`.

Export does not imply that applying the resulting configuration will remove undeclared software.

### Group Policy

A new policy controls destructive package-list convergence:

| Policy | Purpose | Default |
|---|---|---|
| `EnablePackageListPurge` | Permit `Microsoft.WinGet/PackageList` to process `_purge: true` | Disabled |

When the policy is disabled or not configured, `_purge: false` continues to work. A resource instance requesting `_purge: true` fails before performing purge removals and returns a policy-specific error.

No user setting can override this policy.

### CLI, API, Schema, and Repository Impact

| Surface | Impact |
|---|---|
| WinGet CLI | `winget configure` applies and tests the resources; implementation-facing `winget dscv3` handlers and manifests are added for `PackageList` and `Pin` |
| COM API | No new public COM API is required; resources use existing package-management operations |
| PowerShell | No cmdlet changes are required |
| User settings | No new `settings.json` setting is required |
| Package manifests | No WinGet package-manifest schema change is required |
| `winget-pkgs` validation | No change |
| `winget-create` | No change |
| `winget-cli-restsource` | No change |
| `winget-dsc` | Existing PowerShell resources remain supported; no immediate deprecation |

### Interactivity

DSC resource protocol operations are non-interactive. `winget configure` may prompt before resource
execution for configuration agreements or elevation, but the resource handlers must not prompt
regardless of terminal availability.

- Package and source agreements must be accepted explicitly through `acceptAgreements`.
- `installMode: interactive` permits an interactive installer UI only when the configuration host supports it; otherwise the resource returns an error before launching the installer.
- `--disable-interactivity`, `--silent`, COM API callers, and SYSTEM context never receive a prompt.
- `--no-vt` changes rendering only and does not change resource behavior.

### Rollout and Telemetry

Implementation and enablement are conditional on:

1. The required DSC runtime and WinGet Configuration processor versions being available.
2. The corresponding WinGet version being available in the applicable Windows feature, capability,
   and integration builds and servicing channels.
3. Package-operation scheduling from [#6295](https://github.com/microsoft/winget-cli/pull/6295) being available before parallel behavior is enabled.
4. Purge safety and rollback testing meeting reliability requirements.

Privacy-preserving telemetry should measure:

- Resource type and operation (`get`, `test`, `set`, or `export`).
- Duration, success, failure category, and cancellation.
- Whether `_purge` was requested, permitted, or blocked by policy.
- Counts of packages installed, upgraded, removed, failed, or skipped as outside the purge boundary.
- Installer-type requirement success or failure.

Telemetry must not include package identifiers, source URLs, settings content, file paths, or pin values.

## UI/UX Design

Users interact with the resources through a WinGet Configuration file:

```yaml
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
metadata:
  winget:
    processor:
      identifier: dscv3
resources:
  - type: Microsoft.WinGet/PackageList
    name: DeveloperTools
    properties:
      packages:
        - id: Git.Git
          source: winget
          installerType: portable
          useLatest: true
        - id: Microsoft.VisualStudioCode
          source: winget
          installerType: exe
          useLatest: true
      _purge: false
    metadata:
      description: Developer tools
  - type: Microsoft.WinGet/UserSettingsFile
    name: WinGetPreferences
    properties:
      action: Partial
      settings:
        installBehavior:
          preferences:
            scope: machine
    metadata:
      description: Configure WinGet preferences
```

Testing the configuration does not change the machine:

```text
> winget configure test --file developer-tools.dsc.yaml

Testing configuration...
  Developer tools: Package changes required
  Configure WinGet preferences: In desired state

Configuration is not in the desired state.
```

Applying the configuration uses the existing command:

```text
> winget configure --file developer-tools.dsc.yaml

Applying configuration...
  Developer tools: Successfully applied
  Configure WinGet preferences: In desired state

Configuration successfully applied.
```

If purge is requested but prohibited by policy, WinGet fails before removing packages:

```text
The configuration requested package-list purge, but the EnablePackageListPurge
policy does not permit this operation. No packages were removed.
```

If a required installer type is unavailable, the relevant package fails without fallback:

```text
No installer of type 'portable' satisfies the requirements for package Git.Git.
```

## Capabilities

### Accessibility

The resources are declarative and introduce no graphical UI. `winget configure` continues to use the existing accessible terminal output and must provide equivalent status text without relying solely on color, symbols, or VT rendering.

### Security

- `_purge` is explicit, disabled by default, and controlled by Group Policy.
- Purge applies only to packages for which WinGet has authoritative management ownership.
- Test reports candidates without changing the machine.
- Source trust, installer hash validation, signature validation, and existing security policies remain in effect.
- Administrator settings continue to require elevation and cannot override Group Policy.
- Resource manifests ship and are serviced with the WinGet client.

### Reliability

- Existing resource contracts remain compatible.
- A required installer type prevents unexpected fallback to another installer technology.
- Package-level results identify partial failures in `PackageList`.
- `_purge` ignores software outside WinGet's managed boundary.
- Resource manifests use `implementsPretest: true` so set operations avoid unnecessary work.
- Package scheduling follows [#6295](https://github.com/microsoft/winget-cli/pull/6295) rather than defining a second scheduler.

### Compatibility

- `winget dsc` remains equivalent to `winget configure`; this specification does not repurpose it.
- Existing configurations using `Microsoft.WinGet/Package`, `Source`, `UserSettingsFile`, or `AdminSettings` continue to work.
- Existing `Microsoft.WinGet.DSC/*` PowerShell resources remain supported through the adapter.
- The `installerType` property is optional and additive.
- No WinGet package-manifest schema version change is required.

### Performance, Power, and Efficiency

Native command resources avoid PowerShell adapter startup for each operation. `PackageList` also permits the package scheduler to coordinate a set of operations in one resource invocation. Actual concurrency and serialization follow [#6295](https://github.com/microsoft/winget-cli/pull/6295); this specification does not independently define installer scheduling rules.

## Potential Issues

1. **Purge ownership data** — Existing installations may lack sufficient tracking data to be eligible for purge. This is a safe limitation: the resource skips them rather than risking removal.
2. **Partial convergence** — A package list can partially apply before another package fails. Results must identify every completed and failed operation.
3. **Installer-type availability** — Requiring an installer type can make a configuration fail when a publisher changes installer technologies.
4. **Version availability** — Exact versions may disappear from mutable sources, preventing convergence.
5. **Export fidelity** — WinGet cannot safely export software it does not authoritatively manage.
6. **SYSTEM context** — Sources, network access, and user-scoped packages can differ under SYSTEM.
7. **Pin correlation** — Pin operations require a stable package and source identity.
8. **Resource schema evolution** — Additive changes must remain compatible with configurations authored for earlier WinGet versions.

## Future Considerations

- Explicitly adopting an existing installation into WinGet management so it can become eligible for purge.
- A protected-package or purge-exclusion mechanism if enterprise scenarios require additional safeguards.
- Continuous drift detection and reporting.
- Intune and other management-service integration.
- Dependency-aware package-list planning.
- Additional native resources where command-based implementation provides measurable value.
- Adapted resource manifests for the existing `Microsoft.WinGet.DSC` PowerShell resources.

## Resources

- [Native DSC v3 resources issue #6289](https://github.com/microsoft/winget-cli/issues/6289)
- [Parallel installation specification PR #6295](https://github.com/microsoft/winget-cli/pull/6295)
- [Package pinning specification](https://github.com/microsoft/winget-cli/blob/master/doc/specs/%23476%20-%20Package%20Pinning.md)
- [DSC resources](https://learn.microsoft.com/powershell/dsc/concepts/resources)
- [DSC `_exist` property](https://learn.microsoft.com/powershell/dsc/reference/schemas/resource/properties/exist)
- [DSC `_purge` property](https://learn.microsoft.com/powershell/dsc/reference/schemas/resource/properties/purge)
- [DSC resource manifest schema](https://learn.microsoft.com/powershell/dsc/reference/schemas/resource/manifest/root)
- [WinGet Configuration v3 schema](https://learn.microsoft.com/windows/package-manager/configuration/create-v3)
- [WinGet Configuration documentation](https://learn.microsoft.com/windows/package-manager/configuration/)
- [WinGet DSC module](https://github.com/microsoft/winget-dsc)
