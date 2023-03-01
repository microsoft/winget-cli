---
author: Roy MacLachlan RDMaclachlan/roy.maclachlan@microsoft.com
created on: 2023-02-09
last updated: 2023-02-09
issue id: 3026
---

# Spec Title

"For [#3026](https://github.com/microsoft/winget-cli/issues/3026)"

## Abstract

This spec describes the allowing WinGet install to install applications from the Microsoft Store when the "Turn off the Store application" group policy object has been enabled.

## Inspiration

This is inspired by
* Enterprise customer feedback requiring the ability to disable the Microsoft Store on Enterprise devices but capable of delivering Microsoft Store apps through deployment technologies (Example: Microsoft Intune).
* Education customers feedback requiring the ability to control what a student can install on education devices, while continuing to leverage deployment technologies (Example: Microsoft Intune).


## Solution Design

A new ADMX policy will be created that will override the "Turn off the Store application" group policy object when enabled.

### Policy Name:
Enable Microsoft Store App Bypass

### Policy Description:
Enables the search and install of Microsoft Store applications on a Windows device that have enabled the "Turn off the Store application" policy to disable the use of the Microsoft Store application.

If you enable this setting, the Windows Package Manager can be used to perform application installations on the device while the use of the Microsoft Store application is disabled.

If you disable, or do not configure this setting, the Windows Package Manager will continue to block the installation of Microsoft Store applications while the "Turn off the Store application" policy has been enabled.

### Configurable settings:
Enable / Disable.

## UI/UX Design

A new Group Policy object will exist that enables IT Administrators to deploy applications from the Microsoft Store without enabling the Microsoft Store application UI.

## Capabilities

### Accessibility

This feature will have no impact.

### Security

This feature will allow `WinGet install` of Microsoft Store applications.

### Reliability

This feature will have no impact.

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
