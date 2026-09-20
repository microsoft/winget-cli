---
author: Przemysław Kłys https://github.com/PrzemyslawKlys
created on: 2026-09-19
last updated: 2026-09-20
issue id: 184
---

# Structured CLI output

For [#184](https://github.com/microsoft/winget-cli/issues/184)

## Abstract

Add an opt-in JSON output mode to commands that return package data, starting with `list` and the non-mutating form of `upgrade`. Define the contract before shipping it, and keep serialization in the reporter so later commands do not each invent their own JSON path.

## Inspiration

The PowerShell module and COM API already provide structured package data, but callers of `winget.exe` in other shells and tools still have to parse localized, width-dependent tables. [#6346](https://github.com/microsoft/winget-cli/pull/6346) demonstrates the first use case, while its review identified two things to settle first: a stable shape aligned with the PowerShell objects, and a design that scales beyond two commands.

This proposal is for CLI consumers; it does not replace the PowerShell module or COM API. PowerShell remains the richer option when an application can use it directly.

## Solution Design

### Scope and rollout

Introduce `--format json` behind an experimental feature setting. The first supported operations are installed-package listing and available-upgrade listing. The default table output does not change. Until an operation has a defined schema and typed result, it must reject `--format json` rather than return an empty document that looks successful. Mutating upgrade modes remain unsupported in this first slice.

The schemas are reviewed with the first implementation and stored with the client. The experimental gate allows the contract to change before stabilization. Once stable, new optional fields increment the schema minor version; removing a field, changing its type, or changing its meaning requires a new major version. Consumers must ignore fields they do not understand within a supported major version.

### Proposed decisions

| Area | Proposal | Reason |
| --- | --- | --- |
| Common shape | A lower camel case envelope with a command-specific `result` object | Later commands can add their own result types without growing one universal object |
| Package fields | Keep PowerShell property names and types where the concepts overlap | Scripts see the same package vocabulary across CLI and PowerShell |
| Upgrade candidate | Add nullable `UpgradeVersion` alongside `AvailableVersions` | The full version list does not identify the version WinGet selected after applying command rules |
| Incomplete output | Include `result.truncated`, typed `warnings`, and typed `errors` | Automation can distinguish a complete empty result from an incomplete query |
| Schema identity | Versioned JSON Schema files with stable identifiers | Consumers can validate the exact contract they received |
| Partial failure | Return usable data, include the errors, and use a nonzero process exit code | Callers can use partial data without mistaking it for a complete success |

These are the proposed contract decisions for design review. They are not claims about what the implementation in #6346 currently emits.

### Common envelope

One invocation writes one UTF-8 JSON document without a byte order mark to stdout, followed by a newline. Every supported operation uses the same envelope:

```json
{
  "$schema": "https://aka.ms/winget-cli-output.list.1.0.schema.json",
  "schemaVersion": "1.0",
  "command": "list",
  "mode": "installed",
  "result": {
    "packages": [],
    "truncated": false
  },
  "warnings": [],
  "errors": []
}
```

`command` is the command entered by the caller. `mode` identifies the normalized read operation:

| Invocation | `command` | `mode` |
| --- | --- | --- |
| `winget list --format json` | `list` | `installed` |
| `winget list --upgrade-available --format json` | `list` | `availableUpgrades` |
| `winget upgrade --format json` | `upgrade` | `availableUpgrades` |

The envelope fields, `result`, `warnings`, and `errors` are always present. An empty successful query returns empty arrays and `result.truncated` set to `false`.

If JSON was selected but the command or mode is unsupported, there is no normalized read operation or command result to describe. That failure uses the versioned error-envelope schema with `mode: null` and `result: null`. Supported `list` and `upgrade` operations continue to use their command schema even when they fail before returning packages; their typed result remains present and contains an empty package array.

### Package result

The installed-package result uses the PowerShell object's property names and types where the concepts overlap. `UpgradeVersion` is CLI-specific because a list of all known versions does not show which version the command selected.

```json
{
  "$schema": "https://aka.ms/winget-cli-output.list.1.0.schema.json",
  "schemaVersion": "1.0",
  "command": "list",
  "mode": "installed",
  "result": {
    "packages": [
      {
        "Name": "Example App",
        "Id": "Example.App",
        "InstalledVersion": "1.0.0",
        "AvailableVersions": ["1.1.0", "1.0.0"],
        "IsUpdateAvailable": true,
        "UpgradeVersion": "1.1.0",
        "Source": "winget"
      }
    ],
    "truncated": false
  },
  "warnings": [],
  "errors": []
}
```

`Name`, `Id`, `InstalledVersion`, `AvailableVersions`, `IsUpdateAvailable`, and `Source` correspond to `PSInstalledCatalogPackage` properties. Their rules are:

- Version values are opaque strings. Consumers must not assume semantic version syntax.
- `AvailableVersions` contains all versions known from the matched source. It must not be created by splitting or interpreting a rendered table cell.
- `IsUpdateAvailable` means that the matched source contains a newer version than `InstalledVersion`. It does not by itself promise that policy, pins, or the current command mode allow an upgrade.
- `UpgradeVersion` is the version selected by the current command after its filters and pin behavior are applied. It is `null` when no actionable candidate is selected.
- `Source` is the actual source name when known, even when the human table hides its Source column. It is `null` when the source cannot be established.
- More than one row may have the same `Id` because WinGet can find multiple installed entries. Consumers must not use `Id` alone as an array key.
- The package order follows the command's existing default or requested sort behavior. Consumers should not treat position as package identity.

Returning all available versions may require catalog access beyond today's table. The implementation must measure that cost and must not reopen each package only to recreate display data. If the full list cannot be produced within an acceptable cost, the schema must be revised during the experimental phase rather than changing the meaning of `AvailableVersions`.

### Warnings, errors, and process status

Warnings describe a result that is usable but needs context. Warning codes are stable, non-localized identifiers; messages are localized diagnostic text. For example, the `warnings` array can contain:

```json
[
  {
    "code": "SearchTruncated",
    "message": "More results are available than were returned.",
    "source": null
  }
]
```

`result.truncated` is `true` whenever a source or result limit prevented the command from returning the complete matching set. The warning explains the condition to a person, while the boolean gives automation a direct completeness check. Other conditions that change or omit results, such as packages skipped because an installed version is unknown, must use typed warnings rather than text written around the JSON document.

Errors use the same document rather than replacing it with table output:

```json
{
  "$schema": "https://aka.ms/winget-cli-output.list.1.0.schema.json",
  "schemaVersion": "1.0",
  "command": "list",
  "mode": "installed",
  "result": {
    "packages": [],
    "truncated": false
  },
  "warnings": [],
  "errors": [
    {
      "code": "0x8A15000F",
      "message": "Source data is missing.",
      "source": "winget"
    }
  ]
}
```

The example code and message are illustrative, not a new error mapping. Error `code` contains the actual HRESULT as `0x` followed by eight uppercase hexadecimal digits. `message` is localized and must not be parsed. `source` is the source name when the error belongs to a source and `null` for command-level failures.

A partial source failure may return packages from healthy sources and error entries for failed sources. An empty `errors` array means that the command completed without an execution error. A non-empty `errors` array always produces a nonzero process exit code; for partial results, the first error is the primary error and supplies the process HRESULT. Warnings alone do not change the exit code.

Once argument parsing recognizes `--format json`, feature-gate, option-validation, source, and execution failures must use the JSON envelope. Failures before argument parsing can select JSON mode, such as process startup failure or an invalid command line that cannot be parsed, remain outside this guarantee. `--help` keeps normal help output because it describes the command rather than executing a package query. An unsupported value such as `--format xml` also uses the normal argument error because JSON mode was not selected.

### Reporter and table ownership

Select the output mode once in `Reporter`. Workflows pass typed results, warnings, and errors to it. The reporter serializes the command result to JSON or renders the existing human output. Plain decorative strings are not JSON data and are suppressed from JSON stdout. `TableOutput` can consume typed rows and keep presentation choices such as column hiding and truncation on the text side; it must not be the source of truth for serialized fields.

The reporter finalizes the JSON document once per invocation, including empty and error cases. Progress, settings warnings, source notices, and diagnostic logs must not precede or follow it on stdout. Diagnostics may use stderr, but stderr is not part of the JSON schema and callers must not parse it as structured output. Any warning needed by a caller belongs in the envelope.

Add a privacy-conscious telemetry event for use of JSON mode, with the command, normalized mode, schema major version, and success, warning, partial-failure, or failure category. Do not record package names, IDs, source contents, warning or error messages, or the JSON payload. Existing telemetry controls continue to apply.

### Schema files and compatibility

Store the output schemas under `schemas/JSON/cli-output/` and use JSON Schema draft 2020-12. Each supported command and schema version has a stable `$id`, exposed through the document's `$schema` property. The first files are the list 1.0 schema, upgrade 1.0 schema, and error-envelope 1.0 schema; shared envelope, package, warning, and error definitions may use referenced schema files in the same folder.

The schemas define every required field, nullable value, enumeration, and string format described here. Envelope, result, package, warning, and error objects allow additional properties so a consumer that supports the same major version can ignore later optional fields. A minor schema version may add optional fields or new warning codes. A major version is required to remove a field, make an optional field required, change a type, rename a value, or change existing semantics.

This schema is separate from package manifests and from `winget export`'s interchange format. It does not change manifest schema versions or validation in `winget-pkgs`, `winget-create`, or `winget-cli-restsource`.

### Settings and supported modes

The new `experimentalFeatures` key defaults to false. The final key name follows the repository's feature naming convention; the proposed settings shape is:

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
| On | `json` on a supported operation | JSON document |
| Off | `json` | JSON feature-gate error and nonzero exit code |
| On | `json` on an unsupported operation | JSON unsupported-operation error and nonzero exit code |

The settings flag permits the argument; it does not make JSON the default format. `--no-vt` changes only terminal decoration, not JSON data. `--disable-interactivity` and noninteractive hosts use the same contract. If a query would require a prompt, the command fails with a typed error rather than interleaving a prompt with JSON. `--silent` is installer behavior, not a listing modifier, so it is outside the first slice.

| Command or mode | First-slice behavior |
| --- | --- |
| `list` and filters that only narrow or sort the query | Supported as `mode: installed` |
| `list --upgrade-available` | Supported as `mode: availableUpgrades` |
| `upgrade` with no package argument | Supported as `mode: availableUpgrades` |
| `--count` on a supported query | Supported; `result.truncated` reports an incomplete set |
| `--include-unknown` on an available-upgrades query | Supported; the installed version remains an opaque version string |
| `list --details` | Unsupported until a details result schema exists |
| `--include-pinned` | Unsupported until the result models pin type and upgrade eligibility |
| `--silent` | Unsupported because it selects installer behavior rather than a read-only listing |
| Targeted upgrade or any option that can execute an upgrade | Unsupported in the first slice |
| `upgrade --all` | Unsupported in the first slice |
| Other commands | Unsupported until they have a typed result and schema |
| Empty result | Supported; empty arrays and a valid document |
| Source-open or source-search failure | Typed error and valid document after JSON mode is selected |

### Other surfaces and validation

No manifest fields, manifest JSON schemas, COM IDL interfaces, PowerShell cmdlets or output objects, WinGet Configuration resources, or group policies change in this proposal. No `winget-create`, `winget-cli-restsource`, or `winget-pkgs` validation-pipeline change is required for the first slice.

The first implementation PR must include the versioned CLI output schemas and focused tests for:

- schema validation for success, warning, partial-failure, and command-failure documents;
- single- and multi-source provenance, including `Source: null`;
- all version fields and selected upgrade behavior;
- duplicate package IDs and multiple installed entries;
- empty results and truncated results;
- skipped unknown installed versions and partial source failures;
- process exit codes for warnings, partial failures, and command failures;
- feature-disabled, unsupported-format, unsupported-command, and unsupported-mode behavior;
- `--help`, `--no-vt`, interactive, and noninteractive execution;
- existing text-table behavior.

The generic reporter path must be demonstrated by `list`, `list --upgrade-available`, and listing-only `upgrade` without duplicating serialization branches in their workflows.

## UI/UX Design

With the feature enabled, `winget list --format json` prints the installed-package document shown above. `winget list --upgrade-available --format json` and `winget upgrade --format json` return the available-upgrades result with the command and mode values defined above. Without `--format json`, users see the current localized table.

An unsupported command or mutating mode reports a JSON error and a nonzero exit code after JSON mode has been selected. It must not perform an operation, return an empty success document, or present table text as JSON.

## Capabilities

### Accessibility

The existing text presentation remains the default, including its accessibility behavior. JSON is intended for tools and is not a replacement for readable command output.

### Security

JSON values must be escaped by a serializer, never assembled by string concatenation. The format option does not widen source access or bypass policy and agreement checks. Telemetry excludes package, source, warning, error-message, and payload data.

### Reliability

One machine-readable document, explicit empty and truncated results, typed warnings and failures, and a defined exit-code policy give callers a reliable contract. Source provenance comes from the data model, not table-column visibility.

### Compatibility

No existing default output or PowerShell/COM contract changes. The option remains experimental while the schemas are under review. After stabilization, consumers use the schema major version as the compatibility boundary and ignore unknown optional fields.

### Performance, Power, and Efficiency

The JSON path reuses the typed package data collected by the workflow. It must not render a table and parse it back, and it should not open every package again only to populate display-equivalent fields. The first implementation measures the cost of collecting all available versions before the 1.0 schema is stabilized.

## Potential Issues

- The PowerShell object model and CLI query semantics are related but not identical. CLI-only fields must be named explicitly rather than changing the meaning of a PowerShell field.
- Returning every available version may add source work compared with the current table. The experimental contract may need revision if measurement shows unacceptable cost.
- Partial results are deliberately nonzero. Consumers that accept them must parse the document even when the process reports failure.
- Some failures occur before JSON mode can be selected. The guarantee begins when argument parsing recognizes `--format json`.
- A generic reporter change could alter text output accidentally. Keep table regression tests alongside JSON tests.

## Deprecation Path

No existing feature is deprecated. The human table stays the default, and this proposal does not replace the PowerShell module, COM API, or `winget export` format.

## Future Considerations

After the first schemas and reporter path are agreed, `search`, `show`, and other result-producing commands can add their own typed results. Pin-aware output needs a stable model for pin type and upgrade eligibility before `--include-pinned` is supported. Mutating commands may eventually expose structured progress and final results, but that is a separate contract and is not implied by this first slice.

## Resources

- [Structured output tracking issue #184](https://github.com/microsoft/winget-cli/issues/184)
- [Initial list/upgrade implementation PR #6346](https://github.com/microsoft/winget-cli/pull/6346)
- [PowerShell installed package object](https://github.com/microsoft/winget-cli/blob/master/src/PowerShell/Microsoft.WinGet.Client.Engine/PSObjects/PSInstalledCatalogPackage.cs)
- [Reporter](https://github.com/microsoft/winget-cli/blob/master/src/AppInstallerCLICore/ExecutionReporter.h) and [TableOutput](https://github.com/microsoft/winget-cli/blob/master/src/AppInstallerCLICore/TableOutput.h)
