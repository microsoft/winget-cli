---
author: JohnMcPMS, GitHub Copilot <Copilot>
created on: 2026-10-29
last updated: 2026-11-03
---

# Protected Settings Overlay

## Abstract

Add a per-user, administrator-writable settings file using the existing user
settings schema, whose explicitly configured values override the ordinary user
settings file where eligible for the execution context. Selected settings ignore
ordinary-file values with the non-default full token; module and default install
roots additionally use separate values across the token boundary, while keeping
the existing settings interface and authoritative in-process settings.

## Inspiration

The ordinary `settings.json` file is writable by a medium-integrity process but
can be read by an elevated process. Its values can influence privileged
filesystem destinations, recursive portable-package deletion, and configuration
module discovery. JSON type and path validation do not establish that an
administrator selected those values.

Ignoring sensitive values in that file prevents administrators from configuring
those settings for elevated use unless an alternative is provided. A protected
file supplies that alternative without introducing a second settings schema.
Protected overrides can be shared across contexts for output locations and
other suitable controls. Locations used for reading or importing content may
instead need independent ordinary and full-token values so ordinary operations
do not require the administrator-selected location to be medium-writable.

## Solution Design

### Terminology and scope

| Term | Meaning |
|---|---|
| Ordinary file | Existing medium-writable user settings file and its backup. |
| Protected file | New `admin-settings.json`, writable only with administrative privileges and readable by the current user without elevation. |
| Full token | The condition identified by the existing `Runtime::IsRunningWithNonDefaultFullToken()` helper. |
| Restricted setting | A setting whose ordinary-file value is ignored with a full token. |
| Shared override | A protected-file value eligible in either token context. |
| Split setting | A restricted setting whose protected-file value is eligible only with the full token. |
| In-process settings | Custom JSON supplied through the existing one-time settings initialization API instead of settings files. |

The full-token helper checks `TokenElevationTypeFull`: the non-default full side
of a split token. Reuse it without adding checks for administrator membership,
integrity level, SYSTEM, or impersonation. This restriction is not defined as a
new universal check for every privileged process.

The protected file is separate from the existing admin capability settings,
such as `InstallerHashOverride`, and does not change those controls.

### Protected storage

Store `admin-settings.json` directly in the existing protected settings
directory, using `PathName::SecureSettingsForRead` for reading and
`PathName::SecureSettingsForWrite` for creation and writes.

The existing paths are:

```text
Packaged:
%PROGRAMDATA%\Microsoft\WinGet\<UserSID>\settings\pkg\<PackageName>\admin-settings.json

Unpackaged:
%PROGRAMDATA%\Microsoft\WinGet\<UserSID>\settings\win\<RuntimeStateName>\admin-settings.json
```

These are per-user locations, not machine-wide settings. Reuse the same path
enum values in both contexts; their existing implementations separate package
identity and unpackaged runtime state. Discover the actual path through
`winget settings export` rather than reconstructing it in management tools.

Reuse the existing administrator ownership, SYSTEM access, and current-user
read permissions. Reads must not create the directory or change permissions.
Primary, backup, temporary files, and their containing directories must retain
protection against medium-token replacement, deletion, or redirection.

The existing `Type::Secure` container stores content separately from protected
verification metadata. This feature reuses the protected directory and its ACL
mechanisms, not that content-plus-hash arrangement. Existing admin capability
settings and source configuration hashes are unchanged.

Use `admin-settings.json.backup` for protected recovery. Mirror the existing
backup-on-open behavior: an elevated CLI open of a successfully loaded primary
can update the backup before editing. Do not back up every write, create a
backup during an unelevated open, or replace a good backup with a primary known
to be invalid.

### Setting classifications

Add explicit treatment metadata to every canonical `SettingMapping`, alongside
its JSON path, type, default, and policy mapping. Three treatments are needed:
unrestricted/shared override, restricted/shared override, and restricted/split.
Do not introduce a general flags framework for hypothetical future treatments.

Require an explicit classification for new settings, including settings
compiled only with test hooks.

The initial restricted set is:

