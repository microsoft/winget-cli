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

A new `winget pin` command with 3 sub-commands will be introduced.
- Add package pinning configuration:

  `winget pin add <package> [--version <optional gated version>] [--source <source>] [--force] [--blocking]`

- Remove package pinning configuration:

  `winget pin remove <package> [--source <source>] [--force]`

- List package pinning configuration:

  `winget pin list <package> [--source <source>]` for a specific package or `winget pin list` to list all

#### Blocking
To block a package from upgrade, use `winget pin add <package> --blocking`
```text
cmd> winget pin Microsoft.TestApp --blocking
```
Now the pinning configuration is recorded as
```text
PackageIdentifier       SourceIdentifier       Version        PinningType
----------------------------------------------------------------------------
Microsoft.TestApp       winget                                Blocking
Microsoft.TestAppStore  msstore                               Blocking
```
**Note:** by default packages correlated from all sources are blocked, user can pass in `--source` to block for a specific source

Corresponding upgrade behavior
```text
cmd> winget upgrade -all
Microsoft TestApp is blocked from upgrade and skipped

cmd> winget upgrade Microsoft.TestApp
Microsoft TestApp is blocked from upgrade

cmd> winget upgrade Microsoft.TestApp --force
Success
```

#### Pinning
To pin a package from `winget upgrade --all`, use `winget pin add <package>`
```text
cmd> winget pin Microsoft.TestApp
```
Now the pinning configuration is recorded as
```text
PackageIdentifier       SourceIdentifier       Version        PinningType
----------------------------------------------------------------------------
Microsoft.TestApp       winget                                Pinning
Microsoft.TestAppStore  msstore                               Pinning
```
**Note:** by default packages correlated from all sources are pinned, user can pass in `--source` to pin for a specific source

Corresponding upgrade behavior
```text
cmd> winget upgrade -all
Microsoft TestApp is pinned from upgrade and skipped

cmd> winget upgrade Microsoft.TestApp
Success
```

#### Gating
To gate a package to some specific version, use `winget pin add <package> --version <gated version>`
```text
cmd> winget pin Microsoft.TestApp --version 1.2.*
```
Now the pinning configuration is recorded as
```text
PackageIdentifier       SourceIdentifier       Version        PinningType
----------------------------------------------------------------------------
Microsoft.TestApp       winget                 1.2.*          Gating
Microsoft.TestAppStore  msstore                1.2.*          Gating
```
**Note:** by default packages correlated from all sources are gated, user can pass in `--source` to gate for a specific source

Corresponding upgrade behavior
```text
cmd> winget upgrade -all
Success  // If the available versions for upgrade are: 1.2.3 and 1.3.0, the selected version for upgrade is 1.2.3

cmd> winget upgrade Microsoft.TestApp
Success  // If the available versions for upgrade are: 1.2.3 and 1.3.0, the selected version for upgrade is 1.2.3

cmd> winget upgrade Microsoft.TestApp --version 1.3.0
Microsoft TestApp is gated to version 1.2.* Override with --force

cmd> winget upgrade Microsoft.TestApp --version 1.3.0 --force
Success
```

**Note:** Regarding gated version syntax, it will be mostly same as what current winget version supports, except with special `.*` in the end as wild card matching any remaining version parts if there are any.

Example:
When `.*` in the end is detected:
Gate version `1.0.*` matches Version `1.0.1`
Gate version `1.0.*` matches Version `1.0`
Gate version `1.0.*` matches Version `1`
Gate version `1.0.*` matches Version `1.0.alpha`
Gate version `1.0.*` matches Version `1.0.1.2.3`
Gate version `1.0.*` matches Version `1.0.*`
Gate version `1.0.*` does not match Version `1.1.1`

In rare cases where `*` is actually part of a version, only the last `.*` is considered wild card:
Gate version `1.*.*` matches Version `1.*.1`
Gate version `1.*.*` matches Version `1.*.*`
Gate version `1.*.*` does not match Version `1.1.1`

If no `.*` in the end is detected, the gate version gates to the specific version:
Gate version `1.0.1` matches Version `1.0.1`
Gate version `1.0.1` does not match Version `1.1.1`

## Capabilities

### Accessibility

Accessibility should not be impacted by this change. There will be a few more tables printed to the terminal in certain cases, but they should use the current table implementation used by `winget upgrade` and `winget list`.

### Security

Security of the Windows Package Manager should not be impacted by this change. However, security of user's software may be, as if they pin a insecure version of a package, it will not be upgraded by Winget unless explicitly requested by user.

### Reliability

The change will improve reliability, as users will be able to have fine-grained control of the Windows Package Manager's upgrade functionality to ensure their workflow is not disrupted.

### Compatibility

There should not be any breaking changes to the code. Although there could be a mild breaking change to the behavior of `upgrade --all` (not all packages are upgraded anymore since pinned ones are skipped), this is purely opt-in from the user's perspective at this time (if they do not pin software, there should not be a change).

### Performance, Power, and Efficiency

There should not be any notable performance changes.

## Potential Issues

- Installation/Upgrades from Com Apis may be impacted by user's package pinning configuration. It could be mitigated by returning a specific error code and the caller  retrying with Force option.
- Package dependencies resolution may be impacted by user's package pinning configuration.
- Package imports may be impacted by user's package pinning configuration.

## Future considerations

- Implementation in this spec only supports pinning from remote sources, so all installed versions from same package share the same pinning configuration. Winget could better support side by side installations by introducing package pinning from installed source.
- Package pinning from user and from manifest are stored separately, we may integrate the `winget pin` commands to control package pinning from manifests.
- A couple UI integrations can be made to `winget upgrade` and `winget list` to show pinned status during listing.
- Dependencies flow can be improved to first check pinned status of each dependent package before trying to install all dependencies.
- Support setting pinned state right after installation/upgrades like `winget install foo --pin`.
- Improvements to import export commands to work seamlessly with existing package pinning configurations.

## Resources

- [Brew - How do I stop certain formulae from being upgraded?](https://docs.brew.sh/FAQ#how-do-i-stop-certain-formulae-from-being-updated)
- [NPM - package.json dependencies](https://docs.npmjs.com/cli/v7/configuring-npm/package-json#dependencies)
- [APT - Introduction to Holding Packages](https://help.ubuntu.com/community/PinningHowto#Introduction_to_Holding_Packages)
- [Chocolatey - pin a package](https://docs.chocolatey.org/en-us/choco/commands/pin)

Special thanks to [@jedieaston](https://github.com/jedieaston) for coming up with the initial draft of Package Pinning spec at [#1894](https://github.com/microsoft/winget-cli/pull/1894/). A lot has been discussed and this spec is much inspired from the draft.
