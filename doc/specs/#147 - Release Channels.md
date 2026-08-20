---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-17
last updated: 2026-06-17
issue id: 147
---

# Release Channels

For [#147](https://github.com/microsoft/winget-cli/issues/147).

## Abstract

Add support for release channels (stable, beta, dev, canary, LTS, etc.) to the WinGet package model. Users subscribe to a channel for a package and receive upgrades only within that channel. This eliminates accidental cross-channel upgrades and enables proper management of packages with multiple concurrent release streams.

## Inspiration

Many popular packages publish multiple concurrent release streams:

- **Google Chrome** — Stable, Beta, Dev, Canary
- **Microsoft Edge** — Stable, Beta, Dev, Canary
- **VS Code** — Stable, Insiders
- **Node.js** — Current, LTS (multiple active LTS lines)
- **Firefox** — Release, Beta, Developer Edition, Nightly

Today in `winget-pkgs`, these are handled inconsistently:
- Some use separate package IDs (`Google.Chrome` vs `Google.Chrome.Beta` vs `Google.Chrome.Dev`)
- Some publish only the stable channel, with beta users unable to track updates
- `winget upgrade --all` may cross channel boundaries (beta→stable "downgrade")
- No way to express "keep me on LTS" without pinning to a specific version

This is one of the oldest open feature requests ([#147](https://github.com/microsoft/winget-cli/issues/147), opened 2020) and relates to side-by-side package support ([#2129](https://github.com/microsoft/winget-cli/issues/2129)).

## Solution Design

### Manifest Schema Extension

Add a `Channel` field to the version manifest:

> [!NOTE]
> The `Channel` field already exists in the installer manifest at the installer and root level. This spec extends the channel concept to the version level for subscription tracking and version resolution, while acknowledging that a single package version can exist in multiple channels simultaneously.

```yaml
PackageIdentifier: Google.Chrome
PackageVersion: 127.0.6533.0
Channel: Dev
ManifestType: version
ManifestVersion: 1.29.0
```

**Schema:**

| Property | Type | Required | Default | Description |
|----------|------|----------|---------|-------------|
| `Channel` | string | No | (empty = stable) | Channel name for this version |

- If `Channel` is omitted or empty, the version belongs to the implicit `stable` channel
- A single version can appear in multiple channels simultaneously (e.g., a build may be in both `dev` and `canary` at the same time)
- Channel names are case-insensitive, publisher-defined strings
- Recommended vocabulary: `stable`, `beta`, `dev`, `canary`, `lts`, `preview`, `insiders`, `nightly`, `rc`
- Maximum length: 32 characters
- Allowed characters: `[a-zA-Z0-9._-]`

**Channel detection challenge:**

When a package is installed or updated outside of WinGet, there is no reliable mechanism for WinGet to determine which channel the installed version belongs to. This varies by installer type:

- **MSIX/AppX** — Channel can often be identified from the package identity itself
- **MSI/EXE** — Registry entries (ARP) rarely encode channel information; the same version number may exist in multiple channels with no distinguishing metadata
- **External updates** — If an application self-updates, WinGet's channel subscription remains unchanged but the installed version may no longer align with the subscribed channel

The design must account for these ambiguities. WinGet's channel subscription is a user-expressed intent ("keep me on Dev") rather than a detected property of the installed binary. The tracking database records the user's subscription, and version resolution uses it to filter available upgrades — but WinGet cannot guarantee that the currently installed version is exclusively associated with the subscribed channel.

### Version Resolution Logic

The version resolver gains channel awareness:

**Current behavior (no channel specified by user):**
1. Only versions with no `Channel` field (or `Channel: stable`) are considered
2. Backwards compatible — existing behavior unchanged

**Explicit channel subscription:**
1. `winget install Google.Chrome --channel dev` — installs from the `dev` channel
2. The channel subscription is recorded in the local tracking database
3. Subsequent `winget upgrade Google.Chrome` only considers versions in the `dev` channel

**Channel switching:**
1. `winget install Google.Chrome --channel stable` while `dev` is subscribed → prompts: "Switch from Dev to Stable? This may install an older version."
2. With `--force`, switches without prompting

### Tracking Database

WinGet stores channel subscriptions in the installed packages database:

| Column | Type | Description |
|--------|------|-------------|
| `PackageId` | TEXT | Package identifier |
| `Channel` | TEXT | Subscribed channel (NULL = stable/default) |
| `SubscribedOn` | DATETIME | When the subscription was set |

This is stored alongside existing installation tracking data (ARP correlation, source, installed version).

### CLI Commands Affected

#### `winget install`

```
winget install <package> [--channel <channel>]
```

- New `--channel` argument selects the channel to install from
- If the package is already installed on a different channel, prompt for confirmation (or require `--force`)
- Records channel subscription on successful install

#### `winget upgrade`

```
winget upgrade [<package>] [--channel <channel>] [--include-channel]
```

- Default: only shows upgrades within subscribed channel
- `--channel` temporarily overrides for this operation (does not change subscription)
- `--include-channel` adds a Channel column to output

Behavior of `winget upgrade --all`:
- Each package is upgraded within its subscribed channel only
- Packages without channel subscription only see stable versions
- No cross-channel upgrades occur

#### `winget show`

```
winget show <package> [--channels] [--channel <channel>] [--versions]
```

- `--channels` lists all available channels with their latest version
- `--channel <channel>` shows versions within that channel
- `--versions` combined with `--channel` lists all versions in that channel

#### `winget list`

```
winget list [--include-channel]
```

- `--include-channel` adds Channel column showing subscribed channel per package

#### `winget export` / `winget import`

Export includes channel information:

```json
{
  "Sources": [{
    "Packages": [{
      "PackageIdentifier": "Google.Chrome",
      "Channel": "Dev"
    }]
  }]
}
```

Import respects channel — installs from the specified channel.

### Settings

```json
{
  "installBehavior": {
    "defaultChannel": "",
    "showChannelInList": false,
    "showChannelInUpgrade": true
  }
}
```

| Setting | CLI Argument | Description |
|---------|-------------|-------------|
| `defaultChannel` | `--channel` | Default channel for new installations (empty = stable) |
| `showChannelInList` | `--include-channel` | Always show channel column in `winget list` |
| `showChannelInUpgrade` | `--include-channel` | Always show channel column in `winget upgrade` |

### Group Policy

| Policy | Type | Default | Description |
|--------|------|---------|-------------|
| `AllowedChannels` | MultiString | Empty (all allowed) | Restrict which channels users can subscribe to |
| `DefaultChannel` | String | Empty (stable) | Override default channel for managed devices |
| `PreventChannelSwitching` | Bool | False | Block users from changing subscriptions |

### COM API Surface

```idl
// Extended PackageCatalogReference
interface IPackageVersionInfo2 : IPackageVersionInfo
{
    String Channel { get; };
}

// Install options
interface IInstallOptions2 : IInstallOptions
{
    String PreferredChannel { get; set; };
}

// Package channel enumeration
interface IPackageChannelInfo
{
    String Name { get; };
    PackageVersionId LatestVersion { get; };
    UInt32 VersionCount { get; };
}
```

### PowerShell Cmdlets

```powershell
# Install on a channel
Install-WinGetPackage -Id "Google.Chrome" -Channel "Dev"

# List with channel info (Channel is a property on the output object)
Get-WinGetPackage | Select-Object Id, Name, InstalledVersion, Channel

# See available channels
Show-WinGetPackage -Id "Google.Chrome" -Channels

# Upgrade within channel (automatic)
Update-WinGetPackage -Id "Google.Chrome"  # uses subscribed channel
```

### Migration Path for Existing Packages

Packages currently using separate IDs for channels (e.g., `Google.Chrome.Beta`) will need a migration path to the unified model. This is deferred to a future spec once the underlying mechanisms (such as package tombstones or alias resolution) are designed and implemented. See [Future Considerations](#future-considerations).

### WinGet Configuration / DSC

```yaml
resources:
  - resource: Microsoft.WinGet/Package
    settings:
      id: Google.Chrome
      channel: Dev
      ensure: latest
```

The `channel` property is added to the `Microsoft.WinGet/Package` resource schema.

### Validation Pipeline Impact

- `winget-pkgs` validation must handle manifests with `Channel` field
- Channel names are validated against the allowed character set
- Multiple versions of the same package with different channels can exist (this is the point)
- Existing submission workflows unchanged for packages not using channels

### Cross-Repository Impact

- **winget-cli** — Channel-aware version resolver, tracking database, CLI arguments, settings, GPO
- **winget-pkgs** — Schema validation for `Channel` field, parallel versions per channel allowed
- **winget-create** — Support `Channel` field in manifest creation/update
- **winget-cli-restsource** — REST API filter/query by channel

### Schema Version

Requires a new manifest schema version for the `Channel` field.

## UI/UX Design

### `winget show --channels`:

```
> winget show Google.Chrome --channels
Google Chrome [Google.Chrome]

Available channels:
  Channel    Latest Version     Versions
  ─────────────────────────────────────────
  Stable     127.0.6478.0       152
  Beta       127.0.6500.0       48
  Dev        127.0.6533.0       51
  Canary     128.0.6550.0       203
```

### `winget upgrade` with channels:

```
> winget upgrade --include-channel
The following packages have updates available:

Name           Id               Channel   Installed       Available       Source
──────────────────────────────────────────────────────────────────────────────────
Google Chrome  Google.Chrome    Dev       127.0.6510.0    127.0.6533.0    winget
Node.js        OpenJS.NodeJS    LTS       20.13.0         20.14.0         winget
VS Code        Microsoft.VS..   Insiders  1.91.0          1.92.0          winget

3 upgrades available (within subscribed channels).
```

### Channel switch prompt:

```
> winget install Google.Chrome --channel stable
Google Chrome [Google.Chrome] is currently installed from the Dev channel (127.0.6533.0).
The latest Stable version is 127.0.6478.0 (older than currently installed).

Switch from Dev to Stable? This will not downgrade the current installation,
but future upgrades will come from the Stable channel. [y/N]:
```

### `--disable-interactivity` with channel switch:

Channel switch fails with error — explicit user consent required. Use `--force` to override.

## Capabilities

### Accessibility

- Channel information is text-based, screen-reader accessible
- No color-only indicators for channel status
- Channel names are simple strings — no special characters that screen readers might misread

### Security

- Channels do not bypass security validation — all versions undergo the same SmartScreen/hash/signature checks
- GPO `AllowedChannels` prevents users from subscribing to unstable channels on managed devices
- No security regression from supporting multiple channels

### Reliability

- Eliminates accidental cross-channel upgrades (the primary reliability improvement)
- `winget upgrade --all` becomes safe for users on non-stable channels
- Channel subscription persists across WinGet updates

### Compatibility

- **Fully backwards compatible** — packages without `Channel` field behave exactly as today
- Older clients ignore the `Channel` field and see all versions in a flat list (they may offer cross-channel "upgrades" — existing behavior)
- The separate-ID pattern (`Google.Chrome.Beta`) continues to work indefinitely
- Schema version 1.28.x manifests without `Channel` are implicitly on `stable`

### Performance, Power, and Efficiency

- Channel filtering is a lightweight in-memory operation on the version list
- No additional network calls — channel is part of manifest metadata in the source index
- Tracking database addition is a single column — negligible storage impact

## Potential Issues

1. **Publisher adoption** — Requires publishers to add `Channel` fields. Until they do, separate-ID patterns persist. Community PRs can backfill.
2. **Channel name inconsistency** — Publishers may use `beta` vs `Beta` vs `preview`. Case-insensitive matching helps; recommended vocabulary in docs provides guidance.
3. **ARP correlation complexity** — Detecting which channel is installed from registry entries is hard (Chrome Dev and Chrome Stable have different ARP entries, but some apps don't differentiate). May need manifest-level `AppsAndFeaturesEntries` per channel.
4. **Version ordering** — A Dev version (128.0.x) is numerically "higher" than Stable (127.0.x). Channel filtering prevents cross-channel comparison, but UI must make this clear.
5. **REST source compatibility** — REST sources need schema updates to support channel-filtered queries efficiently. Until updated, REST sources serve all versions and client filters locally.
6. **Side-by-side installation** — This spec does NOT address installing multiple channels simultaneously (that's [#2129](https://github.com/microsoft/winget-cli/issues/2129)). A user subscribed to Dev cannot also have Stable installed via WinGet.

## Deprecation Path

For packages migrating from separate IDs to unified ID + Channel:

> [!NOTE]
> Migration from separate package IDs to the unified channel model requires mechanisms (such as package tombstones or alias resolution) that do not exist today. The phases below describe a conceptual progression — specific timelines and mechanisms are deferred to a future spec.

| Phase | Behavior |
|-------|----------|
| Introduction | Both old (separate ID) and new (Channel) exist side-by-side |
| Transition | Old ID shows deprecation notice, redirect resolution added |
| Deprecation | Old ID stops receiving new submissions |
| Removal | Never in 1.X — old IDs continue resolving |

Adoption will be driven by the winget-pkgs schema version policy (n/n-1) which encourages publishers to use the latest schema.

## Future Considerations

- **Migration from separate IDs** — Design alias resolution or tombstone-based redirection for packages currently using separate IDs per channel (e.g., `Google.Chrome.Beta` → `Google.Chrome` channel `Beta`). Tombstones may be part of the solution.
- **Cross-package channel dependencies** — Allow packages to depend on a specific channel of another package (e.g., an app that requires the LTS channel of Node.js)
- **LTS subscription semantics** — Distinguish between subscribing to a specific LTS line (e.g., Node.js 24.x) vs. "always latest LTS" (which could jump from 24.x to 26.x when the active LTS line changes)
- **Channel-aware pinning** — `winget pin add --channel stable` prevents channel drift
- **Side-by-side channels** — Install both Stable and Dev simultaneously (separate feature, requires different approach)
- **Channel notifications** — Alert when a new channel becomes available
- **Automatic channel fallback** — If a channel stops publishing, suggest switching to next-stable
- **REST API channel filtering** — Server-side filtering for efficiency

## Resources

- Original issue: https://github.com/microsoft/winget-cli/issues/147
- Side-by-side packages: https://github.com/microsoft/winget-cli/issues/2129
- npm dist-tags (similar concept): https://docs.npmjs.com/cli/dist-tag
- Chrome release channels: https://www.chromium.org/getting-involved/dev-channel/
- Node.js release schedule: https://nodejs.org/en/about/releases/