| Setting | Protected override | Rationale |
|---|---|---|
| `configureBehavior.defaultModuleRoot` | Split | Modules are saved, discovered, and imported here; ordinary operations must not force the full-token root to be medium-writable. |
| `installBehavior.portablePackageUserRoot` | Shared | Keep consistent paths; separate portable work will block full-token user-scope operations, then implement de-elevation. |
| `installBehavior.portablePackageMachineRoot` | Shared | Keep consistent paths; machine-scope portable mutations require elevation. |
| `installBehavior.defaultInstallRoot` | Split | Package installers may read existing contents or installed components; not a guaranteed write-only destination. |
| `downloadBehavior.defaultDownloadDirectory` | Shared | Download-only output, not ordinary installation staging or automatic execution input. |
| `uninstallBehavior.purgePortablePackage` | Shared | Boolean deletion policy, not a configurable content search location. |

Portable roots are not write-only: WinGet later reads hashes and portable SQLite
indexes at installed locations. Sharing the user root is an explicit design
choice based on separate blocking/de-elevation work, not a claim that those
protections are implemented by this feature. Document machine-scope elevation
requirements in the schema/spec; do not add another runtime metadata dimension.

Keep all other settings unrestricted initially. Installer preferences select
among otherwise applicable artifacts; extraction settings select fixed
implementations; network settings do not supply endpoints or bypass
verification; log retention settings operate within the selected log location.
These controls affect behavior, but are not automatically equivalent to
supplying a privileged destination or module search path.

### Settings schema and examples

Both files use the existing settings schema. No new settings keys or manifest
schema version are required.

Update the descriptions of all six restricted properties in
`schemas\JSON\settings\settings.schema.0.2.json` to explain that ordinary-file
values are ignored with a non-default full token and direct users to
`winget settings --admin` to configure the protected file. This makes the
restriction discoverable in editors using the shared schema without changing
property types or validation. For the two split properties, also explain that
protected-file values do not apply without the full token. Document the
context-specific module default and the machine portable root's elevated-only
operational use.

For example, the ordinary file can contain:

```json
{
  "$schema": "https://aka.ms/winget-settings.schema.json",
  "visual": {
    "progressBar": "rainbow"
  },
  "downloadBehavior": {
    "defaultDownloadDirectory": "C:\\UserDownloads"
  },
  "configureBehavior": {
    "defaultModuleRoot": "C:\\UserModules"
  }
}
```

The protected file can contain:

```json
{
  "$schema": "https://aka.ms/winget-settings.schema.json",
  "downloadBehavior": {
    "defaultDownloadDirectory": "C:\\ManagedDownloads"
  },
  "configureBehavior": {
    "defaultModuleRoot": "C:\\ManagedModules"
  }
}
```

The resulting view uses `C:\ManagedDownloads` in both token contexts and retains
the ordinary progress-bar preference. If the protected download setting is
absent, an ordinary-token process uses `C:\UserDownloads`; a full-token process
uses the compiled default and records a migration warning.

The module root instead uses `C:\UserModules` without the full token and
`C:\ManagedModules` with it. The administrator must protect the latter directory
against ordinary writes; storing its name in the protected file does not
establish that directory's permissions.

An administrator may override unrestricted settings as well. A protected file
containing only selected values does not default the remaining user settings.

An eligible protected field can explicitly select the built-in default with
`null`, even when the ordinary file supplies a value:

```json
{
  "$schema": "https://aka.ms/winget-settings.schema.json",
  "downloadBehavior": {
    "defaultDownloadDirectory": null
  }
}
```

This intentional selection emits neither a validation nor migration warning and
blocks ordinary-file fallback. Eligibility still applies: a protected module or
default install root, including null, has no effect without the full token.
Existing Group Policy handling still takes precedence.

### Loading and effective settings

Implement the change within `UserSettings`. Preserve `User().Get<S>()` and the
operational consumers that already use it.

Preserve the existing settings policy gate. If `EnableSettings` disables
settings, retain the early exit and do not load either file.

When in-process content is supplied, use that content instead of loading either
file. Do not filter its values by file classification, overlay protected-file
values, recover from disk on custom parse failure, or emit file migration
warnings. Preserve existing typed validation, policy handling, and one-time
initialization behavior. Its authority is over file loading, not Group Policy.

