---
author: AmelBawa-msft, GitHub Copilot <Copilot>
created on: 2026-10-06
last updated: 2026-10-06
issue id: 5345
---

# Cross-source package deduplication

For [#5345](https://github.com/microsoft/winget-cli/issues/5345)

## Abstract

An experimental heuristic resolves ambiguous package matches across sources using versions and installer SHA256 hashes, with a few additional checks. It keeps the selected source's manifest intact; unresolved cases retain the existing prompt or error.

## Inspiration

Equivalent packages can have different identifiers, names, URLs, and metadata. Source priority and interactive selection help resolve ambiguity but do not identify equivalent installers automatically.

## Solution Design

### Settings and scope

`packageDeduplication` defaults to `false`, independently of `interactivePackageSelection`:

```json
{
  "experimentalFeatures": {
    "packageDeduplication": true
  }
}
```

The existing experimental-features group policy applies; there is no new policy or CLI flag. Honor `--source`, search filters, and source priority before comparison.

Require two or three complete, failure-free candidates, exactly one per source, with source identities present in the opened source order. Otherwise perform no heuristic manifest evaluations. Never compare lower-priority candidates, partially remove matches, or merge search rows. Command exclusions are listed below.

### Comparison rules

Use existing command-specific version, channel, pin, and installer-applicability rules. Compare the preferred installer, not any installer with a matching hash. For install without an explicit version, evaluate only the first permitted version; an inapplicable installer or missing requested version leaves the set unresolved. Never probe older manifests.

Support only direct `exe`, `burn`, `inno`, `nullsoft`, `msi`, `wix`, and `msix` installers.

Normal applicability still respects architecture, scope, locale, installer-type requests, and operating-system support. Their raw metadata need not match across sources.

Every candidate must have:

1. Equal resolved versions under existing version comparison (`2.4` equals `2.4.0`) and case-insensitively equal channels. Reject empty, `Unknown`, `Latest`, or approximate versions. Unknown search metadata is acceptable if retrieval resolves a concrete version.
2. Identical 32-byte, nonzero installer SHA256 values. Names, IDs, URLs, product codes, and package-family names cannot substitute for these signals.

Apply only these additional checks:

| Check | Rule and reason |
| --- | --- |
| Execution family | Use existing installer compatibility: EXE/Burn/Inno/Nullsoft, MSI/Wix, or MSIX. Different families use different execution mechanisms. |
| Declared scope | For scope-sensitive types, reject any set containing both explicit user and machine scope, even with an unspecified third candidate. Missing scope alone is allowed; scope-agnostic types retain command-controlled behavior. |
| Operation restrictions | Reject download prohibition during download. For install, reject elevation prohibition conflicting with the current context or a supplied unsupported argument normally treated as an error. Unused or warning-only arguments do not block selection. |
| Automatic dependencies | If install/download would process package dependencies, any such dependency leaves ambiguity: identical IDs can resolve differently across sources. For install enabling Windows features, require equal case-insensitive feature-ID sets, ignoring order. Skip these checks for show or when dependency processing is disabled. Download does not enable features; reported-only Windows library/external dependencies do not block selection. |

Other metadata differences do not block selection once normal applicability passes. Do not compare switches, generated arguments, success/return-code tables, or authentication metadata for equality. The selected source's instructions remain authoritative, even when differences affect execution or exit-code interpretation. This is payload-based selection, not a guarantee of identical outcomes.

### Selection and failures

If every candidate passes, choose the participating source earliest in the existing opened source order, regardless of result-row order. Keep its identity and exact version, manifest, and installer without merging metadata.

Reuse that result for this invocation: if Alpha publishes 2.5 after comparing Alpha/Bravo 2.4, continue with the checked 2.4 result. Do not fetch another manifest or select "latest" again.

A failed or unevaluable check preserves all candidates and existing command state. Expected manifest retrieval/parsing failures warn and log; cancellation stops; unexpected failures propagate. Authentication, agreement, or installer failures never trigger fallback to another source.

### Settings and behavior matrix

For supported commands when policy allows:

| Deduplication | Interactive selection | Behavior with multiple results |
| --- | --- | --- |
| Off | Off | Existing ambiguity error. |
| Off | On | Existing prompt/error rules. |
| On | Off | Resolve if checks pass; otherwise error. |
| On | On | Try deduplication before existing prompt/error rules. |

| Situation | Behavior |
| --- | --- |
| Single-package CLI install/download/ordinary show | Eligible. |
| Zero/one match, four or more, repeated source, truncation, source failure, or unknown source identity | No heuristic evaluations. |
| `show --versions` | Existing interactive selection only. |
| Other commands, local manifests, multi-package/dependency contexts | No deduplication. |
| Installed associations, install that may upgrade, or multiple source identities within a candidate | No deduplication. |
| COM, PowerShell cmdlets (including `-Force`), configuration/DSC, completion | Unchanged. |
| Disabled interactivity, redirected input/output, or unavailable prompt output | Passing candidates resolve without input; unresolved matches error. |
| `--no-vt` | Same behavior using plain text. |
| `--silent` | Controls installer UI, not selection prompts. |
| Experimental features disabled by policy | Neither experimental selection feature runs. |

### Schemas, authoring, and validation

Only the settings schema changes. Manifest schemas, COM IDL, PowerShell surfaces, validation pipelines, manifest authoring tools, and REST source reference tooling remain unchanged.

Alpha can publish this existing-format manifest:

```yaml
PackageIdentifier: Contoso.Editor
PackageVersion: 2.4.0
InstallerType: exe
Scope: user
Installers:
- Architecture: x64
  InstallerUrl: https://alpha.example/editor.exe
  InstallerSha256: A91F43BBD50C2A147216D7C2F7A06E0EBD7DF95A08C5CA6F0AB6BEE06C9E4C20
ManifestType: installer
ManifestVersion: 1.10.0
```

Bravo can use a different ID/URL with the same version/hash. These domains and hash are fictional.

### Acceptance cases

Assume eligible candidates with matching version/channel/hash and all other checks passing unless stated otherwise. "Unresolved" preserves the whole set.

| Case | Expected result |
| --- | --- |
| Different names/IDs/URLs, switches, return codes, or authentication metadata | Resolve; retain selected-source instructions. |
| Different version/channel/hash, invalid hash, or unresolved version | Unresolved. |
| EXE/Inno versus EXE/MSI | Resolve versus unresolved. |
| User/machine scope versus unspecified/user, both applicable | Unresolved versus resolve. |
| Download prohibited during download versus show | Unresolved versus resolve. |
| Package dependencies processed versus skipped | Unresolved versus resolve. |
| Windows feature sets differ versus only order/case differs | Unresolved versus resolve during install. |

## UI/UX Design

On selection, emit localized text, then continue normal command output:

```text
Equivalent installers found. Selected: Contoso Editor [Contoso.Editor] from source 'Alpha'.
```

For expected manifest retrieval/parsing failures:

```text
Could not compare installers from all matching sources. Automatic selection was skipped.
```

Unresolved cases retain the existing prompt/error, candidate table, and refinement guidance:

```text
Specify a package with --id <ID> --exact --source <SOURCE>.
```

Ordinary mismatches produce only verbose diagnostics.

## Capabilities

### Accessibility

Use localized, line-oriented text without requiring color, cursor movement, or new input.

### Security

Hashes and source order do not establish trust. Preserve source boundaries, authentication, agreements, policy, and installer hash/signature checks.

### Reliability

Retain the checked result to prevent a race condition. Comparison must not download binaries, authenticate installers, accept agreements, resolve dependencies, or execute installers.

### Compatibility

Default-off and excluded-call behavior stays unchanged. `--disable-interactivity` prevents prompts, not deduplication; use `--source` or disable the setting to avoid automatic cross-source selection. No surfaces are deprecated.

### Performance, Power, and Efficiency

Evaluate at most one target manifest per candidate, at most three total, excluding pre-existing search/correlation traffic. No historical probing or persistent cache.

## Potential Issues

Missing scope is not proof of identical scope. Package dependencies intentionally limit coverage. Propagating child truncation through both available-only and installed-aware searches can also suppress existing prompts that would otherwise show incomplete choices.

## Future Considerations

Cross-version matching, dependency equivalence, persistent correlation, search-result grouping, additional installer types, and API deduplication remain separate proposals.

## Resources

- [Deduplicating results from multiple sources](https://github.com/microsoft/winget-cli/issues/5345)
- [Matching and source-priority discussion](https://github.com/microsoft/winget-cli/issues/6435)
- [Existing multi-source matching and interactive selection](https://github.com/microsoft/winget-cli/pull/6577)
- [Interactive package selection](%235345%20-%20Interactive%20package%20selection.md)
- [Settings reference](../Settings.md)
