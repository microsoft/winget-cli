---
author: Demitrius Nelon @denelon
created on: 2021-06-11
last updated: 2021-06-11
issue id: 1155
---

# Enhancements to List Command

For [#1155](https://github.com/microsoft/winget-cli/issues/1155).

## Abstract

The `winget list` command was designed to display all packages installed on a users Windows 10 machine in Add / Remove Programs (ARP). The command would also show which Apps with possible upgrades were available from sources configured in the Windows Package Manager. This specification proposes enhancements to the output provided by the list command.

The first enhancement is adding the values for "Available" and "Source" for every package in configured sources rather that just packages with upgrades available.

The second enhancement is reducing the set of packages to the source specified when the source is passed as an argument to the list command.

The third enhancement is adding an argument to provide the list of Apps with no matching package in configured sources.

## Inspiration

Several suggestions have been made on how to improve user experience. Recently, many users have been attempting to identify Apps on their system with no corresponding package in the Microsoft Community Repository.

## Solution Design

Modify the output behavior for `winget list` to display available version from any source. When a package is available via more than once source, the first configured source should be displayed.

>Note: Users will be able to see if packages are available from individual sources by specifying the source as an argument to the list command.

Modify the output behavior for `winget list -s <source>` to display only packages available from the specified source.

Add a new argument `-u, --unavailable`  to the list command to display Apps installed in ARP not matching any configured source.

## UI/UX Design

The UI would remain consistent with the current tabular output.

## Capabilities

### Accessibility

This should not change impact accessibility for users of screen readers, assistive input devices, etc.

### Security

This should not change or impact security?

### Reliability

This should not impact reliability.

### Compatibility

This will change existing behaviors. The `winget list` output currently displays available versions and sources in addition to all other installed Apps. The `winget upgrade` command should be used instead to view available upgrades.

### Performance, Power, and Efficiency

## Potential Issues

This may impact performance, but it should be minor. Another Issue [#964](https://github.com/microsoft/winget-cli/issues/964) "Improvements to the list command" was created to address performance issues related to the `winget list` command.

## Future considerations

This feature may simplify the process of identifying packages missing from source repositories.

## Resources

Issue [#977](https://github.com/microsoft/winget-cli/issues/977) "`winget list` should be able to show hidden apps" is also related to the `winget list` command.
