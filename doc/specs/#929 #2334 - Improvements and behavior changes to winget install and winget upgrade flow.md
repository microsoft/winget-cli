---
author: Yao Sun @yao-msft
created on: 2022-09-19
last updated: 2022-09-19
issue id: 929, 2334
---

# Improvements and behavior changes to winget install and winget upgrade flow

For [#929](https://github.com/microsoft/winget-cli/issues/929), [#2334](https://github.com/microsoft/winget-cli/issues/2334)

## Abstract

This is a mini spec for describing upcoming behavior changes and improvements to `winget install` and `winget upgrade` workflows.

## Solution Design

### Winget Install flow will look for installed packages and act accordingly

**Existing Behavior**: `winget install` is a light-weight command that just installs the found package with latest version without any installed package detection and applicable version selection.

**New Behavior**: By default, `winget install` will check for installed package after a package is found in a source. If an installed package is found, `winget install` will inform user in bold text and try to do a `winget upgrade` workflow instead. User will get `No applicable upgrade` if upgradable version not available. If an installed package is not found, `winget install` will try to search through all available package versions and find the latest that's applicable, in hope that installation success will be higher with less `No applicable installer`.

**Note**: To better accommodate various user needs, the existing behavior will be preserved and can be invoked with `--force` argument.

### Winget Upgrade flow will try to select installer that better matches installed package

**Existing Behavior**: `winget upgrade` does not try to select installer by installed package's architecture, locale.

**New Behavior**:

- `architecture` and `locale` arguments will be added to `winget upgrade` command
- winget will try to record selected installer's architecture and locale for installation through winget
- winget will record architecture or locale from command line arguments as user intent. i.e. `winget install foo --architecture x86 --locale en-US`
- During upgrade flow installer selection, installer architecture or locale from previous installation will be treated as preference. Installer architecture or locale from user intent will be treated as requirement (i.e. the upgrade will fail if architecture or locale requirement cannot be met).

**Note**: This improvement only works for installations through winget. Due to current limitations of winget tracking implementation, side by side installations may not work perfectly, as winget will only honor metadata from last installation for the same package. User would need to provide `--architecture` or `--locale` to override the last installation metadata when side by side scenarios fail to work as expected.

### `--force` argument separation from Override Hash Mismatch

**Existing Behavior**: Currently, the `--force` argument is overloaded with overriding installer hash mismatch, overriding conflicting portable package, and potentially overriding the new `winget install` behavior.

**New Behavior**: Since hash mismatch overriding is security related, it warrants a dedicated argument. `--ignore-security-hash`(name suggested in [#715](https://github.com/microsoft/winget-cli/issues/715)) will be introduced to represent installer hash mismatch overriding. `--force` argument will be kept for generic workflow behavior overriding.