For file-backed initialization:

1. Select the ordinary primary or backup using existing behavior.
2. Load the protected primary or recover its protected backup.
3. Compose explicit entries at canonical setting granularity, filtering
   restricted ordinary entries with the full token and protected entries for
   split settings without it.
4. Apply existing typed validation and expose the effective settings view.

Preserve the existing policy-first validation in `Validate<S>()`. It already
selects a mapped Group Policy value before reading JSON; an invalid policy value
uses the default rather than JSON. Do not add a separate policy precedence
layer or change unrelated existing no-valid-JSON behavior.

Absent a mapped policy, file-backed selection is:

| Context | Classification | Protected field absent | Protected field valid | Protected field null | Protected field invalid |
|---|---|---|---|---|---|
| Ordinary token | Unrestricted/shared | Ordinary value or default | Protected value | Default; no warning | Warn; default |
| Ordinary token | Restricted/shared | Ordinary value or default | Protected value | Default; no warning | Warn; default |
| Ordinary token | Restricted/split | Ordinary value or default | Ordinary value or default | Ordinary value or default | Ordinary value or default |
| Full token | Unrestricted/shared | Ordinary value or default | Protected value | Default; no warning | Warn; default |
| Full token | Restricted/shared | Default | Protected value | Default; no warning | Warn; default |
| Full token | Restricted/split | Full-token default | Protected value | Full-token default; no warning | Warn; full-token default |

An eligible invalid protected field is not the same as an absent field: it blocks
ordinary-file fallback and selects the default. Protected entries for split
settings are ineligible without the full token, so even an invalid entry cannot
replace the ordinary value or force its default. Other valid eligible protected
settings still apply. Any whole-file load failure with no usable protected
backup leaves both files' settings unapplied and selects compiled defaults
with warnings in either context. Settings load failures must not prevent
functionality.

"Default" normally means the existing compiled default. The module root has a
context-specific default described below. An absent default install root still
uses the existing empty default and location-prompt behavior.

Null is an explicit default selection, not an absent or invalid field.
False, zero, and valid empty collections are explicit values. Arrays replace
the lower-precedence array as a whole; they are not concatenated. Overriding one
nested setting must preserve unrelated siblings.

Keep explicit presence and source information long enough to make these
decisions and produce diagnostics. Do not overlay two fully default-expanded
objects or implement a second schema/parser. Preserve the existing user source
and edit semantics exposed by `GetType()`; track overlay state separately as
needed.

### Full-token module storage and discovery

Without a valid protected module-root value, use a distinct administrator-writable
per-user module directory with the full token. Reuse protected per-user path/ACL
mechanisms and preserve packaged/unpackaged separation. Ordinary-token processes
retain their existing module default and cannot change the full-token default's
contents. Invalid eligible root values select this context-appropriate default,
not an empty value that would fall back to LocalAppData.

Use `PathName::DefaultConfigurationModules` through the existing
`GetPathDetailsFor`/`GetPathTo` initialization rather than custom path getters.
`PathName::ConfigurationModules` continues to select the effective configured
root, delegating an empty value to that built-in default. Initialize the
protected parent before creating module descendants; non-creating default
lookups return a literal path without display anonymization.

The internal native export
`WindowsPackageManagerConfigurationGetModulePath(BOOL useSettings, BOOL* fullToken, BSTR* path)`
in WindowsPackageManager.dll supplies the managed processor with its local
token decision and module path. `useSettings` selects effective settings or the
built-in default; it does not select DSC executables or other remoting inputs.
The HRESULT-returning export allocates the returned BSTR for the caller to free.
It reuses the existing settings view and runtime path initialization, not a new
parser or shared-library path implementation.

For file-backed CLI settings, leave the factory location unset when no module
path argument is supplied. The receiving processor, including an elevated
receiver, resolves its own effective `ConfigureDefaultModuleRoot`. PowerShell
configuration cmdlets likewise preserve an omitted module path. An explicitly
assigned default instead chooses the built-in default, ignoring the configured
root. Preserve explicit current-user/all-users/custom roots and authoritative
in-process content, including a custom-content selection of the default.

