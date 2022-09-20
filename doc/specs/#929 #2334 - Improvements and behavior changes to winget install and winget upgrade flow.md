---
author: Yao Sun @yao-msft
created on: 2022-09-19
last updated: 2022-09-19
issue id: 929, 2334
---

# Improvements and behavior changes to winget install and winget upgrade flow

For [#929](https://github.com/microsoft/winget-cli/issues/929), [#2334](https://github.com/microsoft/winget-cli/issues/2334)

## Abstract

This is a mini spec for describing upcoming beahvior changes and improvements to `winget install` and `winget upgrade` workflows.

## Solution Design

### Winget Install flow will look for installed packages and act accordingly

**Existing Behavior**: `winget install` is a light-weight command that just installs the found package with latest version without any installed package detection and applicable version selection.

**New Behavior**: By default, `winget install` will check for installed package after a package is found in a source. If an installed package is found, `winget install` will inform user in bold text and try to do a `winget upgrade` workflow instead. User will get `No applicable upgrade` if upgradable version not available. If an installed package is not found, `winget install` will try to search through all available package versions and find the latest that's applicable, in hope that installation success will be higher with less `No applicable installer`.

**Note**: To better accommodate various user needs, the existing behavior will be preserved and can be invoked with `--force` flag.

### Winget Upgrade flow will
