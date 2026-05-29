---
applyTo: "doc/specs/*"
---

# Writing WinGet Specifications

## Template

All specifications must follow the template at `doc/specs/spec-template.md`. Use the YAML front matter and all required sections. Do not omit sections — if a section is not applicable, include it with a brief explanation of why.

## File Naming

Specs are named `#<issue-id> - <Short Description>.md`. The issue ID must correspond to an existing GitHub issue in this repository.

Example: `#3483 - UserMessages.md`

## Front Matter

Every spec must include YAML front matter with:

```yaml
---
author: <name> <github-id>
created on: <yyyy-mm-dd>
last updated: <yyyy-mm-dd>
issue id: <github issue id>
---
```

If an AI agent (e.g., GitHub Copilot) assisted in authoring the spec, include it in the author field:

```yaml
---
author: <name> <github-id>, GitHub Copilot <Copilot>
created on: <yyyy-mm-dd>
last updated: <yyyy-mm-dd>
issue id: <github issue id>
---
```

Update `last updated` to the current date (format: `yyyy-mm-dd`) whenever the spec is modified. Do not append notes or descriptions — the value must be a plain date only.

## Required Sections

Follow the template structure in order:

1. **Title** — `# Spec Title` followed by a link to the issue: `For [#1234](https://github.com/microsoft/winget-cli/issues/1234)`
2. **Abstract** — Brief summary of what the spec proposes. 2-3 sentences.
3. **Inspiration** — Why this change is needed. Reference community feedback, issues, or limitations.
4. **Solution Design** — The detailed technical design. This is the largest section. Include:
   - Schema changes (JSON schema snippets for manifest fields)
   - Client behavior (CLI commands affected and how they behave)
   - COM API surface changes (IDL definitions)
   - PowerShell cmdlet changes (affected cmdlets, interactive vs. pipeline behavior, output objects)
   - Settings changes (new or modified settings with JSON examples and interaction tables)
   - Validation pipeline impact (winget-pkgs automated validation, non-interactive behavior)
   - Manifest examples (YAML snippets showing real-world usage)
   - Flow behavior matrix (table covering all commands, interactivity modes, and edge cases)
5. **UI/UX Design** — What the user sees. Include terminal output examples showing exact formatting.
6. **Capabilities** — Subsections for Accessibility, Security, Reliability, Compatibility, and Performance/Power/Efficiency.
7. **Potential Issues** — Known risks, trade-offs, and concerns.
8. **Deprecation Path** (when applicable) — If the spec replaces an existing field or feature, include a phased deprecation table.
9. **Future Considerations** — What this enables for later. Use this for ideas that are explicitly out of scope.
10. **Resources** — Links to related issues, documentation, schema files, and external references.

## Content Guidelines

### Areas of Impact

A complete specification should address all areas the feature touches. For WinGet, this typically includes:

- **CLI commands** — `install`, `upgrade`, `uninstall`, `show`, `validate`, `import`, `export`
- **COM API** — `Microsoft.Management.Deployment` interfaces and WinRT projections
- **PowerShell cmdlets** — `Install-WinGetPackage`, `Update-WinGetPackage`, `Uninstall-WinGetPackage`, `Repair-WinGetPackage`, `Find-WinGetPackage`, `Show-WinGetPackage`
- **Settings** — User settings in `settings.json` (see `doc/Settings.md`)
- **CLI arguments** — When adding a new setting, also consider a corresponding CLI argument (and vice versa). Settings define defaults; arguments provide per-invocation overrides.
- **Manifest schema** — JSON schema files in `schemas/JSON/manifests/`
- **Validation** — The winget-pkgs community repository validation pipeline
- **WinGet Configuration / DSC** — Declarative configuration flows
- **Group Policy** — Admin policy controls if applicable

### Schema and Versioning

- Manifest schema changes require a new minor version (e.g., 1.28.0 → 1.29.0).
- WinGet does not make breaking changes in 1.X releases. New fields must be additive and optional.
- Older clients silently ignore unknown fields. Do not rely on older clients reading new fields.
- The winget-pkgs community repository enforces schema version n or n-1 for new submissions. This is the practical mechanism that drives publisher adoption of new fields.

### Cross-Repository Impact

Schema changes in `winget-cli` have downstream impact across several repositories. A complete specification should address:

- **winget-cli** — Client implementation, schema files, settings, and CLI arguments.
- **winget-create** — The manifest creation tool must be updated to support new schema fields so publishers can author manifests with the new data.
- **winget-cli-restsource** — The REST source reference implementation must be updated to support new schema fields so enterprise REST sources can serve the new data. Enterprise scenarios do not carry the same deprecation concerns as the community repository since organizations control their own source update cadence.
- **winget-pkgs** — The community repository's validation pipeline must be updated to accept (and validate) new fields. The n/n-1 schema version acceptance policy at winget-pkgs is the practical driver for publisher adoption and eventual deprecation of replaced fields.

When writing the spec, describe the end-to-end customer impact across these repositories so reviewers understand the full scope of work required.

### Interactivity

WinGet has multiple interactivity modes that specifications must account for:

- **Interactive** (default) — Prompts are shown and user input is accepted.
- **`--no-vt`** — I/O stream is available but VT (virtual terminal) escape sequences are not processed. This is a layer between fully interactive and non-interactive — specs should consider whether features need special handling when VT is unavailable.
- **`--disable-interactivity`** — No prompts; operations proceed automatically.
- **`--silent`** — Implies non-interactive.
- **COM API** — Never prompts; returns data for the consumer to handle.
- **PowerShell `-Force`** — Suppresses confirmation prompts.
- **WinGet Configuration** — Inherently non-interactive.

### GitHub Markdown Features

Use GitHub alert syntax for important callouts:

```markdown
> [!NOTE]
> Supplementary information.

> [!IMPORTANT]
> Critical information users need to know.

> [!WARNING]
> Potential risks or dangerous actions.
```

### Deprecation

When proposing a replacement for an existing field or feature:

- Define a phased deprecation path (Introduction → Transition → Deprecation → Removal).
- Make timelines conditional on adoption telemetry, not fixed dates.
- Clearly state that WinGet 1.X will not make breaking changes.
- Explain that the community repository schema version policy (n/n-1) is the practical driver for adoption.

## Spelling

The repository uses `check-spelling`. If your spec introduces uncommon words (proper names, technical terms), add them to `.github/actions/spelling/expect.txt` in alphabetical order.

## Pull Request

- Spec PRs should be created as **draft** PRs.
- The PR body should include a "Related Issues" section with an unordered list of related issues (do not use "Closes" or "Fixes" — a spec does not close an issue).
- Specs require review and approval before implementation begins.
