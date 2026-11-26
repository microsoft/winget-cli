---
author: Kaleb Luedtke @Trenly, GitHub Copilot <Copilot>
created on: 2026-09-21
last updated: 2026-09-21
issue id: 6545
---

# Add manifest validation COM API

For [#6545](https://github.com/microsoft/winget-cli/issues/6545)

## Abstract

This specification adds a Windows Package Manager COM API for validating a manifest file or a
directory containing a multi-file manifest. The API uses the same manifest parser and validation
rules as `winget validate`, while returning structured diagnostics that applications can inspect
without parsing command-line output.

The API is introduced in a new version of the
`Microsoft.Management.Deployment.WindowsPackageManagerContract`. It also exposes full, partial, and
schema-only validation modes so callers can select the level of validation appropriate for their
scenario.

## Inspiration

Applications that author, edit, or submit WinGet manifests need to validate those manifests before
using them. The `winget validate` command provides this functionality to command-line users, but COM
API callers do not have an equivalent operation. They must currently launch a separate process,
depend on a different interop layer, or duplicate validation behavior.

A COM API allows an application to validate manifests in process with its existing
`Microsoft.Management.Deployment` integration. Structured diagnostics also let the application
associate issues with fields and source locations, present errors in its own user interface, and
react to warnings without parsing localized console text.

## Solution Design

### Scope

The API validates:

- A path to one YAML manifest file.
- A path to a directory containing the files for one multi-file manifest.
- The manifest structure and values selected by the requested validation mode.

The API does not:

- Return the parsed manifest.
- Return or display dependency information.
- Download or inspect installer payloads.
- Add or change a `winget validate` command-line option.
- Change manifest schemas, settings, group policy, or WinGet Configuration behavior.

The `winget validate` command reports dependencies after successful parsing. Dependency reporting is
not part of the manifest validation result and is therefore not included in this API.

### API contract

The following API is added in a new version of
`Microsoft.Management.Deployment.WindowsPackageManagerContract`.

```idl
namespace Microsoft.Management.Deployment
{
    /// Specifies the set of manifest validation rules to apply.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, <new contract version>)]
    enum ManifestValidationMode
    {
        /// Applies schema and semantic validation equivalent to winget validate.
        Full,

        /// Parses and populates the manifest without schema or full semantic validation.
        Partial,

        /// Validates manifest input and schema without populating the manifest or applying semantic validation.
        SchemaOnly,
    };

    /// Describes the outcome of manifest validation.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, <new contract version>)]
    enum ManifestValidationResultStatus
    {
        /// The manifest passed according to the requested options.
        Success,

        /// The manifest produced only warnings and warnings are configured to fail validation.
        Warning,

        /// The manifest produced one or more errors.
        Error,
    };

    /// Specifies the severity of one manifest validation issue.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, <new contract version>)]
    enum ManifestValidationIssueSeverity
    {
        Warning,
        Error,
    };

    /// Options for manifest validation.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, <new contract version>)]
    runtimeclass ValidationOptions
    {
        ValidationOptions();

        /// Gets or sets the validation mode. The default is Full.
        ManifestValidationMode Mode;

        /// Gets or sets whether warning-only diagnostics cause a Warning result.
        /// The default is true.
        Boolean TreatWarningsAsErrors;

        /// Gets or sets whether fields reserved for verified publishers are errors instead of warnings.
        /// The default is false.
        Boolean ErrorOnVerifiedPublisherFields;

        /// Gets or sets whether schema header issues are warnings instead of errors during Full validation.
        /// The default is true.
        Boolean SchemaHeaderValidationAsWarning;
    };

    /// A single manifest validation error or warning.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, <new contract version>)]
    runtimeclass ManifestValidationIssue
    {
        /// A stable, non-localized identifier for the issue.
        String Code { get; };

        /// A human-readable, localized description of the issue.
        String Message { get; };

        /// The severity of the issue.
        ManifestValidationIssueSeverity Severity { get; };

        /// The manifest field or context associated with the issue, if available.
        String Context { get; };

        /// The value associated with the issue, if available.
        String Value { get; };

        /// The source file name associated with the issue, if available.
        String FileName { get; };

        /// The 1-based source line, or 0 when it is not available.
        UInt64 Line { get; };

        /// The 1-based source column, or 0 when it is not available.
        UInt64 Column { get; };
    };

    /// The result of manifest validation.
    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, <new contract version>)]
    runtimeclass ManifestValidationResult
    {
        /// The validation outcome.
        ManifestValidationResultStatus Status { get; };

        /// The HRESULT corresponding to the validation outcome.
        HRESULT ExtendedErrorCode { get; };

        /// The errors and warnings produced by validation.
        Windows.Foundation.Collections.IVectorView<ManifestValidationIssue> Issues { get; };
    };

    [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, 1)]
    runtimeclass PackageManager
    {
        [contract(Microsoft.Management.Deployment.WindowsPackageManagerContract, <new contract version>)]
        {
            /// Validates a manifest file or a directory containing a multi-file manifest.
            Windows.Foundation.IAsyncOperation<ManifestValidationResult>
                ValidateManifestAsync(String manifestPath, ValidationOptions options);
        }
    };
}
```

The IDL `declare` block will include `IVector<ManifestValidationIssue>` and
`IVectorView<ManifestValidationIssue>` so the issue collection can be marshalled.

`ValidationOptions` is activatable for both in-process and out-of-process COM callers. The result and
issue classes are created by Windows Package Manager and are not independently activatable.

### Manifest path

`manifestPath` is separate from `ValidationOptions` because it is the subject of the operation rather
than a validation policy.

If the path identifies a file, the API validates that file as a singleton manifest. If it identifies
a directory, the API loads the files immediately within that directory and validates them as a
multi-file manifest. The directory is not traversed recursively, and a child directory causes the
operation to fail.

An empty path or null `ValidationOptions` is invalid. Path-not-found, access-denied, and other
filesystem failures fail the asynchronous operation with the applicable HRESULT. These failures are
not returned as manifest validation issues because validation could not be performed.

### Validation modes

The modes map to existing native manifest parser behavior.

| Validation stage | Full | Partial | SchemaOnly |
|---|---:|---:|---:|
| Validate manifest input and multi-file consistency | Yes | Yes | Yes |
| Validate against the JSON schema | Yes | No | Yes |
| Populate the native manifest model | Yes | Yes | No |
| Apply parser and field-processing diagnostics | Yes | Yes | No |
| Apply full semantic validation | Yes | No | No |
| Validate the YAML schema header | Yes | No | No |

`Full` is the default and matches the validation level used by `winget validate`. It is the
appropriate mode when deciding whether a manifest is ready for submission.

`Partial` performs the validation needed while reading and populating a manifest, but it intentionally
omits schema and full semantic validation. A successful partial result does not mean that the manifest
would pass `winget validate`.

`SchemaOnly` validates input integrity and JSON schema conformance. It returns before populating the
manifest model and before semantic or schema-header validation.

### Validation options

| Property | Default | Behavior |
|---|---:|---|
| `Mode` | `Full` | Selects the validation stages described above. |
| `TreatWarningsAsErrors` | `true` | Controls whether warning-only diagnostics produce a `Warning` result and warning HRESULT. It does not change issue severity. |
| `ErrorOnVerifiedPublisherFields` | `false` | Reports use of verified-publisher-only fields as errors instead of warnings. It applies when the manifest model is populated in `Full` and `Partial` modes and has no effect in `SchemaOnly` mode. |
| `SchemaHeaderValidationAsWarning` | `true` | Reports schema-header issues as warnings instead of errors. Schema headers are evaluated only in `Full` mode. |

The defaults reproduce the relevant `winget validate` behavior: full validation, schema-header issues
reported as warnings, and warning-only validation treated as unsuccessful unless the caller opts out.

### Installer payload validation

The native parser has an option named `InstallerValidation`, but it is not exposed by this API. That
option is not general installer or manifest validation. It only applies to MSIX and MSIX bundle
installers and may read or download each unique installer payload with up to three download attempts.
It compares package metadata with manifest values including `SignatureSha256`, `PackageFamilyName`,
package version, and `MinimumOSVersion`.

Exposing that behavior on `ValidationOptions` would make a manifest-validation operation unexpectedly
perform network I/O and potentially download large files, while doing nothing for EXE, MSI, portable,
and other installer types. A future API options may expose MSIX payload validation with a name and operation
contract that make its network, progress, cancellation, and cleanup behavior explicit.

### Result behavior

The result contains every structured issue produced by the native validator. Issues retain their
original severity even when `TreatWarningsAsErrors` changes the overall result.

| Diagnostics | `TreatWarningsAsErrors` | Status | Extended error code |
|---|---:|---|---|
| None | Either | `Success` | `S_OK` |
| Warnings only | `false` | `Success` | `S_OK` |
| Warnings only | `true` | `Warning` | `APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_WARNING` |
| One or more errors, with or without warnings | Either | `Error` | `APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_FAILURE` |

`Code` is a stable, non-localized string identifier derived from the native manifest error identifier,
such as `RequiredFieldMissing` or `InvalidFieldValue`. A string is used instead of a WinRT enum so a
new native diagnostic does not require a new COM contract before callers can receive it. Callers
should use `Code` for programmatic decisions and treat unknown codes as valid future values.

`Message` is localized and intended for display. Callers must not parse it or use it as a stable
identifier.

Some parser failures, such as malformed YAML, do not produce a collection of native structured
validation errors. The API returns one error issue with the code `ManifestParseError`, the available
parser message, empty context/value/file fields when unavailable, and line or column set to zero when
the parser does not provide them.

Invalid arguments and failures that prevent validation from running fail the asynchronous operation
with their actual HRESULT. They do not return a `ManifestValidationResult` that resembles a completed
validation.

### Processing flow

```text
ValidateManifestAsync(path, options)
            |
            +-- Validate arguments and path access
            |
            +-- Map ValidationOptions to native ManifestValidateOption
            |
            +-- Parse file or multi-file directory
            |
            +-- Apply selected validation mode
            |
            +-- Convert native diagnostics to ManifestValidationIssue
            |
            +-- Apply warning policy to result status and HRESULT
            |
            +-- Return ManifestValidationResult
```

The implementation captures native `ValidationError` objects directly. It does not parse the
human-readable validation message or serialize and parse the existing JSON diagnostic format.

The operation is asynchronous because reading and validating files should not block the caller's
thread. It does not report progress because manifest parsing and local validation do not have stable,
meaningful progress stages.

### Other product surfaces

#### Command-line interface

There is no CLI behavior change. `winget validate` continues to use full validation and its existing
warning behavior and dependency reporting.

#### PowerShell

No PowerShell cmdlet is added or changed by this specification. PowerShell callers may consume the COM
API through the projected `Microsoft.Management.Deployment` types.

#### Manifest schema and validation pipeline

No manifest schema changes are required. The winget-pkgs validation pipeline is not changed because
the API reuses existing parser and validation behavior.

#### Settings, policy, and configuration

No settings, group policy, or WinGet Configuration changes are required.

### Validation

Automated tests will cover:

- Valid singleton and multi-file directory manifests.
- Empty, missing, inaccessible, and otherwise invalid paths.
- Null options.
- Behavioral differences among `Full`, `Partial`, and `SchemaOnly`.
- Warning-only diagnostics with `TreatWarningsAsErrors` enabled and disabled.
- Schema-header warning and error behavior.
- Verified-publisher field warning and error behavior.
- Malformed YAML without native structured diagnostics.
- Multiple diagnostics with code, severity, context, value, file, line, and column data.
- `ValidationOptions` activation through supported in-process and out-of-process activation paths.

Tests will assert stable codes, severity, status, HRESULT, and source metadata. They will not rely only
on localized message text.

## UI/UX Design

The COM API does not display UI or write to the console. The caller decides how to present the
structured result.

The following C# example performs full validation and displays each issue:

```csharp
var packageManager = new PackageManager();
var options = new ValidationOptions
{
    Mode = ManifestValidationMode.Full,
    TreatWarningsAsErrors = true,
    ErrorOnVerifiedPublisherFields = false,
    SchemaHeaderValidationAsWarning = true,
};

ManifestValidationResult result =
    await packageManager.ValidateManifestAsync(manifestPath, options);

foreach (ManifestValidationIssue issue in result.Issues)
{
    Console.WriteLine(
        $"{issue.Severity} {issue.Code}: {issue.Message} " +
        $"({issue.FileName}:{issue.Line}:{issue.Column})");
}

if (result.Status != ManifestValidationResultStatus.Success)
{
    Console.Error.WriteLine(
        $"Manifest validation did not succeed: 0x{result.ExtendedErrorCode:X8}");
}
```

A caller that wants warnings for display without making validation unsuccessful sets
`TreatWarningsAsErrors` to `false`. The warning issues remain in `Issues`, while `Status` is
`Success` and `ExtendedErrorCode` is `S_OK`.

## Capabilities

### Accessibility

The API has no built-in visual interface. Structured issue severity, code, message, and source
location let callers build accessible experiences without extracting meaning from color, formatting,
or console control sequences. Human-readable messages remain available for screen readers and other
assistive technology.

### Security

The operation reads only the file or directory selected by the caller and runs under the caller's
security context. Existing filesystem access checks remain in effect.

Validation does not execute a manifest, install a package, contact a package source, or inspect
installer payloads. In particular, the MSIX-only native installer payload validation option is not
exposed, preventing an ordinary manifest-validation call from causing unexpected downloads.

Callers should treat manifest values and diagnostic text as untrusted data when displaying or logging
them.

### Reliability

The API reuses the same native parser and validation rules as the CLI instead of implementing a second
validator. This keeps validation results consistent across entry points.

Structured native diagnostics are copied into WinRT result objects, so their lifetime does not depend
on a native exception or parser object. Failures that prevent validation are surfaced as failed async
operations rather than success-shaped results.

Because files can change while validation is running, the result applies to the content read during
that operation. Callers that require stronger coordination are responsible for preventing concurrent
changes.

### Compatibility

The API is an additive change in a new contract version. Existing COM API and CLI callers are
unaffected. The default options match `winget validate` where the surfaces overlap.

Diagnostic codes are strings so older clients can receive issues introduced by newer Windows Package
Manager versions. Callers must tolerate unknown codes and enum values according to normal WinRT
versioning practices.

`Partial` and `SchemaOnly` are explicitly weaker than full CLI validation. Their names and
documentation distinguish successful parsing or schema conformance from submission readiness.

### Performance, Power, and Efficiency

Validation performs local file I/O and CPU-bound YAML, schema, and semantic validation. The async
operation prevents that work from blocking the caller's thread.

No installer payload is downloaded. A directory input reads only its immediate files and is not
recursively traversed. The complete issue collection is returned so callers do not need to repeat
validation to discover additional errors.

## Potential Issues

- Callers may incorrectly interpret `Partial` or `SchemaOnly` success as equivalent to
  `winget validate` success. Documentation and enum descriptions must clearly identify `Full` as the
  submission-readiness mode.
- `TreatWarningsAsErrors` changes the overall result but does not promote individual warning severity.
  Callers must use `Status` to decide whether the requested validation policy passed.
- Localized messages can change between releases. Callers must use `Code`, not `Message`, for
  programmatic behavior.
- New diagnostic codes can appear without a new COM contract. Callers must tolerate unknown strings.
- Native parsing is synchronous internally, so cancellation can be observed only at safe async
  boundaries rather than during every parser step.
- Very large manifests or directories containing many files may take longer and consume more memory
  than typical manifests.
- `ValidationOptions` requires new production, development, and in-process activation identifiers and
  registration entries. Missing registration on any deployment path would make options activation
  fail even if the method exists in metadata.

## Future considerations

- Add a separately named MSIX payload-validation API with explicit download, progress, cancellation,
  authentication, proxy, and cleanup behavior.
- Add an API that accepts manifest content or a stream for editors that have not saved a file.
- Add an API that returns a parsed, immutable manifest model in addition to validation diagnostics.
- Add PowerShell cmdlets that wrap the COM API if a first-class PowerShell authoring scenario is
  needed.
- Add filtering or categorization if the number and types of diagnostics expand substantially.

## Resources

- [`winget validate` command documentation](https://learn.microsoft.com/windows/package-manager/winget/validate)
- [Windows Package Manager manifest documentation](https://learn.microsoft.com/windows/package-manager/package/manifest)
- [`ValidateCommand.cpp`](../../src/AppInstallerCLICore/Commands/ValidateCommand.cpp)
- [`ManifestCommon.h`](../../src/AppInstallerCommonCore/Public/winget/ManifestCommon.h)
- [`ManifestValidation.h`](../../src/AppInstallerCommonCore/Public/winget/ManifestValidation.h)
- [`YamlParser.cpp`](../../src/AppInstallerCommonCore/Manifest/YamlParser.cpp)
- [`PackageManager.idl`](../../src/Microsoft.Management.Deployment/PackageManager.idl)
