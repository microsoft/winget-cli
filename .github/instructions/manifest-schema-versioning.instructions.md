---
applyTo: "schemas/JSON/manifests/**,src/ManifestSchema/**"
---

# Manifest Schema Versioning

The `schemas/JSON/manifests/` directory holds JSON schemas for WinGet package manifests.
`latest/` is the in-development schema; each `v{X.Y.0}/` directory is a frozen snapshot.
See `schemas/JSON/manifests/README.md` for full documentation.

## Branching `latest/` to a frozen version

Run `schemas/JSON/manifests/Branch-LatestManifestSchema.ps1` from the repo root to automate
the schema-file and project-file steps. Then verify or complete the manual items below.

### Files to update

| File | Change |
|------|--------|
| `schemas/JSON/manifests/v{VERSION}/` | **Create** — copy each `latest/manifest.*.latest.json` to `manifest.*.{VERSION}.json`. No JSON content changes needed; `$id` already contains the version. |
| `src/ManifestSchema/ManifestSchema.h` | **Add** five `#define IDX_MANIFEST_SCHEMA_V{MAJOR}_{MINOR}_*` constants (Singleton/Version/Installer/DefaultLocale/Locale) using the next available IDs (must stay below 300). |
| `src/ManifestSchema/ManifestSchema.rc` | **Update** the five resource entries for this version from `latest/` paths to `v{VERSION}/` paths, **or add** them if they don't exist yet, pointing directly to `v{VERSION}/`. |
| `src/ManifestSchema/ManifestSchema.vcxitems` | **Add** five `<None Include="...v{VERSION}/..."/>` entries alongside the existing `latest/` entries. |
| `src/ManifestSchema/ManifestSchema.vcxitems.filters` | **Add** a `<Filter Include="schema\v{VERSION}">` with a new GUID, and five `<None>` items with `<Filter>schema\v{VERSION}</Filter>`. |
| `src/AppInstallerCommonCore/Public/winget/ManifestCommon.h` | **Add** `constexpr std::string_view s_ManifestVersionV{MAJOR}_{MINOR} = "{VERSION}"sv;` |
| `src/AppInstallerCommonCore/Manifest/ManifestSchemaValidation.cpp` | **Prepend** a new `if (manifestVersion >= ManifestVer{ s_ManifestVersionV{MAJOR}_{MINOR} })` block at the top of the version-check chain with the five `IDX_MANIFEST_SCHEMA_V{MAJOR}_{MINOR}_*` constants. |
| `src/WinGetUtilInterop/Manifest/ManifestVersion.cs` | **Add** `public const string ManifestVersionV{MAJOR}_{MINOR} = "{VERSION}";` |
| `src/AppInstallerCLITests/TestData/` | **Add** `ManifestV{MAJOR}_{MINOR}-Singleton.yaml` and `ManifestV{MAJOR}_{MINOR}-MultiFile-{Version,Installer,DefaultLocale,Locale}.yaml`. |
| `src/AppInstallerCLITests/AppInstallerCLITests.vcxproj` and `.vcxproj.filters` | **Add** references to the new test manifest files. |
| `src/AppInstallerCLITests/YamlManifest.cpp` | **Add** test cases exercising the new version. |

### ManifestSchemaValidation.cpp pattern

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

### Notes

- If C++ infrastructure (ManifestCommon.h, ManifestSchemaValidation.cpp, ManifestSchema.h, ManifestVersion.cs) was already added in a prior PR pointing to `latest/`, only update ManifestSchema.rc to redirect from `latest/` to the versioned path; the constants and logic stay the same.
- Resource IDs in `ManifestSchema.h` are numbered sequentially in groups of 5 (one group per schema version). The comment near the top warns they must stay below 300.
- The `latest/` `<None>` entries in `ManifestSchema.vcxitems` are never removed; versioned entries are added alongside them.