The dynamic factory omits `modulePath` for an implicit default and serializes
an explicit default as `"modulePath": "Default"`. Scope/custom selections retain
their existing serialization. DSC executable discovery, expected hash/alias
binding, and other remoting inputs remain unchanged.

Full-token module discovery does not automatically prepend the ordinary WinGet
module directory as an additional fallback when selecting a protected custom
or default root. Remove any preexisting equivalent ordinary-root entry before
applying explicit paths. Match whole normalized entries, not substrings or
child/sibling paths. Such an entry is not a PowerShell built-in default; it may
already be present in the process environment.

Preserve explicit additional/implicit paths and custom roots by applying them
after cleanup. AdditionalModulePaths is caller-controlled through the factory
interface. The dynamic elevation factory does not forward that collection.
Each remoting receiver derives its ImplicitModulePaths from its own packaged
ExternalModules location before applying its limitation set.

All hosted/default processor environments use `InitialSessionState.CreateDefault2`
without eagerly importing DSC. Configure discovery and installation paths
before `ValidateRunspace` imports or installs required modules, regardless of
token context. Preserve the selected execution policy.

Do not automatically copy or trust modules from the ordinary directory.
Reacquire or explicitly provision required modules in the protected location.
Preserve authoritative in-process roots and existing explicit API inputs.
This change isolates WinGet's ordinary module directory; it does not claim that
every PowerShell search path or administrator-selected custom path is trusted.

### Recovery and defaults

Ordinary and protected settings files share the same parsing and JSON-object
root validation. Stream failures and read exceptions record source-specific
warnings for either file type and trigger the applicable backup/default path.

| Protected-file condition | Result |
|---|---|
| Neither primary nor backup exists | Normal absence; continue with eligible ordinary values/defaults. |
| Primary loads successfully | Use primary; do not recover individual invalid fields from backup. |
| Primary is absent, unreadable, or malformed; backup loads | Use protected backup and record a recovery warning. |
| Protected configuration cannot be read/parsed, has a non-object root, or otherwise cannot be loaded, and no usable backup exists | Warn and use compiled defaults without applying either file's settings; do not propagate load errors. |
| Individual eligible protected field is invalid | Warn and use its context-appropriate default; retain other valid fields. |

Report source and setting path in diagnostics without unnecessarily exposing
setting values. Report access failures as warnings rather than silently treating
them as missing files.
For restricted full-token settings, no failure or recovery path reactivates an
ordinary-file value.

The load-failure fallback matches the existing no-valid-JSON behavior: no
settings are added to the effective map. Keep ordinary-file source/edit
semantics in `GetType()` even when its values are not applied.

Keep protected-file opening and management path discovery available for repair
when settings could not be loaded. Normal operations continue with compiled
defaults; discovering or opening a file for repair must not depend on its
contents being valid.

### CLI management

Add `winget settings --admin` to target the protected file. Plain
`winget settings` continues to target the ordinary file.

An elevated protected open may create the directory/file and prepare its
backup, using the existing schema-linked settings template. An unelevated open
of an existing file launches the editor without creation or backup. Write
protection prevents saving changes without elevation; do not automatically
request UAC elevation or change ACLs.

If an unelevated open finds no file, report its path and explain that elevated
creation is required. Do not launch an editor with an unsaved template.

The hidden provisioning form `winget settings --admin --create` requires
elevation and creates the schema-linked primary only if absent. It does not open
an editor, replace an existing file, or prepare a backup. Require `--admin` when
`--create` is present. This supports headless callers such as PowerShell
protected-file management without making editor launch a provisioning step.

Reject combinations of `--admin` with existing capability mutation arguments
such as `--enable` and `--disable`. Existing `settings set` and `settings reset`
continue to manage their existing admin capability settings; they do not
silently become JSON file editors.

Extend `winget settings export` with an `adminSettingsFile` string alongside
`userSettingsFile`. Preserve the existing `adminSettings` object. Add the
property to the settings export schema without requiring it from older clients:

