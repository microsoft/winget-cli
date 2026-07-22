---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-17
last updated: 2026-06-17
issue id: 6289
---

# Native DSC v3 Resources in WinGet Client

For [#6289](https://github.com/microsoft/winget-cli/issues/6289).

## Abstract

Implement native DSC v3 command-based resources directly in the WinGet client, including a `PackageList` resource that declares an entire machine's desired package state. This eliminates the ~92s adapter overhead per resource and enables full configuration-as-code parity with the CLI — anything achievable via `winget` on the command line can be expressed declaratively in a configuration file.

## Inspiration

Today's WinGet DSC resources (`Microsoft.WinGet.DSC` PowerShell module) are class-based resources requiring the PowerShell adapter in DSC v3. This creates problems:

1. **Performance** — Each adapted resource invocation takes ~92s through the dscv3 processor vs. ~3-5s for native v3 resources. A 10-package configuration takes 15+ minutes.
2. **Complexity** — Configuration authors must understand adapter mechanics, module installation paths (`$env:PSModulePath` for v3 vs `%LOCALAPPDATA%\Microsoft\WinGet\Configuration\Modules` for v2), and processor selection.
3. **Gaps** — No resource exists for "this machine should have exactly these packages" — the most requested enterprise scenario.
4. **SYSTEM context** — PowerShell adapter resources have additional failure modes in SYSTEM context.

Enterprise IT administrators writing configuration-as-code have explicitly cited incomplete DSC resource coverage as a reason for not adopting WinGet Configuration.

Related: [#3401](https://github.com/microsoft/winget-cli/issues/3401), [#5806](https://github.com/microsoft/winget-cli/issues/5806).

## Solution Design

### Resources

The WinGet client already ships native DSC v3 command-based resources via the `winget dsc` subcommand infrastructure (including `DscPackageResource`, `DscSourceResource`, `DscUserSettingsFileResource`, and `DscAdminSettingsResource`). This spec extends the existing infrastructure with enhanced capabilities and new resources:

| Resource Type | Status | Purpose | DSC Operations |
|--------------|--------|---------|----------------|
| `Microsoft.WinGet/Package` | **Enhanced** — add list support for declarative package sets | Get, Set, Test |
| `Microsoft.WinGet/Source` | Existing | Package source configuration | Get, Set, Test |
| `Microsoft.WinGet/UserSettings` | Existing | User-scoped WinGet settings | Get, Set, Test |
| `Microsoft.WinGet/AdminSettings` | Existing (requires elevation) | Admin-scoped WinGet settings | Get, Set, Test |
| `Microsoft.WinGet/Pin` | **New** | Package pin management | Get, Set, Test |

### Architecture

The existing `winget dsc` subcommand implements the DSC v3 command-based resource protocol:

```
┌─────────────────────────────────────┐
│  DSC v3 Runtime                     │
│  ┌───────────────────────────────┐  │
│  │ Resource Manifest (.dsc.json) │  │
│  │ type: Microsoft.WinGet/*      │  │
│  │ method: command-based         │  │
│  │ executable: winget.exe        │  │
│  └───────────┬───────────────────┘  │
│              │ stdin (JSON)          │
│              ▼                       │
│  ┌───────────────────────────────┐  │
│  │ winget dsc <get|set|test>     │  │
│  │ --resource <ResourceType>     │  │
│  └───────────┬───────────────────┘  │
│              │                       │
│              ▼                       │
│  ┌───────────────────────────────┐  │
│  │ WinGet Engine (COM API)       │  │
│  │ (existing install/search/etc) │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

The `winget dsc` subcommand:
- Reads JSON input from stdin per DSC v3 protocol
- Dispatches to the appropriate resource handler
- Outputs JSON result to stdout
- Returns exit code 0 for success, non-zero for failure

### Resource Manifests

Each resource ships a `.dsc.resource.json` manifest alongside `winget.exe`:

```json
{
  "$schema": "https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2024/04/bundled/resource/manifest.json",
  "type": "Microsoft.WinGet/Package",
  "version": "1.0.0",
  "description": "Manage a single WinGet package",
  "get": {
    "executable": "winget",
    "args": ["dsc", "get", "--resource", "Package"]
  },
  "set": {
    "executable": "winget",
    "args": ["dsc", "set", "--resource", "Package"],
    "implementsPretest": true
  },
  "test": {
    "executable": "winget",
    "args": ["dsc", "test", "--resource", "Package"]
  },
  "schema": {
    "command": {
      "executable": "winget",
      "args": ["dsc", "schema", "--resource", "Package"]
    }
  }
}
```

### Microsoft.WinGet/Package

Manages package desired state — supports both single packages and declarative package lists:

**Single package:**

```yaml
resources:
  - resource: Microsoft.WinGet/Package
    directives:
      description: Install Git
    settings:
      id: Git.Git
      source: winget
      version: ">=2.44.0"
      scope: machine
      ensure: present
```

**Package list (declarative set):**

```yaml
resources:
  - resource: Microsoft.WinGet/Package
    directives:
      description: Developer workstation packages
    settings:
      source: winget
      enforcement: present
      scope: machine
      packages:
        - id: Git.Git
          version: ">=2.44.0"
        - id: Microsoft.VisualStudioCode
          ensure: latest
        - id: Docker.DockerDesktop
        - id: Python.Python.3.12
```

**Schema:**

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `id` | string | Yes (single mode) | Package identifier |
| `packages` | array | Yes (list mode) | Array of package declarations (mutually exclusive with `id`) |
| `source` | string | No | Source name (default: winget) |
| `version` | string | No | Version constraint (exact, range via `>=`/`<=`, or `latest`) |
| `scope` | enum | No | `machine` \| `user` |
| `ensure` | enum | No | `present` (default) \| `absent` \| `latest` |
| `enforcement` | enum | No | List mode only: `present` (default) \| `exact` |
| `installerType` | string | No | Preferred installer type |
| `overrideArguments` | string | No | Custom installer arguments |
| `acceptAgreements` | bool | No | Accept source/package agreements |

> [!NOTE]
> The resource operates in **single mode** when `id` is specified, and **list mode** when `packages` is specified. These are mutually exclusive — specifying both is an error.

**Test behavior:**
- `ensure: present` — true if package is installed with version matching constraint
- `ensure: absent` — true if package is NOT installed
- `ensure: latest` — true if installed version matches latest available in source

**Set behavior:**
- `ensure: present` — install if not present, upgrade if version constraint not met
- `ensure: absent` — uninstall if present
- `ensure: latest` — upgrade to latest if not already

**Enforcement modes (list mode only):**
- `present` (default) — Ensure listed packages exist. Does not remove unlisted packages. Additive-only.
- `exact` — Ensure ONLY listed packages exist. **Removes unlisted packages.** Requires Group Policy `EnableExactEnforcement` to be enabled. Without the policy, `exact` mode fails with a clear error.

**Set behavior in list mode:**
1. For each package in the list, evaluate individually
2. MSIX packages may be installed concurrently (up to 6 concurrent, matching existing WinGet behavior)
3. MSI/EXE packages are installed sequentially (Win32 installer mutex)
4. Failures are reported per-package; other packages continue

**Test output (list mode):**

```json
{
  "inDesiredState": false,
  "packages": [
    { "id": "Git.Git", "inDesiredState": true, "installedVersion": "2.45.1" },
    { "id": "Docker.DockerDesktop", "inDesiredState": false, "reason": "not installed" }
  ]
}
```

### Microsoft.WinGet/Source

```yaml
resources:
  - resource: Microsoft.WinGet/Source
    settings:
      name: corporate
      url: https://winget.contoso.com/api
      type: Microsoft.Rest
      ensure: present
      trustLevel: trusted
```

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `name` | string | Yes | Source name |
| `url` | string | Yes (for set) | Source URL |
| `type` | string | No | Source type (default: `Microsoft.Rest`) |
| `ensure` | enum | No | `present` \| `absent` |
| `trustLevel` | enum | No | `none` \| `trusted` |

### Microsoft.WinGet/UserSettings

> [!NOTE]
> This resource already exists as `DscUserSettingsFileResource` in the WinGet client. The spec documents the target schema for parity with other resources.

```yaml
resources:
  - resource: Microsoft.WinGet/UserSettings
    settings:
      ensure: merged
      settings:
        visual:
          progressBar: rainbow
        installBehavior:
          preferences:
            scope: machine
```

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `ensure` | enum | No | `merged` (default) \| `replaced` |
| `settings` | object | Yes | WinGet settings JSON structure |

- `merged` — overlay provided settings on existing (deep merge)
- `replaced` — overwrite entire settings file

### Microsoft.WinGet/AdminSettings

> [!NOTE]
> This resource already exists as `DscAdminSettingsResource` in the WinGet client. It requires elevated security context (`securityContext: elevated` in DSC metadata) because admin settings are machine-scoped and affect all users.

```yaml
resources:
  - resource: Microsoft.WinGet/AdminSettings
    settings:
      LocalManifestFiles: true
      BypassCertificatePinningForMicrosoftStore: false
    metadata:
      winget:
        securityContext: elevated
```

Admin settings are distinct from user settings — they control machine-wide WinGet behaviors and require administrator privileges to modify.

### Microsoft.WinGet/Pin

```yaml
resources:
  - resource: Microsoft.WinGet/Pin
    settings:
      id: Microsoft.Edge
      pinType: blocking
      ensure: present
```

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `id` | string | Yes | Package identifier |
| `pinType` | enum | No | `pinning` (default) \| `blocking` \| `gating` |
| `version` | string | No | Version to pin to (for `gating`) |
| `ensure` | enum | No | `present` \| `absent` |

### CLI Commands Affected

The `winget dsc` subcommand already exists and implements the DSC v3 command-based resource protocol:

```
winget dsc get --resource <type>      # stdin: JSON, stdout: current state
winget dsc set --resource <type>      # stdin: JSON, stdout: result
winget dsc test --resource <type>     # stdin: JSON, stdout: test result
winget dsc schema --resource <type>   # stdout: JSON schema
winget dsc list                       # List available resources
```

> [!NOTE]
> `winget dsc` previously served as an alias for `winget configure`. With the existing resource protocol implementation in the client, `winget dsc` is now the dedicated entry point for DSC v3 resource operations. It is NOT intended for direct user interaction — it implements the DSC v3 protocol for the runtime to invoke. It does not appear in `winget --help` by default (hidden command, visible with `--verbose`).

### Configuration Export Enhancement

Enhance `winget configure export` to produce `Microsoft.WinGet/PackageList`:

```powershell
winget configure export --output current-state.dsc.yaml
```

Generates a valid configuration file using the native resources, enabling "golden image" workflows.

### Cross-Repository Impact

- **winget-cli** — New `winget dsc` subcommand, resource manifests shipped with the package
- **winget-dsc** — Existing PowerShell resources remain (not deprecated immediately); new native resources coexist under different type namespace (`Microsoft.WinGet/*` vs `Microsoft.WinGet.DSC/*`)
- **winget-pkgs** — No direct impact (validation pipeline unchanged)
- **winget-create** — No impact

### Group Policy

| Policy | Purpose | Default |
|--------|---------|---------|
| `EnableExactEnforcement` | Allow `PackageList` with `enforcement: exact` | Disabled |
| `DscResourceTimeout` | Maximum seconds per resource operation | 600 |

### Settings Changes

```json
{
  "dsc": {
    "resourceTimeout": 600,
    "enableExactEnforcement": false
  }
}
```

## UI/UX Design

### Configuration authoring (user-facing):

```yaml
# config.dsc.yaml - Developer workstation
properties:
  configurationVersion: 0.3
  resources:
    - resource: Microsoft.WinGet/PackageList
      directives:
        description: Developer tools
      settings:
        packages:
          - id: Git.Git
          - id: Microsoft.VisualStudioCode
            ensure: latest
          - id: Docker.DockerDesktop
          - id: Python.Python.3.12
    - resource: Microsoft.WinGet/Source
      directives:
        description: Add corporate source
      settings:
        name: corporate
        url: https://winget.contoso.com/api
        ensure: present
    - resource: Microsoft.WinGet/Settings
      directives:
        description: Configure WinGet preferences
      settings:
        ensure: merged
        settings:
          installBehavior:
            preferences:
              scope: machine
```

### Apply output:

```
> winget configure --file config.dsc.yaml
Configuration: Developer workstation
  [Microsoft.WinGet/PackageList] Developer tools
    ✓ Git.Git 2.45.1 - already installed
    ✓ Microsoft.VisualStudioCode 1.92.0 - upgraded from 1.91.0
    ✓ Docker.DockerDesktop 4.32.0 - installed
    ✓ Python.Python.3.12 3.12.4 - already installed
  [Microsoft.WinGet/Source] Add corporate source
    ✓ corporate - already configured
  [Microsoft.WinGet/Settings] Configure WinGet preferences
    ✓ Settings merged
Configuration complete. 6 resources in desired state.
```

### Test output:

```
> winget configure test --file config.dsc.yaml
Configuration: Developer workstation
  [Microsoft.WinGet/PackageList] Developer tools
    ✓ Git.Git - in desired state (2.45.1)
    ✗ Docker.DockerDesktop - not installed
    ✓ Microsoft.VisualStudioCode - in desired state (1.92.0)
    ✓ Python.Python.3.12 - in desired state (3.12.4)
  [Microsoft.WinGet/Source] Add corporate source
    ✓ corporate - in desired state
  [Microsoft.WinGet/Settings] Configure WinGet preferences
    ✗ installBehavior.preferences.scope - expected "machine", got "user"
Result: NOT in desired state (2 resources require changes)
```

## Capabilities

### Accessibility

No direct accessibility impact — DSC resources are declarative files processed by automation. CLI output during `winget configure` uses standard progress reporting accessible to screen readers.

### Security

- `PackageList` with `enforcement: exact` is gated by Group Policy to prevent accidental mass uninstallation
- `winget dsc` subcommand inherits existing WinGet security (source trust, hash validation, SmartScreen)
- Settings resource cannot disable security features unless Group Policy permits
- Resource manifests are signed as part of the WinGet MSIX package

### Reliability

- Native resources eliminate ~92s adapter overhead, making boot-time configurations viable
- Per-package failure isolation in `PackageList` — one failure does not abort the entire list
- Retry logic for transient failures (network timeouts, MSI mutex from Windows Update)
- `implementsPretest: true` in manifest — resources check state before applying, avoiding unnecessary changes

### Compatibility

- Existing `Microsoft.WinGet.DSC` PowerShell resources remain supported (not deprecated immediately)
- New resources use different type namespace (`Microsoft.WinGet/*` vs `Microsoft.WinGet.DSC/*`) — no conflicts
- Configuration files using old resources continue working through the adapter
- Schema version bump required for `winget configure export` to emit new resources (1.28.0 → 1.29.0)

### Performance, Power, and Efficiency

- Native resources: ~3-5s per operation (vs. ~92s through adapter)
- `PackageList` batches operations — concurrent MSIX installs (up to 6), sequential MSI
- No PowerShell process startup per resource invocation
- `implementsPretest` avoids redundant test+set round-trips

## Potential Issues

1. **`winget dsc` subcommand scope** — Must carefully define the stdin/stdout JSON contract to remain compatible with future DSC protocol versions.
2. **PackageList "exact" mode risk** — Incorrect configuration could remove critical software. Mitigated by GPO gate and dry-run via `winget configure test`.
3. **Version constraint resolution** — Supporting ranges (`>=`, `<=`, wildcards) adds complexity. Must define exact semver-like comparison semantics.
4. **Export fidelity** — `winget configure export` can only export WinGet-tracked packages. Sideloaded or manually installed software won't appear.
5. **SYSTEM context** — Resources must work in SYSTEM context (dependency on PowerShell Module Parity spec [#6288](https://github.com/microsoft/winget-cli/issues/6288)).
6. **Concurrent operation safety** — `PackageList` with parallel MSIX installs must handle the `_MSIExecute` mutex correctly and avoid conflicts with other WinGet instances.

## Future Considerations

- **Configuration drift detection** — Scheduled `winget configure test` with reporting for continuous compliance
- **Intune integration** — Native resources are faster and more reliable for Intune device configuration
- **Package dependency graphs** — `PackageList` could order installations based on inter-package dependencies
- **Remote configuration** — Combined with WinGet REST source enables fleet-wide management
- **Adapted resource manifests** — Once `Microsoft.WinGet.DSC` ships adapted resource manifests (`.dsc.adaptedResource.json`), the PowerShell resources also benefit from faster discovery

## Resources

- DSC v3 command-based resources: https://learn.microsoft.com/powershell/dsc/concepts/resources
- DSC v3 resource manifest schema: https://github.com/PowerShell/DSC/tree/main/schemas
- Current WinGet DSC module: https://github.com/microsoft/winget-dsc
- WinGet Configuration docs: https://learn.microsoft.com/windows/package-manager/configuration/
- Related: [#3401](https://github.com/microsoft/winget-cli/issues/3401), [#5806](https://github.com/microsoft/winget-cli/issues/5806)
- Performance data: Adapted resources ~92s via dscv3 processor vs. ~3-5s for native v3 resources
