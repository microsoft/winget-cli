---
author: Przemysław Kłys <PrzemyslawKlys>
created on: 2026-09-19
last updated: 2026-09-19
issue id: 184
---

# Structured CLI output

For [#184](https://github.com/microsoft/winget-cli/issues/184)

## Abstract

Add an opt-in JSON output mode to commands that return package data, starting with `list` and the non-mutating form of `upgrade`. Define the data contract before shipping it, and put formatting in the reporter so later commands do not each invent their own JSON path.

## Inspiration

The PowerShell module and COM API already provide structured package data, but callers of `winget.exe` in other shells and tools still have to parse localized, width-dependent tables. [#6346](https://github.com/microsoft/winget-cli/pull/6346) demonstrates the first use case, while its review identified two things to settle first: a stable shape aligned with the PowerShell objects, and a design that scales beyond two commands.

This proposal is for CLI consumers; it does not replace the PowerShell module or COM API. PowerShell remains the richer option when an application can use it directly.

## Solution Design

### Scope and rollout

Introduce `--format json` behind an experimental feature setting. The first supported commands are `winget list` and `winget upgrade` when `upgrade` is only listing available updates. The default table output does not change. Until a command has a defined schema and a typed result, it must reject `--format json` as unsupported, rather than return an empty document that looks like a successful query. Mutating `upgrade` modes such as `--all` remain unsupported in this first slice.

The schema is reviewed with the first slice and stored with the client. The experimental gate gives room to correct the contract before it becomes stable; once stabilized, additions should be optional and existing field meanings should remain unchanged. Subsequent commands can adopt the same output machinery with command-specific result types and schemas.

### Data contract

One invocation writes one UTF-8 JSON document to stdout, followed by a newline. The envelope has a schema version, command name, package array, and error array. The initial proposal uses the PowerShell object's names and types for overlapping package fields. Envelope names are CLI-specific; they are not PowerShell package properties.

```json
{
  "schemaVersion": "1.0",
  "command": "list",
  "packages": [
    {
      "Name": "Example App",
      "Id": "Example.App",
      "InstalledVersion": "1.0.0",
      "AvailableVersions": ["1.1.0"],
      "IsUpdateAvailable": true,
      "Source": "winget"
    }
  ],
  "errors": []
}
```

`Name`, `Id`, `InstalledVersion`, `AvailableVersions`, `IsUpdateAvailable`, and `Source` correspond to `PSInstalledCatalogPackage` properties. `AvailableVersions` contains the versions known from the matched source; it must not be populated by splitting or interpreting a rendered table cell. `Source` is the actual source name when known, even when the human table omits its Source column. Use `null` when the source cannot be established, not an empty string that could be mistaken for a hidden table column. An empty successful result uses `"packages": []` and `"errors": []`.

The exact schema version and field casing are proposed here for review, not an assertion that the current implementation already emits this shape. The JSON schema should define required versus nullable fields, version string semantics, whether all available versions can be returned without extra catalog work, and whether `upgrade` needs any additional fields before implementation is finalized. Do not silently substitute only the selected upgrade candidate for `AvailableVersions` if PowerShell exposes more.

Failures should be typed objects, for example:

```json
{
  "schemaVersion": "1.0",
  "command": "list",
  "packages": [],
  "errors": [
    { "code": "0x8A15000F", "message": "Source data is missing.", "source": "winget" }
  ]
}
```

The example code and message are illustrative, not a new error mapping. `code` should carry the actual HRESULT in a fixed representation. Preserve the command's exit code. A partial source failure may return packages from healthy sources and an error entry for the failed source; a command-level failure still produces a parseable document when the reporter has been initialized. Catastrophic startup failures before argument parsing may remain outside this guarantee and should be documented separately.

### Reporter and table ownership

Select an output mode once in `Reporter`. Workflows pass typed results, warnings, and errors to it. The reporter serializes a command result to JSON, or renders the existing human output. Plain decorative strings are not JSON data and are suppressed from JSON stdout. `TableOutput` can consume typed rows and keep presentation choices such as column hiding and truncation on the text side; it must not be the source of truth for serialized fields. This avoids a new format branch at every output call site.

The JSON document should be finalized once per invocation, including empty and error cases. Progress, settings warnings, source notices, and diagnostic logs must not precede or follow it on stdout. Diagnostics can use stderr where appropriate; typed warnings that callers need should be represented in the schema rather than mixed into the JSON stream. Tests should parse stdout as one document, not search for a JSON substring.

Add a privacy-conscious telemetry event for use of the JSON mode, with command and success/failure category. Do not record package names, IDs, source contents, or the JSON payload. Existing telemetry controls continue to apply.

### Other surfaces and validation

No manifest fields, manifest JSON schemas, COM IDL interfaces, PowerShell cmdlets or output objects, WinGet Configuration resources, or group policies change in this proposal. The CLI JSON schema is separate from package manifests and from `winget export`'s existing interchange format; it is not a new manifest schema version. No `winget-create`, `winget-cli-restsource`, or `winget-pkgs` validation-pipeline change is required for the first slice.

The new `experimentalFeatures` key defaults to false. The final key name should follow the repository's feature naming convention; one possible settings shape is:

```json
{
  "experimentalFeatures": {
    "structuredOutput": true
  }
}
```

| Setting | `--format` | Result |
| --- | --- | --- |
| Off | Omitted | Existing text output |
| On | Omitted | Existing text output |
| On | `json` on supported command | JSON document |
| Off | `json` | Experimental-feature error |
| On | `json` on unsupported mode | Explicit unsupported-mode error |

The settings flag permits the CLI argument; it does not make JSON the default format. `--no-vt` changes only terminal decoration, not JSON data. `--disable-interactivity` and noninteractive hosts use the same JSON contract. If a query would require a prompt, the command should fail with a typed error rather than interleave a prompt with JSON. `--help` keeps its normal help output, as it describes the command rather than executing a package query.

| Command or mode | Interactive | `--no-vt` | Noninteractive | JSON behavior |
| --- | --- | --- | --- | --- |
| `list` | Supported | Supported | Supported | Installed package result |
| `upgrade` (listing) | Supported | Supported | Supported | Available-update result |
| `upgrade --all` or package execution | Not in first slice | Not in first slice | Not in first slice | Explicit unsupported-mode error |
| Other commands | Not in first slice | Not in first slice | Not in first slice | Explicit unsupported-command error |
| Empty result | Supported | Supported | Supported | Empty arrays, valid document |
| Source-open failure | Supported | Supported | Supported | Typed error, valid document where reporter is active |

### Implementation and test boundary

The first implementation PR should contain the CLI JSON schema and focused tests for schema validation, known source provenance on single- and multi-source systems, null/empty distinctions, package version fields, empty results, partial and command-level errors, exit codes, help, and unsupported execution modes. Existing table behavior must remain covered. The generic reporter path should be demonstrated by both `list` and listing-only `upgrade`, without duplicating serialization branches in each workflow.

## UI/UX Design

With the feature enabled, `winget list --format json` prints the single document shown above. `winget upgrade --format json` uses the same envelope with `"command": "upgrade"` and the agreed package schema. Without `--format json`, users see the current localized table. `--format json` on an unsupported command or mutating mode reports an explicit error and nonzero exit code; it must not silently perform an operation or present table text as JSON.

## Capabilities

### Accessibility

The existing text presentation remains the default, including its accessibility behavior. JSON is intended for tools and is not a replacement for readable command output.

### Security

JSON values must be escaped by a serializer, never assembled by string concatenation. The format option does not widen source access or bypass policy and agreement checks. Telemetry excludes package and source data.

### Reliability

One parseable document, explicit empty results, typed failures, and exit-code preservation give callers a reliable contract. Source provenance must come from the data model, not table-column visibility.

### Compatibility

No existing default output or PowerShell/COM contract changes. The option is experimental while the schema is under review. Published schema versions need an explicit compatibility policy before the gate is removed.

### Performance, Power, and Efficiency

The JSON path should not open each package again merely to recreate display data. Returning every available version may require catalog access beyond today's table; measure that cost and settle the field semantics before freezing the schema.

## Potential Issues

- The PowerShell object model and CLI query semantics are related but not identical. A field name should not imply stronger data than the CLI can obtain without extra source work.
- A partial result with source errors needs an unambiguous exit-code policy; consumers should inspect both the process status and `errors`.
- Some failures occur before the reporter can select JSON mode. The documented guarantee must have a clear startup boundary.
- A generic reporter change could alter text output accidentally. Keep table regression tests alongside JSON tests.

## Deprecation Path

No existing feature is deprecated. The human table stays the default, and this proposal does not replace the PowerShell module, COM API, or `winget export` format.

## Future considerations

After the first schema and reporter path are agreed, `search`, `show`, and other result-producing commands can add their own typed results and schemas. Mutating commands may eventually expose structured progress and final results, but that is a separate contract and is not implied by this first slice.

## Resources

- [Structured output tracking issue #184](https://github.com/microsoft/winget-cli/issues/184)
- [Initial list/upgrade implementation PR #6346](https://github.com/microsoft/winget-cli/pull/6346)
- [PowerShell installed package object](https://github.com/microsoft/winget-cli/blob/master/src/PowerShell/Microsoft.WinGet.Client.Engine/PSObjects/PSInstalledCatalogPackage.cs)
- [Reporter](https://github.com/microsoft/winget-cli/blob/master/src/AppInstallerCLICore/ExecutionReporter.h) and [TableOutput](https://github.com/microsoft/winget-cli/blob/master/src/AppInstallerCLICore/TableOutput.h)