```json
{
  "properties": {
    "adminSettingsFile": {
      "type": "string"
    }
  }
}
```

This is an additive change to the export schema, not a settings or manifest
schema change. The export remains a path/capability discovery operation, not
an export of raw file contents or resolved effective values. Its output must
remain valid JSON, including when a settings file requires repair.

### Migration warnings

When a full-token file-backed load ignores a restricted ordinary-file value
without a valid protected replacement, add a setting-specific migration warning.
Identify the key, explain default selection, and point to protected-file
management. Do not print the ignored value.

A valid protected replacement or eligible explicit null suppresses this
migration warning. An invalid protected field does not suppress it merely by
being present. Missing ordinary fields, ordinary-token loads, and authoritative
in-process initialization do not generate migration warnings.

Display migration warnings once on every affected elevated CLI invocation,
using a shared reporting point rather than changes to each consumer. Avoid
duplicates when opening settings. Keep the existing visibility of unrelated
settings warnings. Diagnostics must not contaminate structured JSON output.

### COM API and package operations

No IDL or public COM interface changes are required.
`PackageManagerSettings.SetUserSettings` retains its existing in-process,
one-time custom initialization semantics and bypasses both files.

File-backed COM consumers use the same effective settings view. COM remains
non-interactive; settings load failures select compiled defaults and record
diagnostics rather than prompting, displaying CLI output, or propagating file
load errors.

CLI install, upgrade, uninstall, download, and other settings consumers receive
the composed values without changes to their operational logic. Configuration
module default/discovery handling has the narrow exception described above.
Explicit CLI/API argument precedence, installer launching, and resume
serialization remain unchanged. The narrow `modulePath` distinction above is
the only remoting metadata change; other inputs and elevation flow are unchanged.

This guarantee applies to values exposed by the view in its initialization
context. It does not redefine the authority of values already selected in a
medium process and subsequently forwarded to an elevated installer or worker,
apart from the specifically addressed implicit module-root selection.

### PowerShell cmdlets

Add an optional `-Admin` switch to `Get-WinGetUserSetting`,
`Test-WinGetUserSetting`, and `Set-WinGetUserSetting`. It selects the protected
path discovered through `adminSettingsFile`; the default remains the ordinary
file.

Preserve raw-file behavior: Get returns the selected file as a hashtable, Test
returns a Boolean, and Set returns the written settings object. Do not return
the effective overlaid view. Preserve existing `-IgnoreNotSet` and `-Merge`
semantics; management merging is not the runtime overlay operation.
`-Merge` ignores input nulls; omit it to persist an explicit null. Without
`-Merge`, Set replaces the file's settings, so callers must include other values
they intend to retain.

Get and Test may run unelevated. Set requires elevation when targeting the
protected file, including directory/file creation. Do not create backups on Set
or invoke automatic elevation. Pipeline behavior and existing confirmation
behavior remain unchanged; `-Force`, where supported by a calling workflow,
cannot bypass protected-write authorization.

Package cmdlets such as Install/Update/Uninstall/Repair/Find/Show retain their
interfaces and receive applicable effective values through existing consumers.

### WinGet Configuration and DSC

Add an optional Boolean target selector, defaulting to false, to both
`WinGetUserSettings` in the PowerShell DSC module and the native
`UserSettingsFile` resource. Use `Admin` in PowerShell and `admin` in native
JSON input/output. Preserve existing default targeting and Full/Partial action
semantics.

Get/Test/Export describe the selected raw file, not effective settings. Set of
the protected target requires elevation; Get/Test can run unelevated. Return
the selected target in resource state so subsequent operations use the same
file. Do not add an interactive prompt or automatic elevation.
PowerShell DSC partial updates inherit `-Merge` null handling; use `Full` to
persist a null there. Native partial updates retain explicit null values.

For example, the native resource input can select the protected file:

```json
{
  "admin": true,
  "action": "Partial",
  "settings": {
    "downloadBehavior": {
      "defaultDownloadDirectory": "C:\\ManagedDownloads"
    }
  }
}
```

The PowerShell DSC resource can express the same intent:

