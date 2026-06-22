---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-18
last updated: 2026-06-18
issue id: 6298
---

# Day 0 Update

For [#6298](https://github.com/microsoft/winget-cli/issues/6298).

## Abstract

Enable WinGet to detect that it is outdated and self-update on first meaningful command execution. When the first command requiring the source index runs, the client checks a version signal in the preIndexed package source, determines whether a newer stable release exists, and — with user consent — updates itself and relaunches the original command. This eliminates the "chicken and egg" problem where WinGet must already be current to use new functionality (e.g., `winget configure`).

## Inspiration

- Users who receive WinGet via Windows or the Microsoft Store often have an outdated client by the time they first use it. New features (Configuration, DSC v3, parallel install) are unreachable until the user independently discovers they need to update.
- `winget configure enable` is a friction point: users attempting `winget configure` for the first time hit "unrecognized command" because their client predates the feature. Day 0 Update removes this barrier by updating the client before the command fails.
- Other package managers solve this with auto-update (Homebrew, rustup, pip self-check). WinGet currently has no equivalent.
- Enterprise provisioning scenarios (OOBE, Configuration-as-Code) benefit from guaranteed-current clients without requiring a separate pre-flight update step.

## Solution Design

### Version Signal in PreIndexed Source

The preIndexed package source already carries metadata about available packages. A new metadata field advertises the latest stable client version:

```json
{
  "latestStableClientVersion": "1.30.1234.0",
  "minimumRecommendedClientVersion": "1.28.0.0",
  "day0UpdatePackageId": "Microsoft.AppInstaller"
}
```

- `latestStableClientVersion` — The newest stable release of the WinGet client.
- `minimumRecommendedClientVersion` — The minimum version that supports all current source features (e.g., new schema fields, API endpoints). Clients below this threshold get a stronger update prompt.
- `day0UpdatePackageId` — The package identifier to use for self-update (allows flexibility between AppInstaller and a future standalone package).

This metadata is updated as part of the source publication pipeline whenever a new stable client ships.

### Detection Flow

```
User runs: winget configure myconfig.yaml
         │
         ▼
┌─────────────────────────────┐
│ Source index access required │
│ (first command this session)│
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│ Read latestStableClientVersion │
│ from source metadata          │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│ Compare against own version │
│ (Package.Current.Id.Version)│
└──────────────┬──────────────┘
               │
        ┌──────┴──────┐
        │ Up to date? │
        └──────┬──────┘
          Yes  │  No
          │    │
          ▼    ▼
       Proceed  ──► Day 0 Update Flow
```

### Update Flow

When an update is available:

1. **Inform the user** in the current terminal:
   ```
   A newer version of WinGet is available (1.30.1234.0 → current: 1.28.567.0).
   Updating is recommended to access all features.
   
   Update now? [Y/n]:
   ```

2. **If accepted:**
   - Record the original command line arguments.
   - Initiate the self-update via `winget upgrade Microsoft.AppInstaller --silent`.
   - The current process exits (WinGet cannot update itself while running).
   - A watchdog process (or scheduled task) monitors the update completion.
   - On completion, launch a **new terminal window** with the original command, using `--wait` to prevent the window from closing on completion.

3. **If declined:**
   - Record the dismissal (increment counter in local state).
   - Proceed with the original command on the current version.
   - If the command fails because the client is too old (e.g., unrecognized subcommand), show a targeted message: "This feature requires WinGet 1.29+. Run `winget upgrade Microsoft.AppInstaller` to update."
   - The client determines available functionality in the newer version by examining the version signal in the source metadata. When the source index or schema version exceeds what the current client understands, the client infers that new functionality is available in the advertised `latestStableClientVersion`. This heuristic may not capture all edge cases, but provides a reasonable signal for the most common scenario: a user attempting a command that requires a newer client.

4. **If non-interactive** (`--disable-interactivity`, COM API, Configuration):
   - Do not prompt. Proceed with current version.
   - Emit a warning to stderr: "WinGet update available (1.30.1234.0). Run `winget upgrade Microsoft.AppInstaller` to update."
   - If `--nowarn` is specified, suppress the warning entirely. The command proceeds silently on the current version. If the current version cannot execute the command (e.g., unrecognized subcommand), the command fails with an appropriate error code — the update does not occur implicitly.

### Self-Update Mechanism

The update targets `Microsoft.AppInstaller` from the winget default source:

```
winget upgrade --id Microsoft.AppInstaller --source winget --silent
```

Because WinGet cannot update its own running process:

1. WinGet records the original command line and context using the resume infrastructure (`winget resume`).
2. WinGet spawns a detached update orchestrator process (a small helper EXE bundled with the client).
3. The orchestrator:
   - Waits for the original WinGet process to exit.
   - Triggers the Microsoft Store update or direct MSIX install.
   - On success: uses `ShellExecute` to invoke the app execution alias (`winget`) with `resume` to continue the original operation. This avoids a hard dependency on Windows Terminal or `cmd.exe` and works regardless of the user's default terminal application.
   - On failure: uses `ShellExecute` to launch `winget` with an error message displayed and the original command for manual retry.

### Relaunch Behavior

The relaunch leverages `winget resume` (currently experimental) to restore the original command context:

```
winget resume --resume-id <saved-id>
```

The resume infrastructure records the full command line, working directory, and any relevant state. After a successful self-update, the orchestrator invokes `winget resume` via `ShellExecute` on the app execution alias. This delegates terminal window creation to the operating system's default handler, avoiding any dependency on a specific terminal application.

If `winget resume` is unavailable (older client somehow reached this path), fall back to `ShellExecute` with the original command line:

```
ShellExecute(NULL, "open", "winget", "configure myconfig.yaml --wait", workingDir, SW_SHOWNORMAL)
```

### `winget configure` Special Case

When the first command is `winget configure` and the current client doesn't recognize it (pre-Configuration builds):

- The Day 0 Update check happens *before* command parsing fails.
- If `latestStableClientVersion` indicates the feature is available in the new version, the update prompt specifically mentions: "The `configure` command requires WinGet 1.29+. Update now to proceed."
- This eliminates the need for `winget configure enable` entirely on fresh installs — the user gets the latest client which has Configuration enabled by default.

### Session Throttling

To avoid pestering users:

- Day 0 Update check runs **once per session** (once per process lifetime).
- After dismissal, suppress for 24 hours (configurable).
- After N dismissals (default: 3), suppress permanently until the setting is reset or a major version ships.
- The check adds negligible latency (metadata is already being fetched as part of source index access).

### Group Policy Controls

| Policy | Type | Default | Description |
|--------|------|---------|-------------|
| `EnableDay0Update` | Bool | Enabled | Master toggle for Day 0 Update |
| `Day0UpdateBehavior` | Enum | Prompt | `Prompt` / `AutoUpdate` / `NotifyOnly` |
| `Day0UpdateSource` | Enum | Store | `Store` / `Direct` (direct MSIX from CDN) |
| `Day0UpdateDismissLimit` | Int | 3 | Dismissals before permanent suppression |
| `ForceMinimumClientVersion` | String | Empty | If set, blocks command execution below this version (enterprise lockdown) |

`AutoUpdate` mode (enterprise): update happens without prompt. Useful for kiosk/OOBE scenarios.

`ForceMinimumClientVersion`: Hard block. If the running client is below this version, all commands fail with: "Your organization requires WinGet [version]+. Please update." This enables enterprises to enforce client currency.

### Settings

```json
{
  "day0Update": {
    "enabled": true,
    "behavior": "prompt",
    "dismissCount": 0,
    "suppressUntil": "",
    "preferredUpdateSource": "store"
  }
}
```

| Setting | CLI Argument | GPO Policy | Interaction |
|---------|-------------|------------|-------------|
| `day0Update.enabled` | `--no-update-check` | `EnableDay0Update` | GPO wins; arg overrides setting |
| `day0Update.behavior` | N/A | `Day0UpdateBehavior` | GPO wins |
| `day0Update.preferredUpdateSource` | N/A | `Day0UpdateSource` | GPO wins |

### CLI Arguments

| Argument | Commands | Description |
|----------|----------|-------------|
| `--no-update-check` | All | Skip the Day 0 Update check for this invocation |
| `--wait` | All | (Existing) Keep window open after completion — used by relaunch |

### COM API Surface

```idl
interface IPackageManagerStatus
{
    Boolean IsClientUpdateAvailable { get; };
    String LatestStableClientVersion { get; };
    String CurrentClientVersion { get; };
    String MinimumRecommendedClientVersion { get; };
}
```

The COM API **never** self-updates or prompts. It exposes status for the caller to handle.

### PowerShell Cmdlets

```powershell
# Check if update is available
Test-WinGetClientUpdate

# Returns:
# UpdateAvailable       : True
# CurrentVersion        : 1.28.567.0
# LatestStableVersion   : 1.30.1234.0
# RecommendedMinimum    : 1.28.0.0

# Force update
Update-WinGetClient [-Force]
```

`Update-WinGetClient` handles the self-update flow programmatically (useful in scripts and Configuration resources).

### Cross-Repository Impact

- **winget-cli** — Day 0 Update engine, orchestrator helper, GPO policies, settings, COM API, PowerShell cmdlets.
- **winget-cli-restsource** — REST source metadata schema extension for `latestStableClientVersion`.
- **winget-pkgs** — PreIndexed source publication pipeline updated to include client version metadata.
- **winget-create** — No impact (manifest authoring tool, not a runtime).
- **winget-dsc** — A `WinGetClientVersion` DSC resource could assert minimum client version in Configuration files.

### Schema Version

No manifest schema changes required. The version signal lives in source metadata, not package manifests.

## UI/UX Design

### Interactive prompt (default):

```
> winget configure myconfig.yaml

┌─────────────────────────────────────────────────────────────────┐
│  WinGet update available: 1.28.567.0 → 1.30.1234.0             │
│                                                                 │
│  Updating is recommended to access all current features.        │
│  WinGet will restart and run your command after updating.       │
│                                                                 │
│  [U]pdate now    [S]kip this time    [D]on't ask again          │
└─────────────────────────────────────────────────────────────────┘
```

### After update completes (new window):

```
> winget configure myconfig.yaml
WinGet updated successfully (1.30.1234.0).

Applying configuration from: myconfig.yaml
[...]
```

### Non-interactive warning:

```
> winget install Git.Git --disable-interactivity
WARNING: WinGet update available (1.30.1234.0). Run 'winget upgrade Microsoft.AppInstaller' to update.

Found Git [Git.Git] Version 2.45.1
[...]
```

### Command failure due to outdated client:

```
> winget configure myconfig.yaml
Unrecognized command: 'configure'

This command requires WinGet 1.29.0+. Your version: 1.27.234.0
Run 'winget upgrade Microsoft.AppInstaller' to update, or use --no-update-check to skip.
```

### `--no-vt` mode:

Same text content but without the box-drawing characters. Plain text prompt:

```
WinGet update available: 1.28.567.0 -> 1.30.1234.0
Update now? (Y/n):
```

## Capabilities

### Accessibility

- All prompts use text labels accessible to screen readers — no reliance on color or symbols alone.
- Keyboard navigation: single-key responses (U/S/D) with clear labels.
- Non-interactive mode ensures scripts and AT consumers are never blocked by prompts.
- All strings are in the localization resource file (`winget.resw`).

### Security

- The version signal is read from the same signed source index that WinGet already trusts — no new trust boundary.
- Self-update uses the existing Microsoft Store or signed MSIX pipeline — no sideloading.
- `ForceMinimumClientVersion` GPO prevents enterprise devices from running outdated (potentially vulnerable) clients.
- The orchestrator helper is signed and co-ships with the client package.

### Reliability

- If the source is unreachable (offline scenario), the Day 0 check is skipped silently — never blocks operations.
- If the update fails, the user is informed and the original command runs on the current version.
- Relaunch failure (e.g., terminal not found) falls back to displaying the command for manual re-execution.
- Session throttling prevents update check from becoming a recurring annoyance.

### Compatibility

- No breaking changes. Day 0 Update is opt-out, not opt-in.
- `--no-update-check` provides escape hatch for scripts that must not have update prompts.
- COM API callers are never affected (no prompts, no auto-update — just status exposure).
- Older clients ignore the new source metadata field (unknown fields are already silently skipped).

### Performance, Power, and Efficiency

- Negligible additional latency: version comparison happens against metadata already fetched during source access.
- No additional network requests — the version signal is bundled with the source index.
- The orchestrator helper is a lightweight process (no runtime dependencies).

## Potential Issues

- **Process handoff complexity**: The "exit → update → relaunch" flow has edge cases (terminal closed by user mid-update, update stuck, insufficient permissions).
- **Store update latency**: Microsoft Store updates can be slow or queued. Direct MSIX may be more reliable for Day 0 scenarios.
- **Admin vs. user context**: Self-update may require elevation depending on the install method. The orchestrator must handle both contexts gracefully.
- **Circular dependency**: If the source index itself requires a newer client to parse, the version check can't happen. Mitigation: the `latestStableClientVersion` field uses a fixed, simple schema that all client versions can read.
- **Configuration-as-Code**: Automated `winget configure` flows in CI/CD should never be interrupted. Non-interactive mode and `--no-update-check` address this, but documentation must be clear.

## Future Considerations

- **Staged rollout**: Day 0 Update could integrate with the existing feature toggle system for gradual rollout.
- **Delta updates**: Rather than full package replacement, future iterations could support incremental/delta client updates for faster Day 0 flow.
- **Pre-download**: Background download of the update package so the update is instant when the user accepts.
- **Telemetry**: Track how often users are outdated at first launch, update acceptance rates, and time-to-current-version.
- **Extension to plugins**: If WinGet gains a plugin model, Day 0 Update could also check plugin compatibility with the new client version.
- **`winget configure` implicit enable**: Once Day 0 Update ensures current clients, the `winget configure enable` step becomes unnecessary and can be deprecated entirely.

## Resources

- [#6298: Day 0 Update: Self-update on first command when client is outdated](https://github.com/microsoft/winget-cli/issues/6298)
- [#5841: Augmented cache control for PreIndexed sources](https://github.com/microsoft/winget-cli/issues/5841)
- [#4868: Cannot enable WinGet Configuration](https://github.com/microsoft/winget-cli/issues/4868)
- [Homebrew auto-update](https://docs.brew.sh/Manpage#environment) — Prior art for package manager self-update
- [rustup self update](https://rust-lang.github.io/rustup/) — Prior art for toolchain self-update
