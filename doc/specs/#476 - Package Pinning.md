---
author: Yao Sun @yao-msft
created on: 2022-10-12
last updated: 2022-10-12
issue id: 476
---

# Package Pinning

For [#476](https://github.com/microsoft/winget-cli/issues/476)

## Abstract

This spec describes the functionality and high level implementation design of Package Pinning feature.

## Inspiration

This is inspired by functionalities in other package managers, as well as community feedback.
- Packages may introduce breaking changes that users may not want integrate into their workflow quite yet.
- Packages may update themselves so that it will be duplicate effort for winget to try to update them.
- User may want to maintain some of the packages through other channels outside of winget, or prefer one source over others within winget.
- User may want some of the packages to stay in some major versions but allow minor version changes during upgrade.

## Solution Design

#### Package Pinning types
To achieve goals listed above, winget will support 3 types of Package Pinning:
- **Blocking:** The package is blocked from `winget upgrade --all` or `winget upgrade <specific package>`, user has to unblock the package to let winget perform upgrade.
- **Pinning:** The package is excluded from `winget upgrade --all` but allowed in `winget upgrade <specific package>`, a new argument `--include-pinned` will be introduced to let `winget upgrade --all` to include pinned packages.
- **Gating:** The package is pinned to specific version(s). For example, if a package is pinned to version `1.2.*`, any version between `1.2.0` to `1.2.<anything>` is considered valid.

To allow user override, `--force` can be used with `winget upgrade <specific package>` to override some of the pinning created above.

#### Package Pinning Configuration Storage

A separate sqlite db (other than the existing tracking catalog) will be created to store the package pinning configurations from user.
```text
PackageIdentifier       SourceIdentifier       Version        PinningType
----------------------------------------------------------------------------
Microsoft.TestApp       winget                 1.2.*          Gating
```

**Notes:** For this iteration, winget will only support pinning packages that are locally installed and correlatable with at least one of the remote sources. Winget will record a pinned package by the PackageIdentifier and SourceIdentifier. There can only be one pinning configuration for a specific package. In the future, winget may consider pinning packages from installed packages (upon improving the installed package's PackageIdentifier logic).

## UI/UX Design

#### winget pin commands


## Capabilities

### Accessibility

Accessibility should not be impacted by this change. There will be a few more tables printed to the terminal in certain cases, but they should use the current table implementation used by `winget upgrade` and `winget list`.

### Security

Security of the Windows Package Manager should not be impacted by this change. However, security of user's software may be, as if they pin a insecure version of a package, it will not be upgraded by Winget unless explicitly requested by user.

### Reliability

The change will improve reliability, as users will be able to have fine grained control of the Windows Package Manager's upgrade functionality to ensure their workflow is not disrupted. 

### Compatibility

There should not be any breaking changes to the code. Although there could be a mild breaking change to the behavior of `upgrade --all` (not all packages are upgraded anymore since pinned ones are skipped), this is purely opt-in from the user's perspective at this time (if they do not pin software, there should not be a change).

### Performance, Power, and Efficiency

There should not be any notable performance changes.

## Potential Issues

- Installation/Upgrades from Com Apis may be impacted by user's package pinning configuration. It could be mitigated by returning a specific error code and the caller to retry with Force option.
- Package dependencies resolution may be impacted by user's package pinning configuration.

## Future considerations

[comment]: # What are some of the things that the fixes/features might unlock in the future? Does the implementation of this spec enable scenarios?

## Resources

[comment]: # Be sure to add links to references, resources, footnotes, etc.