```powershell
WinGetUserSettings ProtectedDownloads
{
    SID = ''
    Admin = $true
    Action = 'Partial'
    Settings = @{
        downloadBehavior = @{
            defaultDownloadDirectory = 'C:\ManagedDownloads'
        }
    }
}
```

No package manifest fields are added, so a package manifest YAML example is
not applicable. Configuration workers consume the existing settings view;
only implicit module-root resolution and explicit-default metadata receive the
narrow receiver-local changes described above.

### Flow behavior matrix

| Flow or mode | Behavior |
|---|---|
| Normal interactive operation | Resolve the view; display applicable migration warnings; retain existing workflow prompts. |
| `--no-vt` | Same resolution and plain-text warnings; no dependence on color, cursor movement, or terminal images. |
| `--disable-interactivity` or silent package operation | Same resolution, warnings, and failure rules; no new prompts or automatic consent. |
| `settings` | Open ordinary file; preserve existing backup behavior. |
| Elevated `settings --admin` | Open/create protected file and prepare eligible backup. |
| Elevated `settings --admin --create` | Create only if absent; no editor, replacement, or backup. |
| Unelevated `settings --admin --create` | Fail authorization without creating the file. |
| Unelevated `settings --admin`, file exists | Open without creation, backup, automatic elevation, or permission changes. |
| Unelevated `settings --admin`, file absent | Report path and need for elevated creation; do not launch editor. |
| `settings --admin`, file malformed | Remain available for repair; do not replace a good backup with the malformed primary. |
| `settings export`, file malformed | Still report paths/capabilities as JSON for repair tooling. |
| File-backed operation, unrecoverable protected load failure | Warn and use compiled defaults; apply neither file's settings. |
| Custom in-process initialization | Do not load either file; retain existing validation/policy semantics. |
| Eligible protected field explicitly null | Select built-in default, suppress ordinary fallback and migration warning; retain policy precedence. |
| COM | No prompts or CLI output; propagate failures through existing API handling. |
| PowerShell Get/Test with `-Admin` | Read/test protected raw file without elevation. |
| PowerShell Set with `-Admin` | Require elevation; preserve object output and existing merge behavior. |
| DSC protected target | Non-interactive raw-file operations; protected writes require elevation. |
| Settings disabled by policy | Preserve early exit; load neither settings file. |
| Ordinary-token configuration | Use ordinary module root/default; ignore the protected split entry. |
| Full-token configuration | Use protected module root or protected per-user default; do not automatically search/copy ordinary WinGet modules. |
| Configuration with explicit module-path default | Use receiver's built-in default, not its configured setting. |
| Configuration with omitted file-backed module path | Resolve effective settings locally in the receiving processor. |

### Validation and cross-repository impact

Use existing unit and integration test infrastructure to cover:

- All three classifications and token contexts, with each source absent/present.
- Explicit false/zero/empty values, nested sibling preservation, and whole-array
  replacement.
- Invalid eligible protected fields selecting context defaults rather than
  ordinary fallback; ineligible split entries preserving ordinary selection.
- Eligible explicit protected null selecting defaults without validation or
  migration warnings; ineligible split null leaving ordinary selection intact.
- Primary/backup recovery, defaults without either file's settings after any
  unrecoverable protected load failure (including read/access errors), and
  management repair access.
- Authoritative custom initialization and unchanged Group Policy behavior.
- Migration warning presence, suppression, visibility, and duplicate avoidance.
- Protected directory/file permissions, read-only opens, and elevated creation,
  backup, and mutation.
- Additive export schema/property and PowerShell/DSC raw-file target selection.
- Explicit classification coverage for every canonical setting.
- Distinct protected/ordinary module defaults and protected default permissions.
- Exclusion of automatic/inherited ordinary WinGet module paths with the full
  token, preservation of explicit authoritative roots, and no automatic module
  copying/reuse.
- Receiver-local implicit settings roots, explicit default/scope/custom roots,
  custom-content authority, and deferred DSC import for both processor types.

No package manifest fields, publisher validation rules, or source protocol
changes are involved. Consequently, no changes are required in winget-create,
winget-cli-restsource, or winget-pkgs validation. Non-interactive package
validation remains unchanged. Client-side settings and management tests belong
in winget-cli; this feature does not depend on downstream publisher adoption.

