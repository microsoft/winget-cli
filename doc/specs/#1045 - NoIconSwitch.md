---
author: Quan Wei <dakehero>, Doubao Seed via Oh My Pi
created on: 2026-09-22
last updated: 2026-09-22
issue id: 1045
---

# Named Installer Switch for Suppressing Desktop Shortcuts (`NoIcon`)

For [#1045](https://github.com/microsoft/winget-cli/issues/1045)

## Abstract

This spec introduces a first-class, named installer switch — `NoIcon` — that lets a manifest declare the exact argument which suppresses creation of a **desktop** shortcut, a dedicated `--no-icon` argument for `install` and `upgrade`, and corresponding `preferences`/`requirements` settings. WinGet never heuristically deletes desktop content; the switch value is passed to an installer only when that installer is known to support it. A known default is provided for Inno Setup so the capability is effective immediately for the largest affected installer family.

## Inspiration

Issue [#1045](https://github.com/microsoft/winget-cli/issues/1045) (opened 2021, 51+ upvotes) describes the recurring pain: `winget upgrade --all` and new installs recreate desktop shortcuts, forcing manual cleanup. Related issue [#6429](https://github.com/microsoft/winget-cli/issues/6429) asks for shortcut-location control, and [#3314](https://github.com/microsoft/winget-cli/issues/3314) is the inverse request.

The WinGet team defined the accepted direction in [this comment (2026-07-27)](https://github.com/microsoft/winget-cli/issues/1045#issuecomment-5094833020):

> 1. **Manifest schema** – a named installer switch for shortcut suppression …
> 2. **A dedicated argument** (e.g., `--no-icon`) to key that switch – deliberately opt-in, and **not** folded into the `silent`/`interactive` defaults …
> 3. **A corresponding setting** – … a user *prefer* or *require* the behavior, applied when the package/installer supports it.

This spec implements exactly that model. It does **not** attempt a blanket "no shortcut" behavior: shortcuts are created by each package's installer (or by the app on first run), and WinGet has no universal, safe hook to suppress or remove them.

## Solution Design

### Naming

| Surface | Name |
| --- | --- |
| Manifest field (YAML / REST JSON) | `Switches.NoIcon` |
| C++ enum | `AppInstaller::Manifest::InstallerSwitchType::NoIcon` |
| CLI argument | `--no-icon` |
| Settings | `installBehavior.preferences.noIcon` / `installBehavior.requirements.noIcon` |
| COM (`InstallOptions`) | `Boolean NoIcon` |
| PowerShell | `-NoIcon` switch on `Install-WinGetPackage` / `Update-WinGetPackage` |

"Icon" follows the existing installer terminology (Inno's `desktopicon` task; the `--no-icon` name suggested by the maintainer). It specifically means the **desktop** shortcut; Start Menu shortcuts are out of scope (tracked by #6429).

### 1. Manifest schema changes

A new optional `NoIcon` member is added to the installer `Switches` object, alongside `Silent`, `InstallLocation`, `Custom`, `Repair`, etc.

`schemas/JSON/manifests/latest/manifest.installer.latest.json`:

```json
"NoIcon": {
  "type": [ "string", "null" ],
  "minLength": 1,
  "maxLength": 512,
  "description": "NoIcon is the value that should be passed to the installer to suppress creation of a desktop shortcut"
}
```

The identical property is added to the `InstallerSwitches` definition in `manifest.singleton.latest.json`.

This is an additive, optional field. Older clients ignore it silently per the manifest compatibility contract.

**Schema versioning.** The change requires a new minor manifest version:

- `src/binver/binver/version.h`: `VERSION_MINOR` `30` → `31`.
- Run `schemas/JSON/manifests/Checkpoint-LatestManifestSchema.ps1 -BumpVersion`. This freezes the current in-flight schema into `v1.30.0/`, advances `latest/` to **1.31.0**, and updates `ManifestCommon.h`, `ManifestSchema.h`, `ManifestSchema.rc`, `ManifestSchemaValidation.cpp`, and `WinGetUtilInterop/Manifest/ManifestVersion.cs` (steps 1–10 of Workflow A in `schemas/JSON/manifests/README.md`).

**Plumbing** (mirroring every existing switch):

- `InstallerSwitchType` enum gains `NoIcon` (`src/AppInstallerCommonCore/Public/winget/ManifestCommon.h`).
- `InstallerSwitchTypeToString` gains the `NoIcon` → `"NoIcon"` case (`ManifestCommon.cpp`).
- YAML: `GetSwitchesFieldProcessInfo` gains `{ "NoIcon", ... }` for `Major() == 1` when `manifestVersion >= 1.31` (`ManifestYamlPopulator.cpp`).
- YAML round-trip: `ProcessInstallerSwitches` in `YamlWriter.cpp`.
- REST: `DeserializeInstallerSwitches` in `ManifestDeserializer_1_0.cpp`, exposed via a new REST schema override (the `1_7` override adding `Repair` is the template); `winget-cli-restsource` gains the field in a follow-up PR.
- WinGetUtil C# model: `src/WinGetUtilInterop/Manifest/V1/InstallerSwitches.cs` gains `NoIcon`, including `Equals`/`GetHashCode`.

### 2. Known default for Inno Setup

`GetDefaultKnownSwitches(InstallerTypeEnum::Inno)` gains:

```cpp
{InstallerSwitchType::NoIcon, ManifestInstaller::string_t("/MERGETASKS=!desktopicon")},
```

Rationale:

- `desktopicon` is the standard task name emitted by Inno Setup's own script templates for the optional desktop icon. `/MERGETASKS=!desktopicon` keeps all default tasks and deselects only that one — it does not depend on and does not alter the `Silent`/`Interactive` experience switches.
- If an installer script has no `desktopicon` task, the unrecognized task name is ignored by Inno Setup; the argument is harmless. This will be verified during implementation with an Inno script containing no desktop-icon task (compiled with `JRSoftware.InnoSetup`).
- This makes the capability immediately available for the whole Inno family without waiting for per-package manifest updates.

No default is defined for `Nullsoft`, `Burn`, bare `Exe`, or `Msi`: their shortcut flags are vendor-specific. Such packages support the feature only by declaring an explicit `NoIcon` switch (real-world examples already exist, e.g. `/NODESKLINK` in some NSIS-based installers).

### 3. CLI argument

`--no-icon` is a boolean flag registered on `install` and `upgrade` (including `upgrade --all`).

- `Args::Type::NoIcon` in `ExecutionArgs.h`.
- `ArgumentCommon::ForType`: name `"no-icon"`, alias none, category `ArgTypeCategory::InstallerBehavior | ArgTypeCategory::CopyFlagToSubContext`. `CopyFlagToSubContext` ensures the flag propagates to per-package sub-contexts in multi-package installs and `upgrade --all`.
- `Argument::ForType`: resource id `NoIconArgumentDescription`, `ArgumentType::Flag`.
- Registered in the `GetArguments()` of `InstallCommand.cpp` and `UpgradeCommand.cpp`.
- The argument parser (`Command.cpp`) requires no changes.

**Semantics.** `--no-icon` is a *requirement* for that invocation:

- The selected installer must have a usable `NoIcon` switch (manifest-declared, or the Inno known default). Its value is appended to the installer command line.
- If no applicable installer supports it, the command fails with a clear error (no heuristic fallback): *"No applicable installer supports `--no-icon`; pass the installer-specific argument with `--custom`."*
- It is independent of `--silent` / `--interactive` and is never implied by them. This is the maintainer's explicit predictability requirement.
- There is no inverse CLI flag, consistent with how `--scope` works.

### 4. Settings

Two booleans, defaulting to `false`, are added to the shared `InstallPrefReq` definition so they exist under both `preferences` and `requirements` (`schemas/JSON/settings/settings.schema.0.2.json`):

```json
"noIcon": {
  "description": "Controls whether a desktop shortcut is created when supported by the installer",
  "type": "boolean",
  "default": false
}
```

Runtime:

- `Setting` enum gains `InstallNoIconPreference` and `InstallNoIconRequirement` (`UserSettings.h`) with two `SETTINGMAPPING_SPECIALIZATION` entries (`.installBehavior.preferences.noIcon` / `.installBehavior.requirements.noIcon`, `json_t = bool`, `value_t = bool`).
- Two `WINGET_VALIDATE_PASS_THROUGH` lines are added in `UserSettings.cpp`.

Settings apply to both `install` and `upgrade`. This supersedes the note in `doc/Settings.md` that preferences/requirements are only applied for `install` *for the `noIcon` field specifically*: suppressing desktop shortcuts during `upgrade --all` is the primary scenario in #1045. The Settings.md note will be scoped accordingly.

**Precedence**, consistent with existing behavior:

1. CLI `--no-icon` (overrides the matching requirement for the invocation).
2. `installBehavior.requirements.noIcon = true`.
3. `installBehavior.preferences.noIcon = true`.
4. Default (`false`).

A manifest-declared `NoIcon` value always wins over the Inno known default (existing merge semantics: declared switches fill the map before known defaults).

### 5. Installer selection (`ManifestComparator`)

A new filter, modeled directly on `ScopeComparator`, is added:

- `ManifestComparator.h`: `InapplicabilityFlags::NoIcon` and `Options::RequestedNoIcon` (`std::optional<bool>`).
- `ManifestComparator.cpp`: `NoIconFilter`:
  - Requirement (CLI flag or settings requirement): an installer is inapplicable when its effective `Switches` map has no `NoIcon` entry — i.e. no declared switch and no Inno known default.
  - Preference: does not filter; installers with a `NoIcon` entry sort first.
- `WorkflowBase.cpp` (`GetManifestComparatorOptions`): `options.RequestedNoIcon` is set from `context.Args.Contains(Args::Type::NoIcon)`. Settings are read inside the comparator as `ScopeComparator` does.

### 6. Argument assembly

`GetInstallerArgsTemplate` in `ShellExecuteInstallerHandler.cpp` gains a block after the install-location block:

```cpp
bool noIcon = context.Args.Contains(Execution::Args::Type::NoIcon)
    || UserSettings().Get<Setting::InstallNoIconRequirement>()
    || UserSettings().Get<Setting::InstallNoIconPreference>();

if (noIcon)
{
    auto itr = installer.Switches.find(InstallerSwitchType::NoIcon);
    if (itr != installer.Switches.end())
    {
        argTemplate.AppendArg(itr->second);
    }
}
```

The requirement path guarantees an entry exists (the filter removed installers without one); the preference path silently skips when absent. The assembled value flows through `Data::InstallerArgs` and therefore works on both the ShellExecute and DirectMSI (`MsiInstallFlow`) paths. `--override` continues to replace the whole template as today.

### 7. COM API surface

`PackageManager.idl` `InstallOptions`:

```idl
/// Suppresses the desktop shortcut when the selected installer supports it.
Boolean NoIcon;
```

`InstallOptions.h/.cpp` gain the accessor and backing field; `PackageManager.cpp` maps it next to the `PreferredInstallLocation` handling:

```cpp
if (options.NoIcon())
{
    context->Args.AddArg(Execution::Args::Type::NoIcon);
}
```

This is an additive runtimeclass member (COM minor version). The COM API never prompts; the same inapplicability error is returned when no installer supports it.

### 8. PowerShell

`Install-WinGetPackage` and `Update-WinGetPackage` gain a `-NoIcon` switch, mapped to `InstallOptions.NoIcon`. Interactive and pipeline invocation behave identically; errors surface as standard terminating errors. (Implemented together with the COM change since the cmdlets are thin projections of it.)

### Validation pipeline impact

- The winget-pkgs validation pipeline consumes WinGetUtil; updating `InstallerSwitches.cs`, `ManifestVersion.cs`, and embedding the 1.31.0 schema lets it accept and validate `NoIcon`.
- winget-pkgs accepts schema versions n/n-1 for new submissions, which drives publisher adoption once a WinGet release ships the client.
- No behavior change for non-interactive validation: `NoIcon` is an inert data field unless a user requests it.

### Manifest example

```yaml
# Publisher.App.installer.yaml (schema 1.31)
Installers:
  - Architecture: x64
    InstallerType: inno
    InstallerUrl: https://example.com/app-setup.exe
    InstallerSha256: ...
    # No manifest entry needed: Inno known default applies.
  - Architecture: x64
    InstallerType: nullsoft
    InstallerUrl: https://example.com/app-setup.exe
    InstallerSha256: ...
    Switches:
      NoIcon: /NODESKLINK
ManifestType: installer
ManifestVersion: 1.31.0
```

Settings example:

```json
{
    "$schema": "https://aka.ms/winget-settings.schema.json",
    "installBehavior": {
        "preferences": { "noIcon": true },
        "requirements": { "noIcon": true }
    }
}
```

### Flow behavior matrix

| Command / mode | `preferences.noIcon` | `requirements.noIcon` | `--no-icon` |
| --- | --- | --- | --- |
| `install` | Sorts supporting installers first; appends switch when supported | Filters; fails if none support | Requirement; fails if unsupported |
| `upgrade <pkg>` | Same | Same | Same |
| `upgrade --all` | Applied per package; unsupported package upgrades without switch | Applied per package; unsupported package reports an error and continues | Copied to every package sub-context |
| `import` | Inherited per imported install | Inherited | — |
| COM install/upgrade | — | — | `InstallOptions.NoIcon` |
| PowerShell install/upgrade | — | — | `-NoIcon` |
| `--override` given | Ignored (template replaced) | Filter still applies at selection | Flag present but value not appended |
| Interactive / Silent / SilentWithProgress | No effect on experience selection | Same | Independent flag |

Edge cases:

- **Shortcut predates the install/upgrade.** WinGet does not touch it; the switch only suppresses creation by the running installer.
- **Machine-scope installers writing to the public Desktop.** The installer's own shortcut logic is suppressed by the switch before it chooses user-vs-public location; nothing is deleted afterward.
- **App creates a desktop icon on first run.** Out of scope; no installer switch exists for that behavior.

## UI/UX Design

No new output is produced on success — the shortcut is simply absent. Help text (English source in `src/AppInstallerCLIPackage/Shared/Strings/en-us/winget.resw`; other locales come from the localization pipeline):

```
--no-icon   Suppress creation of a desktop shortcut (supported installers only)
```

Failure when required but unsupported:

```text
No applicable installer supports option: --no-icon
Use --custom to pass an installer-specific argument.
```

## Capabilities

### Accessibility

No interactive UI is introduced. The feature reduces unwanted desktop artifacts, which benefits keyboard and screen-reader users navigating the Desktop. Error output flows through the existing reporter and VT pipeline.

### Security

WinGet passes a manifest-declared or fixed-known argument string to an installer it already executes; no new file system access is added. WinGet never enumerates or deletes Desktop content, so there is no risk of removing user data. Manifests remain subject to publisher validation as today.

### Reliability

Behavior is deterministic: the switch is passed iff the selected installer is known to support it; otherwise preference silently skips and requirement deterministically fails. This avoids the fragile heuristics the maintainer rejected.

### Compatibility

Fully additive: optional manifest field, opt-in CLI flag (not implied by `silent`/`interactive`), additive COM member, default-off settings. No existing behavior changes unless the user opts in. The only documentation scope adjustment is the install-only Settings.md note, narrowed per-field for `noIcon`.

### Performance, Power, and Efficiency

One map lookup and at most one extra argument appended during command-line construction. No new network, process, or polling cost.

## Potential Issues

- **Inno installers using a custom desktop-task name** are rare (the standard templates define `desktopicon`) but would ignore the known default while still being classified as supporting it. Mitigation: publishers declare an explicit `NoIcon` value, which overrides the default. If reviewers prefer stricter semantics, the requirement filter could accept the Inno known default only as a *preference* and require an explicit manifest declaration for *requirements*; this costs extra bookkeeping to distinguish declared vs. defaulted switches.
- **NSIS / bare EXE / MSI** coverage depends entirely on manifest authors declaring switches. Adoption is incremental via the n/n-1 schema policy.
- **`upgrade --all` with a hard requirement** can produce per-package errors for unsupported packages. The existing report aggregates them without aborting the whole batch; users wanting best-effort behavior use `preferences`.
- **First-run app-created icons** cannot be addressed by this design.

## Deprecation Path

Not applicable; no existing field or feature is replaced. `--custom` remains supported and is the escape hatch for installers without a declared switch.

## Future Considerations

- Start Menu shortcut suppression/relocation (#6429) could reuse this switch infrastructure with separate `NoStartMenuShortcut` / shortcut-location fields.
- Standardization of an ARP/registry contract for shortcut suppression (suggested by the NSIS maintainer in #1045) would let WinGet enable the behavior for installers without command-line switches.
- Follow-up PRs in **winget-create** (authoring support), **winget-cli-restsource** (new REST schema revision), and **winget-pkgs** (YamlCreate/validation updates).

## Resources

- Issue #1045: https://github.com/microsoft/winget-cli/issues/1045
- Maintainer direction (2026-07-27): https://github.com/microsoft/winget-cli/issues/1045#issuecomment-5094833020
- Issue #6429 (Start Menu shortcuts): https://github.com/microsoft/winget-cli/issues/6429
- Inno Setup command-line parameters: https://jrsoftware.org/ishelp/topic_setupcmdline.htm
- Manifest schema versioning: `schemas/JSON/manifests/README.md`
- Settings documentation: `doc/Settings.md`
