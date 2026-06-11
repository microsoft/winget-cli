# WinGet Manifest Schemas

This directory contains JSON schemas for WinGet package manifests.

## Directory Structure

| Directory | Contents |
|-----------|----------|
| `latest/` | The in-development schema. File names use `latest` as the version token. The `$id` field in each file contains the **current version string** (e.g., `1.29.0`). |
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

The version string embedded in the `$id` of each `latest/` file is the authoritative schema
version. The binary version is defined in `src/binver/binver/version.h` as
`{VERSION_MAJOR}.{VERSION_MINOR}.0`.

---

## Helper script

`Branch-LatestManifestSchema.ps1` (in this directory) automates most of both workflows below.
Run from any directory; it locates the repo root automatically.

```powershell
# Freeze the current latest/ schema at its version (no source-file changes).
.\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1

# Freeze latest/, then advance to match the binary version and update all source files.
.\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1 -BumpVersion

# Preview changes without writing anything.
.\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1 -BumpVersion -WhatIf
```

---

## Workflow A: Advancing to a new schema version

Use this when starting work on a new set of schema changes (e.g. after `version.h` has been
bumped to a new minor version). The goal is to ensure `latest/` carries the new version number
before any schema edits are made.

**Run the script:**

```powershell
.\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1 -BumpVersion
```

The script performs the following automatically:

1. Reads the target version from `src/binver/binver/version.h` (`MAJOR.MINOR.0`).
2. Reads the current schema version from `latest/manifest.installer.latest.json` (`$id` field).
3. If the versions already match, exits with no changes.
4. If they differ, branches the current schema (see Workflow B steps 1–4 below).
5. Replaces every occurrence of the old version string in all five `latest/` JSON files (`$id`
   and `description` fields).
6. Adds a new version constant to `ManifestCommon.h`.
7. Adds five new `IDX_MANIFEST_SCHEMA_*` resource-ID constants to `ManifestSchema.h`.
8. Appends new resource entries (pointing to `latest/`) to `ManifestSchema.rc`.
9. Prepends a new `if (manifestVersion >= ...)` block at the top of the version-check chain in
   `ManifestSchemaValidation.cpp`, converting the old top block to an `else if`.
10. Adds a new version constant to `ManifestVersion.cs`.

**Manual steps always required (not automated):**

- Add representative YAML test manifests:
  - `src/AppInstallerCLITests/TestData/ManifestV{MAJOR}_{MINOR}-Singleton.yaml`
  - `src/AppInstallerCLITests/TestData/ManifestV{MAJOR}_{MINOR}-MultiFile-{Version,Installer,DefaultLocale,Locale}.yaml`
- Reference each file in `src/AppInstallerCLITests/AppInstallerCLITests.vcxproj` and
  `.vcxproj.filters`.
- Add corresponding test cases in `src/AppInstallerCLITests/YamlManifest.cpp`.

---

## Workflow B: Freezing `latest/` into a numbered version

Use this when a WinGet release is approaching and the in-flight schema needs to be locked in —
i.e., `latest/` is frozen at its current version number and subsequent changes will target the
next version.

The version to freeze is embedded in the `$id` field of each `latest/` file — for example:

```
"$id": "https://aka.ms/winget-manifest.installer.1.28.0.schema.json"
                                                   ^^^^^^
                                                   This is the version.
```

**Run the script (automates steps 1–4):**

```powershell
.\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1
```

### Step 1 — Create the versioned schema directory

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

### Step 2 — Update `src/ManifestSchema/ManifestSchema.rc`

Redirect the five resource entries for this version from `latest/` to the versioned paths:

```
# Before
IDX_MANIFEST_SCHEMA_V1_28_SINGLETON   MANIFESTSCHEMA_RESOURCE_TYPE  "..\..\schemas\JSON\manifests\latest\manifest.singleton.latest.json"

# After
IDX_MANIFEST_SCHEMA_V1_28_SINGLETON   MANIFESTSCHEMA_RESOURCE_TYPE  "..\..\schemas\JSON\manifests\v1.28.0\manifest.singleton.1.28.0.json"
```

Repeat for all five manifest types.

### Step 3 — Update `src/ManifestSchema/ManifestSchema.vcxitems`

Add five `<None>` entries for the new versioned files inside the existing `<ItemGroup>` that
contains the `<None>` elements. The `latest/` entries should remain unchanged.

```xml
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.singleton.1.28.0.json" />
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.version.1.28.0.json" />
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.installer.1.28.0.json" />
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.defaultLocale.1.28.0.json" />
<None Include="$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests\v1.28.0\manifest.locale.1.28.0.json" />
```

### Step 4 — Update `src/ManifestSchema/ManifestSchema.vcxitems.filters`

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

## C++ and test changes (both workflows)

If the C++ infrastructure for a version was **not** already added in a prior PR (as happens
when branching occurs separately from the initial feature work), these files also need changes.
When using `-BumpVersion`, the script handles all of these except the test manifests.

### `src/AppInstallerCommonCore/Public/winget/ManifestCommon.h`

Add a version constant before the "Any new manifest version" comment:

```cpp
// V1.N manifest version
constexpr std::string_view s_ManifestVersionV1_N = "1.N.0"sv;
```

### `src/AppInstallerCommonCore/Manifest/ManifestSchemaValidation.cpp`

Prepend a new `if` block at the top of the version-check chain so that manifests at this
version or higher use the new schema resources. The previous top-level `if` becomes `else if`:

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

Add five resource-ID constants, continuing the numeric sequence. IDs must stay below 300
(per the comment in that file):

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
