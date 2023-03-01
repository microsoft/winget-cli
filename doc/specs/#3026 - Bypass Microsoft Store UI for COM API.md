---
author: Roy MacLachlan RDMaclachlan/roy.maclachlan@microsoft.com
created on: 2023-02-09
last updated: 2023-02-09
issue id: 3026
---

# Spec Title

"For [#3026](https://github.com/microsoft/winget-cli/issues/3026)"

## Abstract

This sepc describes the allowing WinGet install to install applications from the Microsoft Store when the "Turn off the Store application" group policy object has been enabled.

## Inspiration

This is inspired by
* Enterprise customer feedback requiring the ability to disable the Microsoft Store on Enterprise devices but capable of delivering Microsoft Store apps through deployment technologies (Example: Microsoft Intune).
* Education customers feedback requiring the ability to control what a student can install on education devices, while continuing to leverage deployment technologies (Example: Microsoft Intune).


## Solution Design

The Windows Package Manager will allow installation of Microsoft Store applications for the COM API used by Mobile Device Management (MDM) solutions (example: Intune) while continuing to block store application installations through the command line interface (CLI) and PowerShell when the "Turn off the Store application" group policy has been enabled.

An exception will be created for the COM APIs to enable 

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
