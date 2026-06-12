---
applyTo: "schemas/JSON/manifests/**,src/ManifestSchema/**"
---

# Manifest Schema Versioning

The `schemas/JSON/manifests/` directory holds JSON schemas for WinGet package manifests.
`latest/` is the in-development schema; each `v{X.Y.0}/` directory is a frozen snapshot.
The binary version is defined in `src/binver/binver/version.h` as `{VERSION_MAJOR}.{VERSION_MINOR}.0`.
See `schemas/JSON/manifests/README.md` for full documentation.

## Script

`schemas/JSON/manifests/Checkpoint-LatestManifestSchema.ps1` automates most steps in both workflows.

```powershell
# Freeze only (no version bump, C++ changes are manual).
.\schemas\JSON\manifests\Checkpoint-LatestManifestSchema.ps1

# Freeze current latest/, then advance schema to match the binary version (automates C++ source edits).
.\schemas\JSON\manifests\Checkpoint-LatestManifestSchema.ps1 -BumpVersion

# Dry run.
.\schemas\JSON\manifests\Checkpoint-LatestManifestSchema.ps1 -BumpVersion -WhatIf
```

## Workflow A: Starting a new schema version (`-BumpVersion`)

Run when `version.h` has been advanced to a new minor version and the `latest/` schema version
does not yet match. The script automates steps 1–7 below. Step 8 (test manifests) is always manual.

| Step | File(s) | Change |
|------|---------|--------|
| 1 | `schemas/JSON/manifests/v{OLD}/` | **Create** frozen copy of current `latest/` (only if the folder doesn't exist). |
| 2 | `schemas/JSON/manifests/latest/*.json` | **Replace** every occurrence of the old version string with the new one (`$id` and `description` fields). |
| 3 | `src/AppInstallerCommonCore/Public/winget/ManifestCommon.h` | **Add** `constexpr std::string_view s_ManifestVersionV{MAJOR}_{MINOR} = "{VERSION}"sv;` before the "Any new manifest version" comment. |
| 4 | `src/ManifestSchema/ManifestSchema.h` | **Add** five `IDX_MANIFEST_SCHEMA_V{MAJOR}_{MINOR}_*` constants (Singleton/Version/Installer/DefaultLocale/Locale) at the next available IDs (must stay below 300). |
| 5 | `src/ManifestSchema/ManifestSchema.rc` | **Append** five new resource entries pointing to `latest/`. |
| 6 | `src/AppInstallerCommonCore/Manifest/ManifestSchemaValidation.cpp` | **Prepend** a new `if (manifestVersion >= ManifestVer{ s_ManifestVersionV{MAJOR}_{MINOR} })` block; old top block becomes `else if`. |
| 7 | `src/WinGetUtilInterop/Manifest/ManifestVersion.cs` | **Add** `public const string ManifestVersionV{MAJOR}_{MINOR} = "{VERSION}";` |

## Workflow B: Freezing `latest/` at a release

Run when a WinGet release approaches and the in-flight schema must be locked in.
The script (without `-BumpVersion`) automates steps 1–4 below. Steps 5–8 are only needed if
the C++ infrastructure was not set up in a prior PR.

| Step | File(s) | Change |
|------|---------|--------|
| 1 | `schemas/JSON/manifests/v{VERSION}/` | **Create** — copy each `latest/manifest.*.latest.json` to `manifest.*.{VERSION}.json`. |
| 2 | `src/ManifestSchema/ManifestSchema.rc` | **Update** five resource entries from `latest/` paths to `v{VERSION}/` paths. |
| 3 | `src/ManifestSchema/ManifestSchema.vcxitems` | **Add** five `<None Include="...v{VERSION}/..."/>` entries. |
| 4 | `src/ManifestSchema/ManifestSchema.vcxitems.filters` | **Add** `<Filter Include="schema\v{VERSION}">` with a new GUID, and five `<None>` items with `<Filter>schema\v{VERSION}</Filter>`. |
| 5 | `src/AppInstallerCommonCore/Public/winget/ManifestCommon.h` | **Add** version constant (if not already present). |
| 6 | `src/ManifestSchema/ManifestSchema.h` | **Add** five IDX constants (if not already present). |
| 7 | `src/AppInstallerCommonCore/Manifest/ManifestSchemaValidation.cpp` | **Prepend** new if-block (if not already present). |
| 8 | `src/WinGetUtilInterop/Manifest/ManifestVersion.cs` | **Add** version constant (if not already present). |

## ManifestSchemaValidation.cpp pattern

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
```

## Key invariants

- `latest/` `<None>` entries in `ManifestSchema.vcxitems` are **never removed**; versioned entries are added alongside them.
- `ManifestSchema.rc` entries for the **current** version always point to `latest/`; entries for all **prior** versions point to their `v{VERSION}/` directories.
- Resource IDs in `ManifestSchema.h` are sequential groups of 5, must stay below 300.