## UI/UX Design

Add the `--admin` flag to settings help and describe the protected file as
administrator-writable, not as the existing admin capability settings.

The following reflects the English resource message formatting; substitute the
actual key, path, and parser diagnostic and localize through the normal resource
pipeline. Warning streams control color without adding a literal `Warning:` prefix.

An affected full-token invocation displays:

```text
The setting '.downloadBehavior.defaultDownloadDirectory' from 'settings.json' is ignored when running with the elevated token. The default is used unless Group Policy supplies a value. Configure an administrator-approved value in admin-settings.json using 'winget settings --admin'.
```

An unelevated protected open with no existing file displays:

```text
PS> winget settings --admin
The administrator settings file does not exist: C:\ProgramData\Microsoft\WinGet\<UserSID>\settings\pkg\<PackageName>\admin-settings.json. Run 'winget settings --admin' as administrator to create it.
```

Protected backup recovery displays through the existing settings-warning path:

```text
The following failures were found validating the settings:
Loaded settings from backup file 'admin-settings.json.backup'.
```

Unrecoverable JSON parsing uses the existing parse/default warning path and
continues without applying either file's settings:

```text
The following failures were found validating the settings:
Error parsing settings file 'admin-settings.json':
<parser diagnostic>
Settings file 'admin-settings.json' couldn't load. Using default values.
```

Unrecoverable read/access failures also warn and continue with compiled defaults:

```text
The following failures were found validating the settings:
Error reading settings file 'admin-settings.json'. <read diagnostic>
Settings file 'admin-settings.json' couldn't load. Using default values.
```

Recovery can also include primary-file read/parse warnings before its backup
notice. These non-migration warning examples describe settings opening.
Warning details identify the failing source. Normal operations and
repair/path-discovery operations remain available after fallback to defaults.

`winget settings export` adds the path without changing existing fields:

```json
{
  "adminSettings": {},
  "userSettingsFile": "C:\\Users\\Example\\AppData\\Local\\Packages\\Example\\LocalState\\settings.json",
  "adminSettingsFile": "C:\\ProgramData\\Microsoft\\WinGet\\<UserSID>\\settings\\pkg\\<PackageName>\\admin-settings.json"
}
```

This example abbreviates the existing capability object; actual export retains
its existing entries. Warnings use diagnostic output, not the JSON output stream.

## Capabilities

### Accessibility

Warnings and instructions are plain text and include the affected setting key.
They must not rely on color, VT support, images, or interactive selection.
Existing editor and assistive-technology behavior remains unchanged.

### Security

Restricted values from ordinary files, including their backups, are ineligible
when the existing full-token check applies. Protected values require an
administrator write, and all associated storage retains the existing protected
directory permissions.

> [!IMPORTANT]
> A protected setting authenticates who configured its value; it does not make
> a referenced directory, module, or other payload trustworthy. This feature
> does not claim to resolve every filesystem or installer-specific attack.

Preserve existing hash, certificate, archive scanning, policy, and capability
gates. Do not promote ordinary settings into the protected file automatically.
Authoritative in-process input and explicit options retain their existing trust
contracts; they are not reclassified as ordinary-file input.

### Reliability

Protected backups provide recovery without silently discarding administrator
overrides. Individual invalid values use defaults and warnings. Any
unrecoverable protected load failure warns and uses compiled defaults without
applying either file's settings. Settings load failures must never prevent
functionality. Built-in defaults must be secure; an unsafe default is a separate
issue, not a reason to require successfully loaded settings.
Management access remains available to diagnose and repair the configuration.

### Compatibility

No setting keys, manifest fields, COM interfaces, or existing default management
targets are removed. Clients without this feature ignore the new file and do
not enforce its restrictions; deployments must not rely on it for older clients.

This intentionally changes full-token behavior for six ordinary-file settings.
Migration warnings and explicit protected configuration provide the transition;
there is no automatic trust promotion. Unrestricted settings retain ordinary
behavior unless explicitly overridden in the protected file.

