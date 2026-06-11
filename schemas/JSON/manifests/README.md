# WinGet Manifest Schemas

This directory contains JSON schemas for WinGet package manifests.

## Directory Structure

| Directory | Contents |
|-----------|----------|
| `latest/` | The in-development schema. File names use `latest` as the version token. The `$id` field in each file contains the **current version string** (e.g., `1.28.0`). |
| `v{X.Y.0}/` | Frozen snapshots. File names use the numeric version. Content is identical to `latest/` at the moment of branching. |
| `preview/` | Legacy v0.1.0 preview schema. |

Each version directory contains five schema files:

| File (using `latest/` as an example) | Manifest type |
|---------------------------------------|---------------|
| `manifest.singleton.latest.json` | Single-file manifest |
| `manifest.version.latest.json` | Multi-file: version manifest |
| `manifest.installer.latest.json` | Multi-file: installer manifest |
| `manifest.defaultLocale.latest.json` | Multi-file: default locale manifest |
| `manifest.locale.latest.json` | Multi-file: locale manifest |

---

## Branching `latest/` to a Frozen Version

When the current `latest/` schemas are ready to be frozen (typically as a WinGet release
approaches), branch them into a numbered version directory. The version to use is embedded in
the `$id` field of each `latest/` file—for example:

```
"$id": "https://aka.ms/winget-manifest.installer.1.28.0.schema.json"
                                                   ^^^^^^
                                                   This is the version.
```

> **Helper script**: `Branch-LatestManifestSchema.ps1` in this directory automates
> the schema-file and project-file steps (1–4 below). The C++ source and test steps
> still require manual edits.

### 1. Create the versioned schema directory

Create `schemas/JSON/manifests/v{VERSION}/` and copy each `latest/` file with the version
token substituted for `latest`:

| Source (`latest/`) | Destination (`v1.28.0/` example) |
|---------------------|----------------------------------|
| `manifest.singleton.latest.json` | `manifest.singleton.1.28.0.json` |
| `manifest.version.latest.json` | `manifest.version.1.28.0.json` |
| `manifest.installer.latest.json` | `manifest.installer.1.28.0.json` |
| `manifest.defaultLocale.latest.json` | `manifest.defaultLocale.1.28.0.json` |
| `manifest.locale.latest.json` | `manifest.locale.1.28.0.json` |

The JSON content does **not** need editing; the `$id` already contains the version string.

### 2. Update `src/ManifestSchema/ManifestSchema.rc`

If the resource IDs for this version currently point to `latest/` (as they will if the C++ work
was done in a prior "add new version" PR), redirect them to the versioned files:

```
# Before
IDX_MANIFEST_SCHEMA_V1_28_SINGLETON   MANIFESTSCHEMA_RESOURCE_TYPE  "..\..\schemas\JSON\manifests\latest\manifest.singleton.latest.json"

# After
IDX_MANIFEST_SCHEMA_V1_28_SINGLETON   MANIFESTSCHEMA_RESOURCE_TYPE  "..\..\schemas\JSON\manifests\v1.28.0\manifest.singleton.1.28.0.json"
```

Repeat for all five manifest types.

### 3. Update `src/ManifestSchema/ManifestSchema.vcxitems`

Add five `<None>` entries for the new versioned files inside the existing `<ItemGroup>` that
contains the `<None>` elements. The `latest/` entries should remain unchanged.

```xml
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.singleton.1.28.0.json" />
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.version.1.28.0.json" />
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.installer.1.28.0.json" />
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.defaultLocale.1.28.0.json" />
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.locale.1.28.0.json" />
```

### 4. Update `src/ManifestSchema/ManifestSchema.vcxitems.filters`

Add a `<Filter>` entry for the new version directory, then add five `<None>` items that
reference the versioned files and assign them to that filter:

```xml
<!-- In the <ItemGroup> containing <Filter> elements -->
<Filter Include="schema\v1.28.0">
  <UniqueIdentifier>{new-guid-here}</UniqueIdentifier>
</Filter>

<!-- In the <ItemGroup> containing <None> elements -->
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.singleton.1.28.0.json">
  <Filter>schema\v1.28.0</Filter>
</None>
<!-- ... repeat for the other four types ... -->
```

---

## C++ and Test Changes

The four steps above cover the schema and project files. Depending on whether the C++
infrastructure was already added in a prior PR (pointing to `latest/`), some or all of the
following may also be needed in the same PR as the branching step.

### `src/AppInstallerCommonCore/Public/winget/ManifestCommon.h`

Add a version constant for the new version, following the pattern of existing entries:

```cpp
// V1.N manifest version
constexpr std::string_view s_ManifestVersionV1_N = "1.N.0"sv;
```

### `src/AppInstallerCommonCore/Manifest/ManifestSchemaValidation.cpp`

Prepend a new `if` block at the top of the version-check chain so that manifests at this
version or higher use the new schema resources:

```cpp
if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_N })
{
    resourceMap = {
        { ManifestTypeEnum::Singleton,     IDX_MANIFEST_SCHEMA_V1_N_SINGLETON },
        { ManifestTypeEnum::Version,       IDX_MANIFEST_SCHEMA_V1_N_VERSION },
        { ManifestTypeEnum::Installer,     IDX_MANIFEST_SCHEMA_V1_N_INSTALLER },
        { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_N_DEFAULTLOCALE },
        { ManifestTypeEnum::Locale,        IDX_MANIFEST_SCHEMA_V1_N_LOCALE },
    };
}
else if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_<PREVIOUS> })
// ...
```

### `src/ManifestSchema/ManifestSchema.h`

Add five resource-ID constants, continuing the numeric sequence (IDs must stay below 300,
per the comment in that file):

```c
#define IDX_MANIFEST_SCHEMA_V1_N_SINGLETON      NNN
#define IDX_MANIFEST_SCHEMA_V1_N_VERSION        NNN+1
#define IDX_MANIFEST_SCHEMA_V1_N_INSTALLER      NNN+2
#define IDX_MANIFEST_SCHEMA_V1_N_DEFAULTLOCALE  NNN+3
#define IDX_MANIFEST_SCHEMA_V1_N_LOCALE         NNN+4
```

### `src/WinGetUtilInterop/Manifest/ManifestVersion.cs`

Add a version constant:

```csharp
/// <summary>
/// V1.N manifest version.
/// </summary>
public const string ManifestVersionV1_N = "1.N.0";
```

### Test manifests and project files

Add representative YAML manifests that exercise the new schema version:

- `src/AppInstallerCLITests/TestData/ManifestV1_N-Singleton.yaml`
- `src/AppInstallerCLITests/TestData/ManifestV1_N-MultiFile-Version.yaml`
- `src/AppInstallerCLITests/TestData/ManifestV1_N-MultiFile-Installer.yaml`
- `src/AppInstallerCLITests/TestData/ManifestV1_N-MultiFile-DefaultLocale.yaml`
- `src/AppInstallerCLITests/TestData/ManifestV1_N-MultiFile-Locale.yaml`

Reference each file in `src/AppInstallerCLITests/AppInstallerCLITests.vcxproj` and
`.vcxproj.filters`, and add corresponding test cases in
`src/AppInstallerCLITests/YamlManifest.cpp`.
