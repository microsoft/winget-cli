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

1. **CVE lookup** — The private validation infrastructure queries vulnerability databases using internal package-to-CVE mappings
2. **Known CVE flagging** — If the submitted version has known CVEs:
   - Add a `Security-CVE` label to the PR
   - Post a bot comment listing CVEs with CVSS scores
3. **Severity-based workflow:**
   - Critical (CVSS ≥ 9.0): **Blocked** — submission rejected, no waiver available
   - High/Medium/Low (CVSS < 9.0): Informational — submission proceeds with the label for visibility

### Part 2: Client CVE Reporting

#### New command: `winget security`

```
winget security scan [--source <source>] [--severity <minimum>]
winget security show <package-id>
```

#### Integration with existing commands

| Command | CVE Behavior |
|---------|-------------|
| `winget list` | CVE column shown via `--details` |
| `winget upgrade` | Security-relevant upgrades highlighted with ⚠️; `--security` flag to upgrade only packages with security fixes |
| `winget install --version` | Non-blocking warning when version has known CVEs (blocking if GPO `CVEBlockInstallThreshold` is set and CVSS meets threshold) |
| `winget show` | CVE details shown in `--details` output |
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

The mapping between WinGet package IDs and vulnerability database entries is maintained by Microsoft's private validation infrastructure:

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
- Maintained internally by Microsoft — community contributions to CVE mappings are **disallowed by policy**
- Updated as part of the private validation infrastructure's periodic rescan process
- Delivered to clients via the source index (same distribution path as other backend-managed metadata)
- Not present in the public `winget-pkgs` manifest files

> [!NOTE]
> Mapping WinGet package IDs to PURLs/CPEs is a known challenge. Many NVD entries have inaccurate version ranges, and GHSA's global database doesn't cover Windows-specific package formats well. Microsoft's internal infrastructure uses multiple data sources and manual curation to improve accuracy over time.

### Part 4: CVE Metadata in Merged Manifests

CVE metadata is added to the back-end **merged manifest** by Microsoft's private infrastructure — the same mechanism used for package icons and other enrichment metadata. This metadata is NOT authored by community contributors and is NOT present in the public `winget-pkgs` repository manifest files.

**How it works:**

1. Microsoft's validation infrastructure periodically rescans packages against vulnerability databases.
2. When CVEs are detected (or when existing CVE metadata changes), the merged manifest stored on Azure blob storage is updated with the security metadata.
3. The PreIndexed package source index is regenerated to reflect the updated merged manifest, ensuring the hash in the source still matches the stored blob.

**Merged manifest security metadata format:**

```yaml
Security:
  Advisories:
    - Id: CVE-2024-32002
      Cvss: 9.8
      FixedIn: "2.45.1"
      Description: "Remote code execution via crafted input"
    - Id: CVE-2023-22490
      Cvss: 5.5
      FixedIn: "2.39.2"
      Description: "Path traversal in clone"
  AdvisoryUrl: https://github.com/git/git/security/advisories
```

**Pipeline hash reconciliation:**

When a periodic rescan detects new CVEs and updates the merged manifest, the publishing pipeline must:
1. Regenerate the merged manifest with updated security metadata
2. Recompute the content hash
3. Update the PreIndexed package source to reference the new hash
4. Publish the updated source index

This requires additional publishing pipeline functionality to support out-of-band manifest updates (updates triggered by rescan rather than by a new PR submission).

### Part 5: Group Policy Controls

| Policy | Type | Default | Description |
|--------|------|---------|-------------|
| `EnableCVEDetection` | Bool | Enabled | Master toggle for all CVE features |
| `CVEBlockInstallThreshold` | Float | 0 (disabled) | Block installs when any CVE has CVSS ≥ this value (e.g., 7.0) |
| `CVEBlockUpgradeThreshold` | Float | 0 (disabled) | Block upgrades to versions with CVSS ≥ this value |
| `CVEScanFrequency` | Int | 1440 | Cache refresh interval in minutes |
| `CVEReportingEndpoint` | String | Empty | URL to POST scan results for fleet visibility |

### CLI Arguments

| Argument | Commands | Description |
|----------|----------|-------------|
| `--ignore-security-warnings` | install, upgrade | Proceed despite CVE warnings |
| `--security` | upgrade | Only upgrade packages with available security fixes |
| `--severity` | security scan | Minimum CVSS score to report (e.g., `--severity 7.0`) |

### Settings

```json
{
  "security": {
    "enableCVEDetection": true,
    "minimumReportCvss": 4.0,
    "cacheRefreshMinutes": 1440,
    "reportingEndpoint": ""
  }
}
```