Module and default install roots are independently configurable for each token
context. Full-token configuration no longer automatically reuses the ordinary
WinGet module directory; administrators must acquire or explicitly provision
modules in the protected location.

The additive export property and optional PowerShell/DSC selectors preserve
existing callers. Raw-file management continues to differ from the effective
settings view.

### Performance, Power, and Efficiency

Load the additional small local file once during existing process-static settings
initialization. Read its backup only for recovery, and write backups only during
eligible CLI opens. No polling is introduced. Module factory initialization also
selects the context-specific default and filters the ordinary WinGet module path.
Initial full-token configuration may need to download modules separately rather
than reuse ordinary modules.

## Potential Issues

- Users may initially see different elevated paths or portable deletion behavior
  until an administrator configures the protected file.
- A malformed or unreadable protected file with no usable backup suppresses both
  files' settings in favor of compiled defaults. Precise warnings and repair
  access must remain available, without preventing normal operations.
- Shared startup code may currently read settings before identifying a repair
  command. Implementation must preserve management access and avoid
  contaminating structured output with warning text.
- Editor association can launch an existing process. Elevation does not guarantee
  that every associated editor can save a protected file; do not relax ACLs or
  automatically elevate an unrelated editor.
- A medium process can resolve values before elevation and forward them through
  an existing protocol. Receiver-local implicit module roots address that
  specific case, not every forwarded value or a broader provenance guarantee.
- Treating invalid protected values as absent would incorrectly restore ordinary
  values. Preserve that distinction through composition and validation.
- Backing up a malformed primary could destroy recovery. Backup-on-open must use
  known source/load state rather than copying unconditionally.
- Protected path and raw file discovery must remain correct across package
  identities, runtime-state names, and account contexts.
- Shared portable roots rely on separate operational elevation protections.
  The planned user-scope blocking/de-elevation work must not be mistaken for an
  implemented guarantee of this overlay.
- Splitting module storage without removing the automatic ordinary WinGet search
  fallback would leave the original cross-context module discovery issue.

## Deprecation Path

No settings, APIs, schema fields, or management operations are deprecated or
removed. The intentional restriction on six ordinary-file values is accompanied
by migration warnings and an administrator-configurable alternative, not a
phased schema deprecation. No fixed removal date or publisher migration is
involved.

## Future considerations

- Additional setting classifications, supported by concrete impact analysis.
- Effective-settings inspection with per-value provenance, separate from raw
  file management.
- Further examination of values resolved before crossing an elevation boundary.

These are out of scope. The initial implementation does not add a machine-wide
overlay, automatic migration, or a new custom-settings authority model.

## Resources

- Existing settings documentation: `doc\Settings.md`.
- Settings schema: `schemas\JSON\settings\settings.schema.0.2.json`.
- Export schema: `schemas\JSON\settings\settings.export.schema.0.1.json`.
- Setting definitions: `src\AppInstallerCommonCore\Public\winget\UserSettings.h`.
- Loading and policy-first validation: `src\AppInstallerCommonCore\UserSettings.cpp`.
- Protected paths: `src\AppInstallerCommonCore\Runtime.cpp`.
- Full-token check: `src\AppInstallerSharedLib\Runtime.cpp`.
- CLI management: `src\AppInstallerCLICore\Workflows\SettingsFlow.cpp`.
- Native module-path export: `src\WindowsPackageManager\main.cpp`.
- Module location forwarding: `src\AppInstallerCLICore\ConfigurationDynamicRuntimeFactory.cpp`.
- PowerShell management: `src\PowerShell\Microsoft.WinGet.Client.Engine\Commands\UserSettingsCommand.cs`.
- Native DSC resource: `src\AppInstallerCLICore\Commands\DscUserSettingsFileResource.cpp`.
- PowerShell DSC resource: `src\PowerShell\Microsoft.WinGet.DSC\Microsoft.WinGet.DSC.psm1`.
- Module default and search paths: `src\Microsoft.Management.Configuration.Processor\Public\PowerShellConfigurationSetProcessorFactory.cs`.
- Portable state and hash reads: `src\AppInstallerCLICore\PortableInstaller.cpp`.
