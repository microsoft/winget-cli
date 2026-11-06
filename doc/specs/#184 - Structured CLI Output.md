---
author: Przemysław Kłys https://github.com/PrzemyslawKlys, OpenAI Codex
created on: 2026-09-19
last updated: 2026-10-07
issue id: 184
---

# Structured CLI output

For [#184](https://github.com/microsoft/winget-cli/issues/184)

## Abstract

Add an opt-in JSON output mode to commands that return package data, starting with `list` and the non-mutating form of `upgrade`. Define a shared invocation envelope and preserve the package relationships available to the client. The reporter owns serialization so later commands can use the same mechanism.

## Inspiration

The COM API and PowerShell module already provide structured package data. Callers of `winget.exe` in other shells and tools still have to parse localized, width-dependent tables. [#6346](https://github.com/microsoft/winget-cli/pull/6346) demonstrates that use case, but its table-derived model loses source information and does not scale to other commands.

This proposal adds another public contract to maintain. COM remains the preferred API for applications that can use it directly, and PowerShell remains available to scripts that can take that dependency. The CLI use case is automation that invokes the executable without introducing either runtime. The specification needs maintainer agreement before the implementation in [#6536](https://github.com/microsoft/winget-cli/pull/6536) is updated or stabilized.

## Solution Design

### Scope and rollout

Introduce `--format json` behind the `structuredOutput` experimental setting. Installed-package listing and available-upgrade listing are the first supported operations. Mutating operations and commands without a result schema return a structured error once JSON mode has been selected. Existing table output remains the default.

The experimental gate permits revisions while the contract is reviewed. Before stabilization, the implementation must demonstrate the shared reporter path across `list`, `list --upgrade-available`, and listing-only `upgrade`.

### Common envelope

Every invocation in JSON mode writes one compact UTF-8 JSON document without a byte order mark to stdout, followed by a newline. Examples below use indentation for readability.

```json
{
  "$schema": "https://aka.ms/winget-cli-output.list.1.31.schema.json",
  "schemaVersion": "1.31",
  "command": "list",
  "arguments": {"format": ["json"]},
  "result": {"packages": [], "truncated": false},
  "warnings": [],
  "error": null
}
```

The common envelope schema defines and requires `$schema`, `schemaVersion`, `command`, `arguments`, `result`, `warnings`, and `error`. Each command schema references that definition and supplies its result type. A generic error-envelope schema uses the same required fields with `result: null` for operations without a supported result type.

`command` is the resolved command name, including parent command names when applicable. `arguments` records accepted arguments under their canonical long names without leading dashes. Values are arrays of strings; flags are empty arrays. Positional arguments use their parser-defined names. Aliases are normalized, repeated values retain their order, and implicit defaults are omitted. Sensitive arguments must be redacted rather than echoed. This object is output data and must not be copied into telemetry.

For example, `list --upgrade-available --format json` has command `list` and arguments `{"upgrade-available":[],"format":["json"]}`. Listing-only `upgrade --format json` has command `upgrade` and arguments `{"format":["json"]}`. No separate `mode` field is needed.

An empty successful query has empty package and warning arrays, `truncated: false`, and `error: null`. A supported command that fails before producing data retains its typed empty result. An unsupported operation uses the generic envelope with `result: null`.

### Package result

Serialize the graph represented by `ICompositePackage`, `IPackage`, and `PackageVersionKey`, rather than rendered table rows. An installed result has its installed package and zero or more available packages, each with its own source and version collection. Preserve version channels and source identity; do not merge versions from different sources into one string array.

```json
{
  "packages": [
    {
      "Name": "Example App",
      "Id": "Example.App",
      "InstalledPackage": {
        "Name": "Example App",
        "Id": "Example.App",
        "Versions": [{"Version": "1.0.0", "Channel": ""}]
      },
      "InstalledSource": {
        "Name": "winget",
        "Identifier": "Microsoft.Winget.Source_8wekyb3d8bbwe"
      },
      "AvailablePackages": [
        {
          "Name": "Example App",
          "Id": "Example.App",
          "Source": {
            "Name": "winget",
            "Identifier": "Microsoft.Winget.Source_8wekyb3d8bbwe"
          },
          "Versions": [
            {"Version": "1.1.0", "Channel": ""},
            {"Version": "1.0.0", "Channel": ""}
          ]
        }
      ],
      "UpgradeVersion": {"Version": "1.1.0", "Channel": ""},
      "UpgradeSource": {
        "Name": "winget",
        "Identifier": "Microsoft.Winget.Source_8wekyb3d8bbwe"
      }
    }
  ],
  "truncated": false
}
```

- `InstalledPackage` comes from `GetInstalled()`. Its identity may differ from the correlated result, for example an installed product code matched to a catalog package ID.
- `InstalledSource` identifies the source the package was last installed from, when known. An installed-packages inventory source is not evidence of the original catalog. Return `null` when that provenance is unavailable.
- `AvailablePackages` comes from `GetAvailable()`, retaining each package, its source, and its `GetVersionKeys()` collection. Each version keeps its opaque version string and channel. Consumers must not assume semantic version syntax.
- `UpgradeVersion` is the candidate selected after command filters and pin behavior. `UpgradeSource` identifies its source. Both are `null` when no actionable candidate is selected. They may differ from the last installed version and `InstalledSource`.
- Source objects contain the configured name and stable identifier when known; unavailable values are `null`. Sources remain visible even when a table hides a repeated Source column. They exclude credentials and authentication headers.
- Multiple installed entries can share an `Id`, and available packages can represent that ID in different sources. Neither IDs nor array positions are unique package identities.
- Package order follows the command's existing sort behavior. Version collections retain the model's descending order, with each channel kept alongside its version.

This projects package relationships and version keys without promising every internal property or downloading every manifest. A future details result can add metadata through the same graph. PowerShell property names are retained where their meaning matches, but its current flattened shape does not limit the CLI contract.

`result.truncated` reports truncation in the source search, for example a source-reported result limit. Terminal width and hidden table columns do not affect serialized data. Ordinary table presentation therefore leaves this value `false`; a warning accompanies actual search truncation.

### Warnings, invocation error, and process status

`warnings` is an array of contextual diagnostics. Reuse stable resource string identifiers for existing diagnostics where possible. Messages remain localized and must not be parsed. A diagnostic may include source context and an HRESULT without becoming the invocation's primary error:

```json
{
  "code": "SourceOpenFailed",
  "message": "Source data is missing.",
  "source": "winget",
  "hresult": -1978335217
}
```

The example is illustrative; use the existing diagnostic identifier and actual HRESULT. The warning schema accepts a string `code`, rather than a closed enumeration requiring edits for every new diagnostic. A published code's meaning remains stable. `source` and `hresult` are nullable.

`error` is either `null` or one object describing the invocation failure. It contains the actual HRESULT as a signed 32-bit JSON integer and its localized message, without per-source context:

```json
{"hresult": -1978335217, "message": "Source data is missing."}
```

There is one invocation HRESULT. Its 32-bit value agrees with the process exit code; hosts displaying unsigned exit codes may show a different decimal representation of the same bits. Success uses `error: null` and exit code zero.

A partial source failure can retain packages from healthy sources and contextual diagnostics for failed sources. The invocation has a non-null `error` and a nonzero exit code indicating incomplete execution. The command's existing error selection supplies that HRESULT; array order does not select it. Ordinary warnings, such as a stale source that remains usable, retain their existing command status semantics.

Once parsing recognizes `--format json`, feature-gate, unsupported-command, option-validation, source, and execution failures use the common envelope. Unsupported operations must not execute, emit table text, or return an empty success document. Startup failures and command lines that cannot be parsed far enough to select JSON remain outside this guarantee. `--help` keeps normal help output. An unsupported format such as `--format xml` keeps the normal argument error because JSON was not selected.

### Reporter and table ownership

Select JSON mode once in `Reporter`. Workflows supply typed package results and diagnostics. The reporter owns the final document, shared envelope, and invocation error. Plain decorative strings are suppressed on JSON stdout. `TableOutput` keeps hidden columns, width truncation, and decoration on the text path.

The reporter finalizes once per invocation, including empty and failure cases. Progress, settings warnings, source notices, and logs must not surround the document on stdout. Diagnostics can still use stderr, but automation obtains structured diagnostics from the envelope.

Extend existing command telemetry and its summary with the selected output format and schema version. Do not add a separate event. Existing command and completion information provides invocation context. Do not record package data, source contents, supplied argument values, messages, or the JSON document. Existing telemetry controls continue to apply.

### Schema files and compatibility

Store JSON Schema draft 2020-12 definitions under `schemas/JSON/cli-output/`. Define the envelope once and reference it from list, upgrade, and generic error-envelope schemas. Share package, source, version-key, diagnostic, and invocation-error definitions across commands.

Begin at the client version shipping the contract, proposed here as `1.31`, rather than introducing an unrelated `1.0` sequence. Select the final version with the implementation's shipping client. Each version has stable identifiers exposed through `$schema` and `schemaVersion`. Example URLs require registration before stabilization.

Schemas specify required fields and types. Objects permit additional properties, and consumers ignore optional fields they do not recognize. Compatible releases can add optional fields and diagnostic codes. Stable 1.X releases do not remove or rename fields, change their types, or change their meaning. An incompatible replacement needs a separately agreed contract and migration plan; it cannot silently replace an existing schema identifier.

This applies to CLI output. It does not change manifest schemas, `winget export`, or the schema acceptance policy in `winget-pkgs`.

### Settings and supported operations

The `structuredOutput` experimental setting defaults to false:

```json
{"experimentalFeatures": {"structuredOutput": true}}
```

| Setting | `--format` | Result |
| --- | --- | --- |
| Off | Omitted | Existing text output |
| On | Omitted | Existing text output |
| On | `json` on a supported operation | JSON result document |
| Off | `json` | JSON feature-gate error and nonzero exit code |
| On | `json` on an unsupported operation | JSON error envelope and nonzero exit code |

The setting permits the argument without changing the default format. JSON is compact; a separate pretty-print option is outside this first slice. `--no-vt` does not change JSON data. Interactive and `--disable-interactivity` callers receive the same contract. A required prompt produces a structured error rather than interleaving prompts with JSON. Agreement acceptance flags remain available where the command supports them.

| Command or condition | First-slice behavior |
| --- | --- |
| `list` and supported query or sort filters | Installed-package graph |
| `list --upgrade-available` | Package graph for the available-upgrade query |
| `upgrade` with no package argument | Package graph for the available-upgrade query |
| `--count` on a supported query | Report search truncation when the source indicates it |
| `--include-unknown` on an available-upgrade query | Preserve the installed version's opaque value |
| `list --details` | Structured error until a details schema exists |
| `--include-pinned` | Structured error until pin type and upgrade eligibility are modeled |
| `--silent`, targeted upgrade, or any mutating upgrade option | Structured error; no operation |
| Other commands | Structured error until a typed result and schema exist |
| Empty result | Typed empty result, valid envelope, normal command status |
| Source-open or source-search failure | Valid envelope, contextual diagnostics, invocation error |

### Other surfaces and validation

No manifest fields, COM IDL interfaces, PowerShell cmdlets or objects, Configuration resources, or group policies change. No changes are needed in `winget-create`, `winget-cli-restsource`, or the `winget-pkgs` validation pipeline. COM and PowerShell retain their existing interactivity and data contracts.

The implementation includes schema validation and focused behavior tests for:

- the shared envelope across success, warnings, partial failure, and invocation failure;
- differing installed and upgrade provenance, unknown provenance, and single-source column hiding;
- multiple available packages with distinct sources and version channels;
- duplicate installed IDs and existing command sorting;
- empty results, source search truncation, and table width independence;
- actual invocation HRESULTs, contextual source diagnostics, and process exit codes;
- canonical arguments, omitted defaults, and sensitive-value redaction;
- feature-disabled, unsupported-command, unsupported-format, and unsupported-operation behavior;
- `--help`, `--no-vt`, interactive, and noninteractive execution;
- unchanged existing text output.

Demonstrate the shared reporter path across all three supported invocations without duplicating serialization in workflows. Collecting version keys must not download every manifest.

## UI/UX Design

With the feature enabled, an empty successful list writes this single line:

```json
{"$schema":"https://aka.ms/winget-cli-output.list.1.31.schema.json","schemaVersion":"1.31","command":"list","arguments":{"format":["json"]},"result":{"packages":[],"truncated":false},"warnings":[],"error":null}
```

Available-upgrade listings use their parsed command and arguments in the same envelope. Unsupported operations return a JSON error and a nonzero exit code without performing the operation. Omitting `--format json` retains the localized table.

## Capabilities

### Accessibility

Readable text output and its accessibility behavior remain the default. Compact JSON is intended for tools; callers can format it with their existing JSON tools when reading it directly.

### Security

Use a JSON serializer to escape values. Source access, trust, policy, and agreement checks still apply. Redact sensitive arguments and omit credentials from source objects. Telemetry excludes argument values and output data.

### Reliability

One document describes the invocation, result, contextual diagnostics, and final failure. Source and version relationships come from the package model, so presentation choices cannot discard them.

### Compatibility

The option remains experimental until the schemas are agreed. Existing default output, COM, and PowerShell contracts do not change. Stable output schemas retain their defined meanings and accept additive optional fields.

### Performance, Power, and Efficiency

Reuse the package graph and version keys available to the command. Do not render and parse a table or download every manifest. Measure the extra cost of enumerating available versions before stabilization.

## Potential Issues

- A CLI output format adds a public contract alongside COM and PowerShell. Maintainers must agree that its executable-only use case justifies that ongoing cost.
- Package relationships are richer than the current PowerShell projection. Naming alignment must not flatten them or change existing property meanings.
- Enumerating available version keys may add source work. Measure it in the experimental implementation.
- Consumers using partial results must parse the document even when the process exits nonzero.
- Failures before parsing can select JSON remain unstructured.
- Reporter changes can affect text output; retain existing behavior tests.

## Deprecation Path

No feature is deprecated. Tables remain the default, and COM, PowerShell, and `winget export` keep their existing contracts.

## Future Considerations

After the envelope and package model are agreed, other query commands can define result schemas. A details result, pin-aware query data, pretty printing, and structured mutating operations need their own reviewed contracts. They are outside this first slice.

## Resources

- [Tracking issue #184](https://github.com/microsoft/winget-cli/issues/184)
- [Initial implementation #6346](https://github.com/microsoft/winget-cli/pull/6346) and [experimental implementation #6536](https://github.com/microsoft/winget-cli/pull/6536)
- [Package graph and version interfaces](https://github.com/microsoft/winget-cli/blob/master/src/AppInstallerRepositoryCore/Public/winget/RepositorySearch.h)
- [PowerShell installed package object](https://github.com/microsoft/winget-cli/blob/master/src/PowerShell/Microsoft.WinGet.Client.Engine/PSObjects/PSInstalledCatalogPackage.cs)
- [Reporter](https://github.com/microsoft/winget-cli/blob/master/src/AppInstallerCLICore/ExecutionReporter.h) and [TableOutput](https://github.com/microsoft/winget-cli/blob/master/src/AppInstallerCLICore/TableOutput.h)
- [Existing command telemetry](https://github.com/microsoft/winget-cli/blob/master/src/AppInstallerCommonCore/AppInstallerTelemetry.cpp)
