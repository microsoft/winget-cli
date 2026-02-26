---
author: Roy MacLachlan RDMaclachlan/roy.maclachlan@microsoft.com
created on: 2023-02-09
last updated: 2023-02-09
issue id: 3025
---

# Bypass Microsoft Store blocked for COM API

"For [#3025](https://github.com/microsoft/winget-cli/issues/3025)"

## Abstract

This spec describes allowing WinGet install COM APIs to install applications from the Microsoft Store when the Store application has been disabled through group policy object or SLAPI policies.

## Inspiration

This is inspired by
* Enterprise customer feedback requiring the ability to disable the Microsoft Store on Enterprise devices but capable of delivering Microsoft Store apps through deployment technologies (Example: Microsoft Intune).
* Education customers feedback requiring the ability to control what a student can install on education devices, while continuing to leverage deployment technologies (Example: Microsoft Intune).


## Solution Design

The Windows Package Manager will allow the WinGet COM APIs to perform Microsoft Store application installations that will bypass the check on Microsoft Store is blocked by policy (IsStoreBlockedByPolicyAsync). Both the WinGet command line interface (CLI) and WinGet PowerShell commands will continue to block application installations from the Store when the Microsoft Store is blocked by policy.

The checks on Microsoft Store blocked by policy will allow calls through COM, with the exception of PowerShell and CLI.

## UI/UX Design

None.

## Capabilities

### Accessibility

None.

### Security

None.

### Reliability

None.

### Compatibility

This proposed change will allow IT Administrators to allow the installation of Microsoft Store applications using the WinGet while preventing users from using the Microsoft Store application to search and install applications.

This setting is an override that will be disabled by default.

### Performance, Power, and Efficiency

## Potential Issues

None.

## Future considerations

None.

## Resources

N/A
