---
author: AmelBawa-msft, GitHub Copilot <Copilot>
created on: 2026-09-28
last updated: 2026-10-01
issue id: 5345
---

# Interactive package selection

For [#5345](https://github.com/microsoft/winget-cli/issues/5345)

## Abstract

Let users resolve ambiguous package matches without restarting their command. An experimental setting enables numbered choices for single-package `install`, `show`, and `download` when interactive input and output are available.

## Inspiration

The same query can match multiple packages, including packages from different sources. Users should be able to choose deliberately without copying an ID into another invocation.

## Solution Design

This feature is disabled by default. Enable it in settings:

```json
{
  "experimentalFeatures": {
    "interactivePackageSelection": true
  }
}
```

Apply existing search matching and source-priority rules first. If multiple candidates remain, eligible CLI call sites opt into selection. Shared workflows remain noninteractive by default.

Display candidates in their existing order with stable, one-indexed numbers. A valid number selects the existing package object without searching again. Preserve command options and continue normal version selection, applicability checks, and agreement handling.

| Situation | Behavior |
| --- | --- |
| Experimental feature disabled (default) | No selection prompts; retain existing ambiguity errors. |
| No match | Existing no-match error. |
| One match after existing policy | Continue without prompting. |
| Multiple matches for single-package `install`, `show`, or `download` | Prompt if eligible, including `show --versions`. |
| Truncated results | Retain ambiguity error and request refinement. |
| Invalid or empty input | Explain the valid range and prompt again; no default. |
| `0` | Cancel without acting on a package. |
| Ctrl+C | Cancel immediately, including while waiting for input. |
| EOF or input failure | Report the existing prompt input error. |
| `--disable-interactivity`, interactivity disabled in settings or context | Retain ambiguity error without reading input. |
| `--silent` | Controls installer UI, not selection prompts; normal interactivity rules apply. |
| Redirected input or output, or disabled informational output | Do not prompt. |
| `--no-vt` | Use the same text and numeric input without terminal escape sequences. |
| Multi-package operations, including individual package contexts | No disambiguation prompts, either per package or up front; retain existing ambiguity errors. |
| Upgrade, uninstall, repair, pin, search, list, or completion | Preserve existing behavior. |
| COM API, PowerShell cmdlets, or configuration/DSC | No new prompts or API changes. |

Apart from the experimental setting, the prompt adds no command-line flags, group policies, manifest fields, or schema versions. Existing interactivity controls and the experimental-features group policy apply. Package validation pipelines and manifest authoring tools are unchanged; manifest examples and schema snippets are not applicable.

## UI/UX Design

Use the existing search table layout with a leading selection number. Always show the Source column, including when all candidates use the same source. For example:

```text
Multiple packages match. Choose one to install.

# Name           Id                 Version Source
--------------------------------------------------
1 Contoso Editor Contoso.Editor     2.4.0   winget
2 Contoso Editor Contoso.Editor.Pro 2.4.0   winget

Enter a number (1-2), or 0 to cancel: 1
Selected: Contoso Editor [Contoso.Editor]
```

For candidates spanning sources:

```text
# Name           Id             Version Source
-----------------------------------------------
1 Contoso Editor Contoso.Editor 2.4.0   winget
2 Contoso Editor Contoso.Editor 2.4.0   private
```

Each source row always shows that source's package name, ID, and version. Additional sources within one candidate appear on continuation rows without another selection number. Use action-specific introductory text for viewing or downloading. Do not add another confirmation after selection. Existing consent prompts still apply.

When prompting is unavailable, retain the candidate list and explain:

```text
Specify a package with --id <ID> --exact --source <SOURCE>.
```

Displayed versions are source metadata, not a promise of installer applicability. The original version option remains authoritative.

## Capabilities

### Accessibility

Numeric, line-oriented input works without color, cursor navigation, or arrow keys. All identifying information is text and uses localized labels. Invalid input includes recovery instructions.

### Security

There is no default selection or inferred equivalence. Choosing a package does not accept agreements or bypass existing source, trust, or installer checks.

### Reliability

Selection uses the displayed candidate object rather than re-running a potentially different search. EOF fails explicitly; cancellation never starts installation or download.

### Compatibility

The feature is disabled by default. After opt-in, scripts can preserve ambiguity errors with `--disable-interactivity`, or avoid ambiguity with exact ID and source selectors.

### Performance, Power, and Efficiency

Rendering uses available package metadata, without downloading manifests for display. No terminal redraw loop is required.

## Potential Issues

Long candidate lists require scrolling. Narrow terminals truncate table cells using the existing formatter; users can widen the terminal or cancel and refine their query if candidates are indistinguishable. The selection message includes the full name and ID. Source-defined result truncation must not be presented as a complete selectable list. Matching names and IDs do not prove that packages from different sources are equivalent.

## Future Considerations

Cross-source equivalence heuristics, arrow-key navigation, and selection for installed-package operations are separate changes.

## Resources

- [Package matching background](%23292%20-%20winget%20should%20install%20an%20app%20if%20there%20is%20an%20exact%20match.md)
- [Settings reference](../Settings.md)
