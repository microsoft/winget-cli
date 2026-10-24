---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-17
last updated: 2026-06-17
issue id: 6189
---

# PUA Allowlisting with User Warnings

For [#6189](https://github.com/microsoft/winget-cli/issues/6189).

## Abstract

Define a mechanism to allow certain classes of Potentially Unwanted Applications (PUA) into the `winget-pkgs` community repository with appropriate warnings and user consent. Legitimate software such as RustDesk (remote control) and Malwarebytes (encrypted installer) are currently excluded despite being widely used and trusted. This spec proposes a tiered classification system, a governed allowlist, client-side warnings, and Group Policy controls.

## Inspiration

Microsoft Defender's PUA detection flags software based on behavioral categories. While protective for most users, these heuristics also block legitimate software:

- **RustDesk** — Open-source remote desktop; flagged as "remote control software" ([#6107](https://github.com/microsoft/winget-cli/issues/6107))
- **Malwarebytes** — Anti-malware tool; flagged for "encrypted/obfuscated" installer ([#6250](https://github.com/microsoft/winget-cli/issues/6250))
- **AnyDesk**, **TeamViewer** portable editions — Flagged under remote access category
- Other legitimate tools — Flagged for system-level access patterns

Their absence from `winget-pkgs` damages credibility and drives users to less-safe acquisition methods (direct downloads from unknown mirrors, Chocolatey community packages without validation).

## Solution Design

### PUA Classification Tiers

| Tier | Description | Repository Policy | Client Behavior |
|------|-------------|-------------------|-----------------|
| **Allowed** | Legitimate software flagged due to category overlap | Accepted with metadata | Warning shown at install |
| **Restricted** | Legitimate uses but elevated risk profile | Moderator review required | Strong warning + consent |
| **Blocked** | Confirmed unwanted/malicious | Rejected | N/A |

### Manifest Schema Extension

Add a `PuaClassification` field to the installer manifest:

```yaml
ManifestType: installer
ManifestVersion: 1.29.0
PackageIdentifier: RustDesk.RustDesk
PackageVersion: 0.3.1
PuaClassification:
  Categories:
    - RemoteControl
  Tier: Allowed
  Justification: "Open-source remote desktop application. Remote control is the primary product function."
  ReviewDate: "2026-03-15"
Installers:
  - Architecture: x64
    InstallerUrl: https://github.com/rustdesk/rustdesk/releases/...
    InstallerSha256: ...
```

**Schema:**

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `Categories` | string[] | Yes | PUA categories detected (enum: see below) |
| `Tier` | enum | Yes | `Allowed` \| `Restricted` |
| `Justification` | string | Yes | Human-readable explanation |
| `ReviewDate` | date | Yes | Date of last moderator review |

**Category enum values:**
- `RemoteControl` — Remote access/desktop tools
- `EncryptedInstaller` — Obfuscated or encrypted binaries
- `SystemMonitor` — Keyloggers, screen capture (legitimate use cases exist)
- `Bundleware` — Packages that bundle other software
- `Cryptocurrency` — Mining software
- `PeerToPeer` — P2P file sharing clients

> [!NOTE]
> These categories are derived from detections across various AV vendors (Defender, ESET, K7, Malwarebytes, etc.). The enum is not exhaustive — additional categories may emerge over time as new detection patterns are identified. The categories abstract vendor-specific detection names into a consistent vocabulary for the WinGet ecosystem.

### Validation Pipeline Changes

1. SmartScreen/Defender scan occurs (existing behavior)
2. On PUA detection, check if package ID + categories exist in the allowlist
3. **Allowlisted**: Allow submission, add `PUA-Allowed` label for visibility
4. **Not allowlisted**: Reject with guidance on how to request allowlisting
5. **New category detected for allowlisted package**: Require re-review

### Allowlist Governance

PUA allowlisting uses the existing **waiver system** — the same mechanism used for other validation pipeline waivers. This is NOT a policy bot configuration or community-maintained file.

**Waiver flow:**

1. A manifest submission triggers a PUA detection during validation pipeline scanning.
2. The validation pipeline applies a label to the PR (e.g., `PUA-Review-Required`) signaling that a Microsoft maintainer must review.
3. A Microsoft maintainer reviews the detection, the package's legitimacy, and the use case.
4. If approved, the maintainer grants a waiver — the package is allowlisted for that category.
5. The waiver is recorded internally and subsequent submissions for the same package + category pass validation without re-review (unless a new category is detected).

**Governance rules:**
- Each waiver requires human validation by a Microsoft maintainer — no automation can grant waivers.
- `Restricted` tier requires Microsoft security team sign-off.
- Annual re-review required (waiver must be within 12 months).
- New category detections for a previously-waivered package require a new review.

### Client-Side Warning Behavior

#### Interactive mode (default):

```
> winget install RustDesk.RustDesk
Found RustDesk [RustDesk.RustDesk] Version 0.3.1
⚠️ This package is classified as: Remote Control Software

This is legitimate software reviewed and approved for the winget community repository.
Remote control software allows remote access to your computer.
Only install if you intend to use remote desktop functionality.

Do you want to proceed? [Y/n]:
```

#### `--disable-interactivity`:

Warning logged to output, installation proceeds. No prompt.

#### `--accept-package-agreements`:

Implies acceptance of PUA warning. Installation proceeds with warning in output.

#### `--ignore-warnings`:

Warning suppressed entirely. Installation proceeds without displaying PUA information.

#### `--silent`:

Does not affect PUA warning behavior. `--silent` only controls installer-level switches (silent install mode). PUA warnings are still shown unless `--ignore-warnings` is specified.

#### COM API:

Warning information returned in result object for caller to handle:

```csharp
var result = manager.InstallPackage(options);
if (result.PuaClassification != null) {
    // Caller decides how to present warning
}
```

### Group Policy Controls

| Policy | Type | Default | Description |
|--------|------|---------|-------------|
| `PuaInstallBehavior` | Enum | Warn | `Allow` \| `Warn` \| `Block` |
| `PuaBlockedCategories` | MultiString | Empty | Categories to block (e.g., `RemoteControl`) |
| `PuaAllowedPackages` | MultiString | Empty | Package IDs that bypass PUA policy |
| `PuaSilenceWarnings` | Bool | False | Suppress warnings (for managed environments) |

**Policy interaction matrix:**

| GPO `PuaInstallBehavior` | `--accept-package-agreements` | Result |
|--------------------------|-------------------------------|--------|
| Allow | Any | Install, no warning |
| Warn | Not set | Prompt (interactive) or warn (non-interactive) |
| Warn | Set | Install with warning in output |
| Block | Any | Blocked with error |

| GPO `PuaBlockedCategories` | Package Category | `PuaAllowedPackages` contains ID | Result |
|---------------------------|------------------|----------------------------------|--------|
| Contains `RemoteControl` | `RemoteControl` | No | Blocked |
| Contains `RemoteControl` | `RemoteControl` | Yes | Allowed (explicit override) |
| Empty | Any | Any | Falls through to `PuaInstallBehavior` |

### Settings

```json
{
  "security": {
    "puaInstallBehavior": "warn",
    "puaBlockedCategories": [],
    "puaSilenceWarnings": false
  }
}
```

| Setting | CLI Argument | GPO Policy | Interaction |
|---------|-------------|------------|-------------|
| `puaInstallBehavior` | `--accept-package-agreements` | `PuaInstallBehavior` | GPO wins |
| `puaBlockedCategories` | N/A | `PuaBlockedCategories` | GPO wins |
| `puaSilenceWarnings` | `--ignore-warnings` | `PuaSilenceWarnings` | Either silences |

### CLI Commands Affected

| Command | Behavior Change |
|---------|----------------|
| `winget install` | Show PUA warning before install |
| `winget upgrade` | Show PUA warning if upgrading a PUA package |
| `winget show` | Display PUA classification in `--details` output |
| `winget search` | PUA flag shown in `--details` output |
| `winget list` | PUA flag shown for installed PUA packages in `--details` output |

### PowerShell Cmdlets

```powershell
# PuaClassification appears in package result objects
$pkg = Find-WinGetPackage -Id "RustDesk.RustDesk"
$pkg.PuaClassification  # Returns: @{Categories=@("RemoteControl"); Tier="Allowed"; Justification="..."}

# Install with -Force skips PUA prompt (like --accept-package-agreements)
Install-WinGetPackage -Id "RustDesk.RustDesk" -Force
```

### Cross-Repository Impact

- **winget-cli** — Client warning logic, GPO policies, settings, manifest parsing for `PuaClassification`
- **winget-pkgs** — Validation pipeline waiver system for PUA allowlisting, `PUA-Review-Required` label, bot comments guiding contributors
- **winget-create** — Support `PuaClassification` field in manifest authoring (read-only display, not user-authored)
- **winget-cli-restsource** — Schema update to serve `PuaClassification` data

### Schema Version

Requires a new manifest schema version for the `PuaClassification` field.

## UI/UX Design

### `winget show` with PUA package:

```
> winget show RustDesk.RustDesk
Found RustDesk [RustDesk.RustDesk]
Version: 0.3.1
Publisher: RustDesk
Description: An open-source remote desktop application.
Homepage: https://rustdesk.com
License: AGPL-3.0
⚠️ PUA Classification: Remote Control Software
   Tier: Allowed (reviewed 2026-03-15)
   Note: Remote control is the primary product function.
```

### `winget search` with `--include-security`:

```
> winget search remote desktop --include-security
Name       Id                Version  Source  Security
─────────────────────────────────────────────────────────────
RustDesk   RustDesk.RustDesk 0.3.1    winget  ⚠️ PUA: Remote Control
```

### Blocked by GPO:

```
> winget install RustDesk.RustDesk
Found RustDesk [RustDesk.RustDesk] Version 0.3.1

❌ Installation blocked by organization policy.
This package is classified as Remote Control Software, which is blocked
by your organization's security policy.

Contact your IT administrator if you need this software.
```

## Capabilities

### Accessibility

- Warnings use full text descriptions readable by screen readers
- No color-only indicators — severity is communicated via text
- JSON output mode provides structured PUA data for programmatic consumption

### Security

- Does not reduce security — adds transparency and enterprise control over software classification
- GPO controls give IT teams MORE granular control than the current binary reject
- Allowlist governance ensures human review of every PUA package
- Annual re-review prevents stale allowlist entries
- `Restricted` tier requires Microsoft security team approval

### Reliability

- No impact on installation reliability — warnings are informational unless GPO blocks
- If allowlist data is unavailable (source not synced), packages not in local cache are treated as non-PUA (fail open, not closed)
- Existing packages without PUA classification are completely unaffected

### Compatibility

- No breaking changes — all PUA features are additive
- Older clients ignore `PuaClassification` manifest field
- GPO policies default to `Warn` (current-equivalent behavior for previously-rejected packages is now "allowed with warning" rather than "silently rejected")
- Packages currently excluded from winget-pkgs can now be submitted once allowlisted

### Performance, Power, and Efficiency

- PUA classification is part of manifest metadata — no additional network calls
- Allowlist is bundled with source index data — O(1) lookup
- Warning display adds negligible time to installation flow

## Potential Issues

1. **Liability** — Microsoft explicitly allowing "PUA" software could create legal/PR perception issues. Mitigation: disclaimers, "approved for repository" ≠ "endorsed by Microsoft", and user consent at install.
2. **Category creep** — Pressure to allowlist more packages. Strict governance and annual re-review mitigate.
3. **SmartScreen reclassification** — Defender updates may add new detection categories to previously-clean packages. Need automated re-scan and notification process.
4. **Defender conflict** — Enterprise running Defender PUA blocking AND WinGet PUA allowlisting may see "WinGet says yes, Defender says no." Documentation must clarify that Defender enforcement is orthogonal and takes precedence at the OS level.
5. **User confusion** — "Why is WinGet warning me about software it's offering?" Clear language needed: "reviewed and approved" + explanation of why the category exists.

## Future Considerations

- **Reputation scoring** — Packages earn trust over time (install count, age, no reports) → warnings reduced
- **User-local allowlist** — Personal suppress list separate from GPO
- **Community voting** — Signal trust in PUA packages (upvotes reduce warning prominence)
- **Integration with Defender for Endpoint** — Enterprise visibility into PUA-classified packages across fleet

## Resources

- Issue: https://github.com/microsoft/winget-cli/issues/6189
- Related — Remote access tools: https://github.com/microsoft/winget-cli/issues/6107
- Related — Encrypted file detections: https://github.com/microsoft/winget-cli/issues/6250
- Microsoft PUA criteria: https://learn.microsoft.com/microsoft-365/security/defender-endpoint/detect-block-potentially-unwanted-apps-microsoft-defender-antivirus
- SmartScreen docs: https://learn.microsoft.com/windows/security/operating-system-security/virus-and-threat-protection/microsoft-defender-smartscreen/
