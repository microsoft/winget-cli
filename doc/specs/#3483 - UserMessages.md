---
author: Demitrius Nelon denelon, GitHub Copilot <Copilot>
created on: 2026-05-03
last updated: 2026-05-03
issue id: 3483
---

# UserMessages

For [#3483](https://github.com/microsoft/winget-cli/issues/3483)

## Abstract

This specification proposes adding an optional `UserMessages` object to the WinGet package manifest schema. The new field provides structured, flow-dependent messaging to users before and after install, upgrade, uninstall, and repair operations. It is designed as a non-breaking, additive change that coexists with the existing `InstallationNotes` string field during a transition period and eventually replaces it.

## Inspiration

The existing `InstallationNotes` field is a single string displayed after a successful install. Community feedback in [#3483](https://github.com/microsoft/winget-cli/issues/3483) identified several limitations:

1. **No flow specificity.** The same message appears regardless of whether the user is installing, upgrading, or uninstalling. Upgrade and uninstall operations have no mechanism for user-facing notes at all.
2. **No pre-action messaging.** Some packages need to inform users of prerequisites, data-loss risks, or configuration steps before an operation begins. Today there is no way to surface this information through the manifest.
3. **No structured extensibility.** The single string cannot be extended to support new scenarios without breaking the schema contract.

Changing `InstallationNotes` from a string to an object (as originally proposed in #3483) would break existing manifests and clients. This specification introduces a new field alongside the existing one, providing a clean migration path.

## Solution Design

### Schema Changes

A new optional `UserMessages` object is added to the **defaultLocale** and **locale** manifest types. The field is localizable — locale-specific manifests may override any or all messages defined in the defaultLocale manifest, following the existing locale fallback behavior.

```json
"UserMessages": {
    "type": ["object", "null"],
    "description": "Optional messages displayed to the user before or after specific package operations.",
    "properties": {
        "PreInstall": {
            "type": ["string", "null"],
            "minLength": 1,
            "maxLength": 2048,
            "description": "Confirmation request displayed to the user before a package installation begins."
        },
        "PostInstall": {
            "type": ["string", "null"],
            "minLength": 1,
            "maxLength": 10000,
            "description": "Message displayed to the user upon completion of a package installation. This is the designated successor to InstallationNotes."
        },
        "PreUpgrade": {
            "type": ["string", "null"],
            "minLength": 1,
            "maxLength": 2048,
            "description": "Confirmation request displayed to the user before a package upgrade begins."
        },
        "PostUpgrade": {
            "type": ["string", "null"],
            "minLength": 1,
            "maxLength": 10000,
            "description": "Message displayed to the user upon completion of a package upgrade."
        },
        "PreUninstall": {
            "type": ["string", "null"],
            "minLength": 1,
            "maxLength": 2048,
            "description": "Confirmation request displayed to the user before a package uninstall begins."
        },
        "PostUninstall": {
            "type": ["string", "null"],
            "minLength": 1,
            "maxLength": 10000,
            "description": "Message displayed to the user upon completion of a package uninstall."
        },
        "PreRepair": {
            "type": ["string", "null"],
            "minLength": 1,
            "maxLength": 2048,
            "description": "Confirmation request displayed to the user before a package repair begins."
        },
        "PostRepair": {
            "type": ["string", "null"],
            "minLength": 1,
            "maxLength": 10000,
            "description": "Message displayed to the user upon completion of a package repair."
        }
    },
    "additionalProperties": false
}
```

> [!NOTE]
> **Field limits:** Pre-action messages use a lower `maxLength` of 2048 characters because they gate the operation with a user prompt in interactive mode. Shorter messages improve readability and reduce the risk of users dismissing important information. Post-action messages retain the 10000-character limit consistent with the existing `InstallationNotes` field.

### Schema Version

This change requires a new manifest schema version that aligns with the client version at the time of implementation. The `UserMessages` field is added to:

- `defaultLocale.schema.json`
- `locale.schema.json`

The `installer.schema.json` is not affected. User-facing messages are locale-sensitive content and belong in the locale manifests.

### Existing `InstallationNotes` Field

The existing `InstallationNotes` field is **unchanged** in this schema version. It continues to function as it does today. During the transition period, manifests should include both fields for backward compatibility.

### Client Behavior

#### Precedence Rules

When both `InstallationNotes` and `UserMessages.PostInstall` are present in a manifest:

- **Clients that support `UserMessages`:** Display `UserMessages.PostInstall` and ignore `InstallationNotes`.
- **Clients that do not support `UserMessages`:** Display `InstallationNotes` (current behavior, unaffected by the unknown field).

> [!NOTE]
> Each operation displays only its corresponding `UserMessages` field. There is no cross-field fallback between operations — for example, if a manifest defines `PreInstall` but not `PreUpgrade`, no pre-action message is displayed during an upgrade.

#### Uninstall Message Sourcing

For uninstall operations, `PreUninstall` and `PostUninstall` are sourced as follows:

- If the package is correlated to a specific installed version, use that version's manifest messages.
- If the package is correlated but no target version manifest exists, use the latest available version's messages.
- If the package is not correlated (for example, it was not installed through WinGet), no uninstall message is displayed.

#### Pre-Action Messages (PreInstall, PreUpgrade, PreUninstall, PreRepair)

Pre-action messages are displayed **before** the operation begins and represent information the user should be aware of before proceeding.

**Interactive mode (default):**

```
The following information has been provided by the manifest author:

  This package requires 2 GB of disk space and will modify your PATH.

Do you wish to continue? [Y/n]:
```

The user is prompted to confirm. Entering `N` cancels the operation. Entering `Y` or pressing Enter proceeds.

> [!IMPORTANT]
> **Automation impact:** The introduction of pre-action prompts in interactive mode will affect existing automation scripts that do not use `--disable-interactivity`. Automation authors should ensure their scripts pass `--disable-interactivity` or use the PowerShell module with `-Force` to avoid being blocked by prompts. The COM API does not prompt and is unaffected.

**Non-interactive mode (`--disable-interactivity`):**

The pre-action message is displayed (rendered to the output stream) but no prompt is shown and the operation proceeds automatically. This ensures automation pipelines see the message in logs without being blocked.

```
The following information has been provided by the manifest author:

  This package requires 2 GB of disk space and will modify your PATH.

Proceeding automatically (non-interactive mode)...
```

> [!NOTE]
> **Design rationale:** Pre-action messages are informational, not contractual agreements. Unlike license terms (which require explicit acceptance via `--accept-package-agreements`), user messages are manifest-author-provided guidance. The `--disable-interactivity` flag is the appropriate suppressor because it already governs all interactive prompts in WinGet. A new flag is not warranted.

#### Post-Action Messages (PostInstall, PostUpgrade, PostUninstall, PostRepair)

Post-action messages are displayed **after** the operation completes successfully. They are informational only — no prompt is shown.

```
Successfully installed ExampleApp [Publisher.ExampleApp] v2.0.0

  Restart your terminal to use the CLI.
```

Post-action messages are displayed in both interactive and non-interactive modes. They are suppressed when the `userExperience.userMessages.disablePostActionMessages` setting is enabled.

#### Flow Behavior Matrix

| Scenario | Pre-message | Post-message | Notes |
|----------|-------------|--------------|-------|
| `winget install` | `PreInstall` Displayed; Prompted (Y/n) | `PostInstall` Displayed | Standard single-package install |
| `winget upgrade <package>` | `PreUpgrade` Displayed; Prompted (Y/n) | `PostUpgrade` Displayed | Single-package upgrade |
| `winget upgrade --all` | All `PreUpgrade` messages Displayed together; Prompted (Y/n) once | Each `PostUpgrade` Displayed | See "Bulk Operations" below |
| `winget uninstall` | `PreUninstall` Displayed; Prompted (Y/n) | `PostUninstall` Displayed | Uses uninstall message sourcing rules above |
| `winget install --disable-interactivity` | `PreInstall` Displayed; Prompt Suppressed | `PostInstall` Displayed | Non-interactive install |
| Dependency packages | Suppressed | Suppressed | Messages apply to the root package only, not transitive dependencies |
| `winget import` | All `Pre*` messages Displayed together; Prompted (Y/n) once | Each `Post*` Displayed | Follows the same batch model |
| WinGet Configuration (DSC) | Displayed | Displayed | Configuration is inherently non-interactive |
| `winget show` | N/A | N/A | `show` displays `UserMessages` as package metadata (see below) |
| `winget repair` | `PreRepair` Displayed; Prompted (Y/n) | `PostRepair` Displayed | Repair has dedicated fields |
| `winget export` | N/A | N/A | Export captures package list, not messages |
| Operation failure | `Pre*` Displayed before attempt | Suppressed | Post-messages only appear on success |
| `disableInstallNotes` setting enabled | `Pre*` Displayed | `PostInstall` Suppressed; other `Post*` Displayed | Backward compatibility behavior for migrated install notes |
| `userExperience.userMessages.disablePostActionMessages` enabled | `Pre*` Displayed | `Post*` Suppressed | New setting for new fields |
| `userExperience.userMessages.disablePreActionMessages` enabled | Suppressed; Prompt Suppressed | `Post*` Displayed | Suppresses both display and prompt |
| `--show-user-messages` | `Pre*` Displayed; Prompted (Y/n) in interactive mode | `Post*` Displayed | Overrides settings that suppress messages for this invocation |
| `--ignore-user-messages` | Suppressed; Prompt Suppressed | `Post*` Suppressed | CLI argument overrides settings for this invocation |
| `--ignore-pre-action-messages` | Suppressed; Prompt Suppressed | `Post*` Displayed | CLI argument overrides `userExperience.userMessages.disablePreActionMessages` |
| `--ignore-post-action-messages` | `Pre*` Displayed | `Post*` Suppressed | CLI argument overrides `userExperience.userMessages.disablePostActionMessages` |
| `winget validate` | Suppressed | Suppressed | Validation checks schema correctness only; messages are not rendered |

#### Bulk Operations (`upgrade --all`, `import`)

For bulk operations, pre-action messages for all packages in the batch are collected and presented together before any operations begin.

In interactive mode, the user is prompted once after seeing the full set of pre-action messages. This is consistent with how preconditions are already handled in WinGet.

### Settings

A new `userExperience.userMessages` settings object is introduced in the WinGet settings file. The `userExperience` section is a new organizational grouping that could eventually house other user-facing settings such as `visual` and `interactivity` in a future settings schema revision.

```json
{
    "userExperience": {
        "userMessages": {
            "disablePreActionMessages": false,
            "disablePostActionMessages": false
        }
    }
}
```

| Setting | Default | Description |
|---------|---------|-------------|
| `userExperience.userMessages.disablePreActionMessages` | `false` | When `true`, suppresses all pre-action messages (`PreInstall`, `PreUpgrade`, `PreUninstall`, `PreRepair`) and their associated prompts. The operation proceeds as if the user confirmed. |
| `userExperience.userMessages.disablePostActionMessages` | `false` | When `true`, suppresses all post-action messages (`PostInstall`, `PostUpgrade`, `PostUninstall`, `PostRepair`). |

**Interaction with existing settings:**

Since `UserMessages.PostInstall` takes precedence over `InstallationNotes` on supporting clients (as described in the Precedence Rules section), the settings that affect post-action messages are:

- `userExperience.userMessages.disablePostActionMessages` — suppresses `UserMessages.Post*`
- `disableInstallNotes` — also suppresses `UserMessages.PostInstall` for backward compatibility (see below)

For backward compatibility, when `disableInstallNotes` is `true`, it also suppresses `UserMessages.PostInstall`. This ensures users who previously disabled install notes continue to have a consistent experience when manifests migrate to the new field.

On **older clients** that do not support `UserMessages`, only `InstallationNotes` is relevant and `disableInstallNotes` controls it as it does today.

### CLI Arguments

Corresponding CLI arguments are provided so users can override the settings on a per-invocation basis.

| Argument | Applies to | Description |
|----------|-----------|-------------|
| `--ignore-user-messages` | `install`, `upgrade`, `uninstall`, `repair`, `import` | Suppresses all `UserMessages` (both pre- and post-action) for this invocation. Pre-action prompts are skipped and the operation proceeds. |
| `--ignore-pre-action-messages` | `install`, `upgrade`, `uninstall`, `repair`, `import` | Suppresses pre-action messages and their prompts for this invocation. Post-action messages are still displayed. |
| `--ignore-post-action-messages` | `install`, `upgrade`, `uninstall`, `repair`, `import` | Suppresses post-action messages for this invocation. Pre-action messages and prompts are still displayed. |
| `--show-user-messages` | `install`, `upgrade`, `uninstall`, `repair`, `import` | Forces all `UserMessages` to be displayed for this invocation, overriding any settings that disable them. |

**Precedence:** CLI arguments override settings. If a user has `userExperience.userMessages.disablePreActionMessages` set to `false` in settings but passes `--ignore-pre-action-messages`, the pre-action messages are suppressed for that invocation. `--show-user-messages` overrides settings but does not override `--ignore-user-messages` if both are passed; ignore wins.

**Relationship to `--disable-interactivity`:** The `--disable-interactivity` flag suppresses the **prompt** on pre-action messages but still displays the message text. The `--ignore-pre-action-messages` flag suppresses both the message and the prompt entirely.

Example usage:

```
winget upgrade --all --ignore-pre-action-messages
winget install Publisher.ExampleApp --ignore-user-messages
winget install Publisher.ExampleApp --show-user-messages
```

### COM API

The `UserMessages` data will be available in the COM API as eight string properties (one per message field). The specific interface shape and versioning will be determined during implementation, following the established WinGet pattern for additive COM API changes.

COM consumers are responsible for their own interactivity model. The WinGet COM API does not prompt — it returns the data and the consumer decides how to handle it.

The `userExperience.userMessages` settings should also be exposed via the COM API so that well-behaved callers (such as the WinGet PowerShell module) can query the user's settings and make informed decisions about whether to display messages.

### PowerShell Cmdlets

The WinGet PowerShell module cmdlets consume the COM API and must surface `UserMessages` appropriately.

#### Affected Cmdlets

| Cmdlet | Pre-message field | Post-message field |
|--------|-------------------|-------------------|
| `Install-WinGetPackage` | `PreInstall` | `PostInstall` |
| `Update-WinGetPackage` | `PreUpgrade` | `PostUpgrade` |
| `Uninstall-WinGetPackage` | `PreUninstall` | `PostUninstall` |
| `Repair-WinGetPackage` | `PreRepair` | `PostRepair` |

#### Interactive Behavior

When running interactively in a PowerShell session, cmdlets follow the same pattern as the CLI:

```powershell
PS> Install-WinGetPackage -Id Publisher.ExampleApp

The following information has been provided by the manifest author:

  This package requires 2 GB of disk space and will modify your PATH.

Do you wish to continue?
[Y] Yes  [N] No  (default is "Y"):
```

The prompt uses `$Host.UI.PromptForChoice` (or equivalent) to integrate with PowerShell's native prompting, supporting `-Confirm` and `-WhatIf` patterns where applicable.

#### Non-Interactive / Pipeline Behavior

When running non-interactively (e.g., in a script with no host, or when the user passes `-Force`), pre-action messages are written to the verbose stream (`Write-Verbose`) and the operation proceeds without prompting:

```powershell
PS> Install-WinGetPackage -Id Publisher.ExampleApp -Force -Verbose
VERBOSE: Manifest author message (PreInstall): This package requires 2 GB of disk space and will modify your PATH.
```

Post-action messages are written to the information stream (`Write-Information`) so they appear by default but can be suppressed with `-InformationAction SilentlyContinue`.

#### Output Object

The result objects returned by cmdlets should include UserMessages in their output when present:

```powershell
PS> $result = Install-WinGetPackage -Id Publisher.ExampleApp
PS> $result.UserMessages

PreInstall  : This package requires 2 GB of disk space and will modify your PATH.
PostInstall : Restart your terminal to use the CLI.
```

This allows scripts to programmatically inspect messages after an operation:

```powershell
$result = Install-WinGetPackage -Id Publisher.ExampleApp -Force
if ($result.UserMessages.PostInstall) {
    Write-Host "Note from manifest author: $($result.UserMessages.PostInstall)" -ForegroundColor Yellow
}
```

#### Find-WinGetPackage

`Find-WinGetPackage` should include `UserMessages` in its output when displaying package metadata from the source, allowing users to review messages before deciding to install.

```powershell
PS> Find-WinGetPackage -Id Publisher.ExampleApp | Select-Object -ExpandProperty UserMessages

PreInstall   : This package requires 2 GB of disk space and will modify your PATH.
PostInstall  : Restart your terminal to use the CLI.
PreUpgrade   : Back up your configuration file before upgrading.
PostUpgrade  : Review the changelog for breaking changes.
PreRepair    : Close the application before starting repair.
PostRepair   : Restart the application to verify the repair completed successfully.
```

### Validation Pipeline (winget-pkgs)

The `winget validate` command and the automated validation pipeline in the winget-pkgs repository must handle `UserMessages` correctly:

1. **Schema validation:** Validate `UserMessages` against the JSON schema (field types, lengths, required/optional).
2. **Non-interactive execution:** The validation pipeline runs with `--disable-interactivity`. Pre-messages must not block validation.
3. **Content validation:** The pipeline should validate that:
   - Messages do not contain ANSI escape sequences or control characters.
   - Messages do not contain URLs that appear to be phishing attempts (leveraging existing content moderation if applicable).
4. **Deprecation warnings:** During the transition period, if a manifest contains `InstallationNotes` but no `UserMessages.PostInstall`, a warning (not an error) may be emitted suggesting the manifest author add the `UserMessages` equivalent.

### Additional CLI Commands

#### `winget show`

The `winget show` command displays package metadata. When `UserMessages` is present, the messages are displayed as part of the package details:

```
Found Example App [Publisher.ExampleApp] v2.0.0
...
Installer:
  Type: Exe
  ...
User Messages:
  Pre-Install: This package requires 2 GB of disk space and will modify your PATH.
  Post-Install: Restart your terminal to use the CLI.
  Pre-Upgrade: Back up your configuration file before upgrading.
  Pre-Repair: Close the application before starting repair.
  Post-Repair: Restart the application to verify the repair completed successfully.
```

No prompt is displayed — `show` is read-only.

#### `winget repair`

The `repair` command uses the dedicated `PreRepair` and `PostRepair` fields. Its behavior mirrors the other operation-specific flows — pre-message with prompt in interactive mode, post-message after success.

#### `winget export`

The `export` command captures the list of installed packages. `UserMessages` are not included in the export output since they are part of the source manifest, not the installed state.

### WinGet Configuration (DSC)

WinGet Configuration is inherently non-interactive. When a configuration file references a package that has `UserMessages`:

- **Pre-messages** are written to the configuration log output as informational entries. They do not block or prompt.
- **Post-messages** are written to the configuration log output after the resource completes successfully.
- The COM API provides the message data so the configuration engine can access it programmatically.

### Group Policy

This specification does not introduce new Group Policy settings. Group Policy controls for `UserMessages` (e.g., enforcing or suppressing messages across managed devices) are deferred to a future specification. The existing Group Policy infrastructure is not affected.

### REST Source (winget-cli-restsource)

The [winget-cli-restsource](https://github.com/microsoft/winget-cli-restsource) reference implementation must be updated to support the new `UserMessages` fields so enterprise REST sources can serve this data. Changes include:

- **Schema update** — Add `UserMessages` fields to the REST source data model and API response contracts.
- **API versioning** — Introduce a new API version that includes `UserMessages` in the package version response. Older API versions continue to function without the new fields.
- **Storage** — The backing data store (Azure Cosmos DB / SQL) must accommodate the new fields.

> [!NOTE]
> Enterprise REST sources do not carry the same deprecation concerns as the WinGet Community Repository. Organizations control their own source update cadence and can adopt new schema fields on their own timeline.

### Community Repository (winget-pkgs)

The [winget-pkgs](https://github.com/microsoft/winget-pkgs) community repository is impacted in two ways:

1. **Validation pipeline** — The automated validation tooling must be updated to:
   - Accept `UserMessages` fields in manifests using the new schema version.
   - Validate field lengths (2048 for pre-action, 10000 for post-action) and structure.
   - Ensure validation runs are non-interactive — pre-action messages must not block the pipeline.

2. **Schema version acceptance policy** — The community repository accepts manifests at schema version **n** or **n-1**. As the minimum accepted version advances to include the `UserMessages` schema, manifest authors will be required to adopt the new fields and migrate away from `InstallationNotes`. This is the practical mechanism that drives deprecation of the older field.

### Manifest Examples

#### Transition period — both fields for compatibility

```yaml
# yaml-language-server: $schema=https://aka.ms/winget-manifest.defaultLocale.1.X.0.schema.json

PackageIdentifier: Publisher.ExampleApp
PackageVersion: 2.0.0
PackageLocale: en-US
PackageName: Example App
ShortDescription: An example application.
InstallationNotes: "Restart your terminal to use the CLI."
UserMessages:
  PreInstall: "This package requires 2 GB of disk space and will modify your PATH."
  PostInstall: "Restart your terminal to use the CLI."
  PreUpgrade: "Back up your configuration file at ~/.example/config.yaml before upgrading."
  PostUpgrade: "Review the changelog at https://example.com/changelog for breaking changes."
  PreUninstall: "Run 'example cleanup' to remove cached data before uninstalling."
  PostUninstall: "Restart your terminal to remove the CLI from your PATH."
  PreRepair: "Close the application before starting repair."
  PostRepair: "Restart the application to verify the repair completed successfully."
ManifestType: defaultLocale
ManifestVersion: 1.X.0
```

#### Pre-install warning only

```yaml
InstallationNotes: "Restart your terminal to use the CLI."
UserMessages:
  PreInstall: "This package requires 2 GB of disk space and will modify your PATH."
  PostInstall: "Restart your terminal to use the CLI."
```

#### Legacy only — no change required

```yaml
InstallationNotes: "Restart your terminal to use the CLI."
```

Existing manifests are unaffected. `UserMessages` is entirely optional.

## UI/UX Design

### Pre-Action Message Display

Pre-action messages are displayed in a visually distinct block before the operation begins. The message is attributed to the manifest author to establish trust context.

```
Found Example App [Publisher.ExampleApp] v2.0.0
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.

The following information has been provided by the manifest author:

  This package requires 2 GB of disk space and will modify your PATH.

Do you wish to continue? [Y/n]:
```

### Post-Action Message Display

Post-action messages appear after the success message, indented and visually separated.

```
Successfully installed Example App [Publisher.ExampleApp] v2.0.0

  Restart your terminal to use the CLI.
```

### Message Attribution

All messages include clear attribution language ("provided by the manifest author") to ensure users understand the content originates from the manifest author, not from Microsoft or WinGet. This distinction is important because in the WinGet community repository, manifests are frequently authored by community contributors rather than the software publisher. This is critical for trust and security.

### Localization

Messages in locale manifests override those in the defaultLocale manifest on a per-field basis. If a locale manifest provides `UserMessages.PreInstall` but not `UserMessages.PostInstall`, the client falls back to the defaultLocale value for `PostInstall`.

Locale manifests may provide different message content per locale, following the existing localization model. Ensuring that translations are accurate and not misleading is the responsibility of the manifest author. A future enhancement to the validation infrastructure could automate checks for potentially misleading translations across locales.

## Capabilities

### Accessibility

- Pre-action messages and prompts are rendered as standard text output, compatible with screen readers.
- The Y/n prompt follows existing WinGet prompt patterns for consistency with assistive technology expectations.
- Post-action messages are rendered as standard text, identical to current `InstallationNotes` behavior.
- No color-only information is used; messages are plain text.

### Security

- **Phishing and social engineering:** Pre-action messages create a stronger social-engineering surface than post-action notes because they appear before the user commits to an action. A malicious manifest author could craft a message like "Enter your password at https://evil.example.com to continue." Mitigations:
  - Clear attribution language ("provided by the manifest author") helps users assess trust.
  - Content validation in the winget-pkgs pipeline strips or rejects ANSI escape sequences and control characters.
  - The winget-pkgs community review process provides human review of manifest content.
  - Future enhancement: automated content moderation for suspicious URLs or social-engineering patterns.
- **Content sanitization:** All message fields must have ANSI escape sequences and control characters stripped before display to prevent terminal manipulation.
- **No elevation bypass:** Displaying a message does not grant any additional permissions. The prompt is a user experience feature, not a security boundary.

### Reliability

- The feature is additive and optional. No existing behavior changes unless the manifest includes `UserMessages`.
- Clients that do not understand `UserMessages` ignore it (unknown fields are skipped in manifest parsing).
- Schema validation catches malformed `UserMessages` objects at submission time in the winget-pkgs pipeline.

### Compatibility

- **No breaking changes.** `InstallationNotes` is unchanged. Existing manifests and existing clients continue to work exactly as they do today.
- **Older clients:** Ignore the `UserMessages` field (unknown fields are silently ignored in manifest parsing). They continue to display `InstallationNotes` if present.
- **Newer clients:** Read `UserMessages` and prefer `UserMessages.PostInstall` over `InstallationNotes` when both are present.
- **Schema version:** The new schema version must align with the client version at the time of implementation. Clients that cannot parse manifests at this schema version will receive manifests at the highest version they support from the source. In the winget-pkgs repository, manifests can exist at multiple schema versions to support this.
- **Minimum client version:** Only clients at or above the version that implements this specification will process `UserMessages`. The exact version will be determined during implementation.

### Performance, Power, and Efficiency

- Negligible impact. The feature adds string parsing and display of up to eight optional text fields. No network calls, no computation, no background processing.
- Pre-action prompts add user wait time in interactive mode only.

## Potential Issues

1. **Prompt fatigue.** If many packages include pre-action messages, users may develop "prompt blindness" and reflexively confirm without reading. Mitigation: the 2048-character limit keeps messages concise, and community review in winget-pkgs ensures messages are meaningful.

2. **Bulk operation noise.** `upgrade --all` with many packages containing pre-messages could still be verbose even when those messages are presented together before execution.

3. **Transition-period duplication.** During the transition, manifest authors must duplicate their install note in both `InstallationNotes` and `UserMessages.PostInstall`. This is a known trade-off for backward compatibility and is temporary.

4. **Content moderation burden.** The winget-pkgs community reviewers now have eight new fields to review per manifest. Automated tooling to flag suspicious content in these fields would reduce this burden.

5. **Locale fallback complexity.** Per-field fallback from locale to defaultLocale adds implementation complexity. However, this follows the existing pattern for all other localizable fields and is well-understood.

## Deprecation Path for `InstallationNotes`

| Phase | Client behavior | Manifest guidance |
|-------|----------------|-------------------|
| **1. Introduction** | Newer clients read `UserMessages`; older clients read `InstallationNotes` | Include both fields. Duplicate the install note in `InstallationNotes` and `UserMessages.PostInstall`. |
| **2. Transition** | Older client versions age out of support | Continue including both fields. Document `InstallationNotes` as deprecated in schema docs. |
| **3. Deprecation** | All supported clients understand `UserMessages` | `InstallationNotes` marked deprecated. Manifests may omit it. Validation warns but does not reject. |
| **4. Removal** (future) | N/A | `InstallationNotes` removed in a future schema version. `UserMessages.PostInstall` is the canonical replacement. |

The timeline for each phase is driven by client adoption telemetry, not fixed dates. Phase 4 (removal) is explicitly deferred to a future specification.

> [!IMPORTANT]
> There is no intent to make breaking changes in WinGet 1.X. The WinGet CLI will continue to function normally with manifests that use `InstallationNotes`, regardless of deprecation phase. The practical driver for deprecation is the **WinGet Community Repository** (winget-pkgs) schema version policy. The community repository requires new manifest submissions to use schema version **n** or **n-1** (the current or immediately prior version). As the repository moves forward to schema versions where `UserMessages` is the expected field, publishers and manifest authors will be required to adopt the new field and stop using `InstallationNotes` as a condition of submitting to the community repository — not because the client rejects it, but because the repository's validation pipeline enforces the current schema versions.

## Future Considerations

The `UserMessages` object is designed to be extended. Potential future additions include:

- **`PostReboot`** — Message displayed after a reboot triggered by the package.
- **`PreDownload` / `PostDownload` ** — Message displayed before the package is downloaded, if different from pre-install. Also applies to `winget download`
- **`Configuration`** — Message displayed when the package is processed as part of a WinGet Configuration.
- **Structured message types** — Replacing plain strings with objects that include severity, category, or formatting hints.
- **Rich formatting** — Support for basic markdown or structured text in messages (requires careful consideration of terminal rendering).
- **Group Policy controls** — Admin policies to enforce or suppress user messages across managed devices.

New keys can be added in future schema versions without breaking existing manifests.

## Resources

- [#3483 — Make InstallationNotes flow-dependent](https://github.com/microsoft/winget-cli/issues/3483) — Original community request by @Trenly.
- [WinGet Manifest Schema Documentation](https://learn.microsoft.com/windows/package-manager/) — Current schema reference.
- [WinGet CLI Settings](https://github.com/microsoft/winget-cli/blob/master/doc/Settings.md) — Settings documentation including `disableInstallNotes`.
- [Manifest Schema JSON](https://github.com/microsoft/winget-cli/tree/master/schemas/JSON/manifests/) — JSON schema source files.