| Setting | CLI Argument | GPO Policy | Interaction |
|---------|-------------|------------|-------------|
| `enableCVEDetection` | N/A | `EnableCVEDetection` | GPO wins |
| `minimumReportCvss` | `--severity` | N/A | Arg overrides setting |
| `cacheRefreshMinutes` | N/A | `CVEScanFrequency` | GPO wins |

### COM API Surface

```idl
interface IPackageSecurityInfo
{
    IVectorView<SecurityAdvisory> Advisories { get; };
    Double HighestCvss { get; };
    Boolean HasKnownVulnerabilities { get; };
}

interface ISecurityAdvisory
{
    String Id { get; };           // CVE-YYYY-NNNNN
    Double CvssScore { get; };
    String Description { get; };
    String FixedInVersion { get; };
    String AdvisoryUrl { get; };
}
```

### PowerShell Cmdlets

```powershell
# Scan installed packages
Get-WinGetSecurityScan [-Source <String>] [-MinimumCvss <Double>]

# Get security info for a specific package (included in --details output)
Get-WinGetPackage -Id "Git.Git" | Select-Object Id, InstalledVersion, SecurityInfo

# Upgrade only packages with security fixes
Update-WinGetPackage -All -Security
```

### Cross-Repository Impact

- **winget-cli** — CVE engine, `winget security` command, GPO policies, settings, COM API additions
- **winget-pkgs** — Validation pipeline integration (CVE lookup during submission), `Security-CVE` label, bot comments
- **winget-cli-restsource** — REST API extension for serving CVE metadata from merged manifests
- **winget-create** — No impact (CVE metadata is backend-managed, not author-managed)
- **Publishing pipeline** — New capability: out-of-band manifest updates triggered by periodic rescan (hash reconciliation for updated merged manifests)

### Schema Version

The CVE metadata lives in the merged manifest (backend-managed, not in public repository manifests). No community-facing manifest schema version change is required. The client reads CVE data from the source index where it is delivered alongside other backend-enriched metadata.

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

⚠️ 2 packages have security updates. Run 'winget upgrade --all --security' to apply security fixes only.
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

1. **Package-to-CVE mapping accuracy** — WinGet package IDs don't directly correspond to PURLs/CPEs. Many NVD entries have inaccurate version ranges, and GHSA's global database doesn't cover Windows-specific package formats well. Microsoft's internal curation improves accuracy over time but gaps will exist.
2. **False positives** — A CVE may apply to a specific platform/build but not the Windows version distributed via WinGet (e.g., Linux-only CVE for cross-platform package). CVSS scoring and specificity metadata can filter.
3. **Warning fatigue** — Too many warnings for lower-CVSS issues may desensitize users. Default `minimumReportCvss: 4.0` helps; enterprises can raise the threshold.
4. **Data freshness** — NVD can lag days behind disclosure. GHSA is faster for GitHub-hosted projects. Microsoft's periodic rescan frequency determines how quickly CVE data propagates to clients.
5. **Privacy** — Bulk-download model avoids per-package queries but requires local storage (~5-10 MB for advisory database).
6. **Pipeline hash reconciliation** — Out-of-band merged manifest updates (from periodic rescan) require the publishing pipeline to recompute hashes and republish source indexes without a corresponding PR submission. This is new pipeline functionality that must be designed carefully to avoid race conditions with ongoing submissions.
7. **Reporting endpoint trust** — Enterprise reporting endpoints must be configured by GPO to prevent data exfiltration of installed software inventory.

## Future Considerations

- **SBOM generation** — `winget security sbom` generates Software Bill of Materials
- **Automated remediation** — Scheduled upgrades for security-critical packages with GPO controls
- **Microsoft Defender integration** — Feed CVE data into Defender for Endpoint vulnerability management
- **Supply chain attestation** — Validate package signatures and provenance
- **REST source CVE serving** — REST source implementations could serve CVE data directly, enabling enterprise REST sources to provide their own vulnerability assessments
- **Reporting endpoint examples** — Community-contributed example servers for the `CVEReportingEndpoint` feature (fleet-wide vulnerability dashboards)

## Resources

- Original issue: https://github.com/microsoft/winget-cli/issues/2204
- GitHub Advisory Database API: https://docs.github.com/en/rest/security-advisories
- NVD API: https://nvd.nist.gov/developers
- PURL specification: https://github.com/package-url/purl-spec
- CPE specification: https://csrc.nist.gov/projects/security-content-automation-protocol/specifications/cpe
- Ecosystem comparisons: npm audit, pip-audit, cargo-audit, Dependabot
