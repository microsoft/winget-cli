---
author: Kaleb Luedtke (Trenly) trenlymc@gmail.com
created on: 2023-03-10
last updated: 2023-03-10
issue id: 3061
---

# Marketing Version

"For [#3061](https://github.com/microsoft/winget-cli/issues/3061)"

## Abstract

There are times when the version which is marketed to consumers on publishers' websites differs from what is written to the Apps and Features Entries data. Additionally, there are times when the same version is marketed for several different versions of the same package. The Windows Package Manager should be able to show an arbitrary version as determined by the publisher.

## Inspiration

The current design using `PackageVersion` for the displayed version and how it differs from `DisplayVersion` can be confusing.

## Solution Design

The Windows Package Manager manifest schema should have a `MarketingVersion` key added at the root level. For clarity, it should be placed immediately following the `PackageVersion` key.

When the Windows Package Manager needs to display the version of a package, the value from the `MarketingVersion` key should be used. If the key is not present or has no value, the value of `PackageVersion` should be displayed instead, with the exception of `winget show <query> --versions` and `winget search <query> --versions` which should show the values as key pairs of `PackageVersion (MarketingVersion)`.

When the `--version` parameter is used, matches with `PackageVersion` should be considered first and matches with `MarketingVersion` should be considered second. If there are multiple package versions of the same identifier matched using `MarketingVersion`, the highest `PackageVersion` shall be selected.

When `MarketingVersion` is present, `PackageVersion` should be validated to be a semantic version.

For the purposes of version comparison, mapping installed packages to versions, and all other behaviors, the current implementation using `PackageVersion` should be retained.

## UI/UX Design

Example of a manifest with a `MarketingVersion` specified:
```yaml
PackageIdentifier: Example.Package
PackageVersion: 1.2.5.1
MarketingVersion: 1.2.5
Installers:
  InstallerUrl: https://example.com
  InstallerSha256: <SHA>
  InstallerType: exe
  AppsAndFeaturesEntries:
    - DisplayVersion: 1.2.5-abcde
...
```

### `winget list`
```
Name                Id               Version   Available
---------------------------------------------------------- 
Example Package     Example.Package  1.2.4     1.2.5
```

### `winget show Example.Package --versions`
```
Found Example Package [Example.Package]
Version
-------------
1.2.5.1  (1.2.5)
1.2.4.9  (1.2.4)
1.2.4.8b (1.2.4 beta)
1.2.4.7b (1.2.4 beta)
```

### `winget upgrade` when the installed version and newer version have the same marketing version
```
Name                Id               Version   Available    Source
--------------------------------------------------------------------
Example Package     Example.Package  1.2.5     * 1.2.5      winget
```

### `winget install Example.Package --version 1.2.4.7b`
*This would install version 1.2.4.7b*
```
Found Example Package [Example.Package] Version 1.2.4 beta
This application is licensed to you by its owner.
...
```

### `winget install Example.Package --version '1.2.4 beta'`
*This would install version 1.2.4.8b*
```
Found Example Package [Example.Package] Version 1.2.4 beta
This application is licensed to you by its owner.
...
```

## Capabilities
Installed packages should still be mapped to `PackageVersion`, which will allow winget to update packages which change `DisplayVersion` but do not change `MarketingVersion`.

`MarketingVersion` will allow for packages which use non-semantic versions to have a semantic `PackageVersion` while retaining the non-semantic mapping for `DisplayVersion` and consumer-facing version messages.

Tab Completion should continue to reference the Package Version.

### Accessibility

The Windows Package Manager has been built in such a way that screen readers will still provide audible output as the command is executed keeping the user informed of progress, warnings, and errors. This should have no direct impact on accessibility.

### Security

There should be no security impact.

### Reliability

This change will require exposing the marketing version as part of the pre-indexed package and REST source. However, the reliability of the publishing pipelines is not anticipated to be affected.

### Compatibility

Marketing Versions will be an extension to the current version framework, so no compatibility issues are expected to occur as a result.

This change will require exposing the marketing version as part of the pre-indexed package and REST source. Older versions of the client may not know how to handle this field

### Performance, Power, and Efficiency

The amount of computation required to display the arbitrary value of a text key should have little impact on the current performance of the Windows Package Manager

## Potential Issues

* There could be confusion around how to specify a version when using commands.

## Future considerations

* Logging the latest installed `PackageVersion` in the `installed.db` may allow for the updating of packages that don't change `DisplayVersion` but do change `MarketingVersion`