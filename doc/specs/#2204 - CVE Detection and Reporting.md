---
author: Demitrius Nelon denelon, GitHub Copilot Copilot
created on: 2026-06-17
last updated: 2026-06-17
issue id: 2204
---

# CVE Detection in Validation and Client Reporting

For [#2204](https://github.com/microsoft/winget-cli/issues/2204).

## Abstract

Integrate CVE (Common Vulnerabilities and Exposures) detection into the WinGet ecosystem at two levels: the `winget-pkgs` validation pipeline (flagging packages with known CVEs during submission) and the WinGet client (informing users when installed packages have known vulnerabilities). Group Policy controls enable enterprise management of blocking behavior and reporting.

## Inspiration

WinGet manages software installations for millions of Windows users but provides zero signal about known vulnerabilities. This gap is notable because:

- Enterprise security teams cannot use WinGet output to identify vulnerable software on managed devices
- The `winget-pkgs` repository accepts manifest updates without checking whether the version has disclosed CVEs
- Other package managers provide this capability (npm audit, pip-audit, cargo-audit, Dependabot)
- Software supply chain security (SLSA, SBOM) is a growing enterprise requirement
- WinGet is becoming critical infrastructure for Windows software management

## Solution Design

### Part 1: Validation Pipeline CVE Detection

During manifest validation in the `winget-pkgs` pipeline:

1. **CVE lookup** — Query the GitHub Advisory Database (GHSA) using the package identifier and version via PURL or CPE mapping
2. **Known CVE flagging** — If the submitted version has known CVEs:
   - Add a `Security-CVE` label to the PR
   - Post a bot comment listing CVEs with severity ratings (CVSS score)
   - Do NOT auto-reject — moderators approve with acknowledgment
3. **Severity-based workflow:**
   - Critical/High (CVSS ≥ 7.0): Require explicit moderator approval
   - Medium (CVSS 4.0–6.9): Warning, auto-approve still possible
   - Low (CVSS < 4.0): Informational only

### Part 2: Client CVE Reporting

#### New command: `winget security`

```
winget security scan [--source <source>] [--severity <minimum>]
winget security show <package-id>
```

#### Integration with existing commands

| Command | CVE Behavior |
|---------|-------------|
| `winget list` | `--include-security` flag adds CVE column |
| `winget upgrade` | Security-relevant upgrades highlighted with ⚠️ |
| `winget install --version` | Non-blocking warning when version has known CVEs |
| `winget show` | `--security` flag shows CVE details |
| `winget configure test` | Reports CVE compliance status per resource |

#### Data Source Architecture

```
┌─────────────────────────────────┐
│ WinGet Client                   │
│  ┌───────────────────────────┐  │
│  │ CVE Engine                │  │
│  │ - Package→PURL mapper     │  │
│  │ - Advisory DB client      │  │
│  │ - Local cache (SQLite)    │  │
│  │ - Policy evaluator        │  │
│  └─────────────┬─────────────┘  │
└────────────────┼────────────────┘
                 │
    ┌────────────┼────────────┐
    ▼            ▼            ▼
┌────────┐  ┌────────┐  ┌──────────────┐
│ GHSA   │  │ NVD    │  │ Enterprise   │
│ API    │  │ API    │  │ Endpoint     │
└────────┘  └────────┘  └──────────────┘
```

- **Primary**: GitHub Advisory Database (GHSA) — fast updates, good OSS coverage
- **Secondary**: National Vulnerability Database (NVD) — broader coverage
- **Mapping**: Package ID → PURL (Package URL) for lookup. Mapping maintained as metadata in source index.
- **Cache**: Local SQLite database with configurable TTL (default: 24 hours)
- **Sync**: Bulk download model (not per-query) to avoid revealing installed software inventory

### Part 3: Package-to-CVE Mapping

The mapping between WinGet package IDs and vulnerability database entries requires a translation layer:

```json
{
  "Git.Git": {
    "purl": "pkg:github/git/git",
    "cpe": "cpe:2.3:a:git-scm:git:*:*:*:*:*:*:*:*"
  },
  "Python.Python.3.12": {
    "purl": "pkg:pypi/cpython",
    "cpe": "cpe:2.3:a:python:python:*:*:*:*:*:*:*:*"
  }
}
```

This mapping is:
- Maintained as part of the source index (community-contributed, reviewed)
- Incrementally updated with source updates (`winget source update`)
- Optional per-package — packages without mappings simply have no CVE data

### Part 4: Manifest Extension

Add an optional `Security` field to the package manifest schema (version 1.29.0):

```yaml
Security:
  Advisories:
    - Id: CVE-2024-32002
      Severity: Critical
      FixedIn: "2.45.1"
  AdvisoryUrl: https://github.com/git/git/security/advisories
```

This supplements automated database lookups for packages not yet indexed or where automated mapping is incomplete.

### Part 5: Group Policy Controls

| Policy | Type | Default | Description |
|--------|------|---------|-------------|
| `EnableCVEDetection` | Bool | Enabled | Master toggle for all CVE features |
| `CVEBlockInstallSeverity` | Enum | None | Block installs at or above severity (None/Low/Medium/High/Critical) |
| `CVEBlockUpgradeSeverity` | Enum | None | Block upgrades to versions with CVEs at/above severity |
| `CVEScanFrequency` | Int | 1440 | Cache refresh interval in minutes |
| `CVEReportingEndpoint` | String | Empty | URL to POST scan results for fleet visibility |
| `CVEDataSources` | MultiString | GHSA | Which databases to query |

### CLI Arguments

| Argument | Commands | Description |
|----------|----------|-------------|
| `--ignore-security-warnings` | install, upgrade | Proceed despite CVE warnings |
| `--include-security` | list, show | Show CVE information |
| `--severity` | security scan | Minimum severity to report |

### Settings

```json
{
  "security": {
    "enableCVEDetection": true,
    "minimumReportSeverity": "medium",
    "cacheRefreshMinutes": 1440,
    "dataSources": ["ghsa"],
    "reportingEndpoint": ""
  }
}
```

| Setting | CLI Argument | GPO Policy | Interaction |
|---------|-------------|------------|-------------|
| `enableCVEDetection` | N/A | `EnableCVEDetection` | GPO wins |
| `minimumReportSeverity` | `--severity` | N/A | Arg overrides setting |
| `cacheRefreshMinutes` | N/A | `CVEScanFrequency` | GPO wins |

### COM API Surface

```idl
interface IPackageSecurityInfo
{
    IVectorView<SecurityAdvisory> Advisories { get; };
    SecuritySeverity HighestSeverity { get; };
    Boolean HasKnownVulnerabilities { get; };
}

interface ISecurityAdvisory
{
    String Id { get; };           // CVE-YYYY-NNNNN
    SecuritySeverity Severity { get; };
    String Description { get; };
    String FixedInVersion { get; };
    String AdvisoryUrl { get; };
}

enum SecuritySeverity { None, Low, Medium, High, Critical };
```

### PowerShell Cmdlets

```powershell
# Scan installed packages
Get-WinGetSecurityScan [-Source <String>] [-MinimumSeverity <SecuritySeverity>]

# Get security info for a specific package
Get-WinGetPackage -Id "Git.Git" -IncludeSecurityInfo
```

### Cross-Repository Impact

- **winget-cli** — CVE engine, `winget security` command, GPO policies, settings, COM API additions
- **winget-pkgs** — Validation pipeline integration, `Security-CVE` label, bot comments, PURL mapping data
- **winget-cli-restsource** — REST API extension for serving CVE mapping data
- **winget-create** — Support for `Security` manifest field in manifest authoring

### Schema Version

This feature requires manifest schema version 1.29.0 for the optional `Security` field. The CVE detection itself works without manifest changes (uses external database lookups).

## UI/UX Design

### `winget security scan` output:

```
> winget security scan
Scanning installed packages for known vulnerabilities...

Name            Id                Version   CVEs              Severity
───────────────────────────────────────────────────────────────────────
Git             Git.Git           2.44.0    CVE-2024-32002    Critical
Node.js         OpenJS.NodeJS     18.12.0   CVE-2023-44487    High
                                            CVE-2023-45143    Medium
Python          Python.Python.3   3.11.2    CVE-2023-27043    Medium

3 packages with known vulnerabilities (1 Critical, 1 High, 1 Medium).
Run 'winget upgrade' to see available fixes.
```

### `winget upgrade` with security:

```
> winget upgrade
The following packages have updates available:

Name        Id              Installed  Available  Source   Security
───────────────────────────────────────────────────────────────────
Git         Git.Git         2.44.0     2.45.1     winget   ⚠️ Critical
Node.js     OpenJS.NodeJS   18.12.0    18.20.3    winget   ⚠️ High
VS Code     Microsoft.VS..  1.90.0     1.91.0     winget

⚠️ 2 packages have security updates. Run 'winget upgrade --all' to apply.
```

### Blocking behavior (GPO-enabled):

```
> winget install OldPackage --version 1.0.0
This version has known vulnerabilities:
  CVE-2024-XXXXX (Critical): Remote code execution via crafted input

Your organization's policy blocks installation of packages with Critical vulnerabilities.
Use 'winget install OldPackage' without --version to install the latest safe version,
or use --ignore-security-warnings to override (if permitted by policy).
```

### Non-interactive / COM API:

The COM API never blocks — it returns security information for the caller to evaluate:

```csharp
var package = manager.GetPackage("Git.Git");
var security = package.SecurityInfo;
if (security.HighestSeverity >= SecuritySeverity.High) {
    // Handle in caller's UI
}
```

### `--disable-interactivity` behavior:

Warnings are emitted to stderr but do not prompt. Blocking (when GPO-enabled) returns a non-zero exit code without prompting.

## Capabilities

### Accessibility

- Security warnings use text labels (not just emoji/color) — screen readers announce severity levels
- `winget security scan --output json` provides structured data for programmatic consumption
- Warning text is in the localization resource file for translation

### Security

- This feature IS the security improvement — provides vulnerability awareness previously absent
- CVE cache uses integrity validation (signed responses from GHSA API)
- Reporting endpoint communication uses TLS 1.3+ with certificate validation
- No data sent to Microsoft unless enterprise explicitly configures reporting endpoint
- Bulk-download sync model prevents installed-software fingerprinting via query patterns

### Reliability

- Graceful degradation: if CVE databases are unreachable, operations proceed with a warning (never block on network failure)
- Cache ensures offline functionality with stale data rather than no data
- PURL mapping may have false positives — blocking mode requires GPO opt-in
- Source update failure does not prevent package operations

### Compatibility

- No breaking changes — all CVE features are additive and off-path when disabled
- Older clients ignore the `Security` manifest field
- GPO policies default to non-blocking behavior
- Existing automation scripts are unaffected unless `CVEBlockInstallSeverity` is configured

### Performance, Power, and Efficiency

- Local SQLite cache eliminates per-operation network calls after initial sync
- Background refresh aligned with `winget source update` (no additional scheduled tasks)
- Incremental sync — only fetch new advisories since last update timestamp
- `winget security scan` scans local cache only — O(n) where n = installed packages

## Potential Issues

1. **Package-to-CVE mapping accuracy** — WinGet package IDs don't directly correspond to PURLs/CPEs. Mapping will have gaps and false positives. Community contribution model for mapping data mitigates over time.
2. **False positives** — A CVE may apply to a specific platform/build but not the Windows version distributed via WinGet (e.g., Linux-only CVE for cross-platform package). Severity and specificity metadata can filter.
3. **Warning fatigue** — Too many Medium/Low warnings may desensitize users. Default `minimumReportSeverity: medium` helps; enterprises can raise to `high`.
4. **Data freshness** — NVD can lag days behind disclosure. GHSA is faster for GitHub-hosted projects. Using GHSA as primary mitigates.
5. **Privacy** — Bulk-download model avoids per-package queries but requires local storage (~5-10 MB for full GHSA database).
6. **Manifest `Security` field abuse** — Malicious/incorrect CVE entries in manifests could create false warnings. Validation pipeline must cross-check against actual databases.

## Future Considerations

- **SBOM generation** — `winget security sbom` generates Software Bill of Materials
- **Automated remediation** — Scheduled upgrades for security-critical packages with GPO controls
- **Microsoft Defender integration** — Feed CVE data into Defender for Endpoint vulnerability management
- **Supply chain attestation** — Validate package signatures and provenance
- **winget-pkgs community mapping** — Crowdsourced PURL/CPE mappings reviewed by moderators

## Resources

- Original issue: https://github.com/microsoft/winget-cli/issues/2204
- GitHub Advisory Database API: https://docs.github.com/en/rest/security-advisories
- NVD API: https://nvd.nist.gov/developers
- PURL specification: https://github.com/package-url/purl-spec
- CPE specification: https://csrc.nist.gov/projects/security-content-automation-protocol/specifications/cpe
- Ecosystem comparisons: npm audit, pip-audit, cargo-audit, Dependabot
