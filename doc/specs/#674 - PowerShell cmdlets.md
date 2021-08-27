---
author: Demitrius Nelon @denelon
created on: 2021-08-06
last updated: 2021-08-27
issue id: 674
---

# When do we begin to see the PowerShell cmdlets?

"For [#674](https://github.com/microsoft/winget-cli/issues/674)"

## Abstract

This specification is to begin drafting PowerShell cmdlets for the Windows Package Manager. @doctordns submitted the issue
and this specification is intended to capture the intent for the syntax. The PowerShell Team has recently released
[PowerShell Crescendo Preview.3](https://devblogs.microsoft.com/powershell/announcing-powershell-crescendo-preview-3/). we
are considering using this framework to help model the syntax and create a module so users can experiment with the syntax.
Several of the winget.exe commands provide output that is not representative of properly formatted objects, so there will
likely be gaps to address natively in the COM interface for the Windows Package Manager.

## Inspiration

@ThomasNieto was the first to create an Issue for [Native PowerShell Support](https://github.com/microsoft/winget-cli/issues/221).
This is one of the [most desired features](https://github.com/microsoft/winget-cli/issues?page=1&q=is%3Aissue+is%3Aopen+sort%3Areactions-%2B1-desc+label%3AIssue-Feature) for the Windows Package Manager.

## Solution Design

The initial solution will start with [PowerShell Crescendo Preview.3](https://devblogs.microsoft.com/powershell/announcing-powershell-crescendo-preview-3/).
Many of the major considerations are related to proper PowerShell nouns and verbs. @MilkFather suggested [command aliases](https://github.com/microsoft/winget-cli/issues/380) for the winget.exe
client and @Jaifroid also suggested [find](https://github.com/microsoft/winget-cli/issues/1299) rather than search.

## UI/UX Design

The main UI/UX differences will be in the command names, parameters, and how the output is formatted for users. For the sake of
consistency the sub headings below will map to existing commands in winget.exe to begin drafting some syntax proposals. In order
to keep the discussion focussed, I'll create GitHub discussions for each of these and link them here as we work through driving
consensus.

The PowerShell module name will be "WindowsPackageManager"
The Adjective "WinGet" may change, but is used consistently in the cmdlets until such time as we agree on the proper term.

>Note: Naming things is hard. One of the conventions of PowerShell is to make nouns singular.
---

### Client command `winget install`

**PowerShell Cmdlet**

```PowerShell
Install-WinGetPackage
```
**Parameters**

-Manifest

-PackageIdentifier

-Name

-Moniker

-Version

-Source

-Scope

-Exact

-Interactive

-Silent
>Note: we may switch the default to "silent" from "silent with progress" in winget.exe so there may be changes here.

-Locale

-Log

-Override

-Location

### Client command `winget show`

>Note: We considered Show-WinGetPackage, but in PowerShell users may want to perform their own search. We are currently thinking we would use "Find-WingetPackage -detail".

### Client command `winget source`

**PowerShell Cmdlets**

```PowerShell
Add-WinGetSource
```

```PowerShell
Remove-WinGetSource
```

```PowerShell
Get-WinGetSource
```

**Example**
> Note: `winget source export` was added as a helper for adding approved sources to Group Policy. To get the same values with PowerShell the example blow would perform the same behavior.


```PowerShell
Get-WinGetSource | convertto-json -compress
```

### Client command `winget search`

**PowerShell Cmdlet**

```PowerShell
Find-WinGetPackage
```

**Parameters**

-Detail

-PackageIdentfier

-Name

-Moniker

-Tag

-Command

-Source

-Count

-Exact

### Client command `winget list`

**PowerShell Cmdlet**

```PowerShell
Get-WinGetPackage
```

### Client command `winget upgrade`

**PowerShell Cmdlet**

```PowerShell
Update-WinGetPackage
```

### Client command `winget uninstall`

**PowerShell Cmdlet**

```PowerShell
Uninstall-WinGetPackage
```

>Note: One example below could be executed to remove all packages with "xbox" in their name, or in other metadata fields provided in the manifest.

```PowerShell
Get-WingetPackage xbox | UninstallWingetPackage
```

### Client command `winget hash`

**PowerShell Cmdlet**

```PowerShell
Get-WinGetHash
```

**Parameters**

-Installer

-Signature

### Client command `winget validate`

**PowerShell Cmdlet**

Test-WinGetManifest

> Note: we are considering building an "interactive" validation that could test installing the package in the Windows Sandbox for example. That could be another cmdlet or a switch like "-validate".

### Client command `winget settings`

**PowerShell Cmdlets**

```PowerShell
Get-WinGetSetting
```

```PowerShell
Set-WinGetSetting
```

### Client command `winget features`

**PowerShell Cmdlets**

```PowerShell
Get-WinGetFeature
```

```PowerShell
Enable-WingetFeature
```

```PowerShell
Disable-WingetFeature
```

### Client command `winget export`

**PowerShell Cmdlet**

```PowerShell
Export-WinGetPackageSet 
```
**Parameters**

-Source

-IncludeVersions

### Client command `winget import`

**PowerShell Cmdlet**

```PowerShell
Import-WinGetPackageSet
```
**Parameters**

-ImportFile

-IgnoreUnavailable

-IgnoreVersions

## Capabilities

The set of capabilities for this feature are to achieve parity with all of the winget.exe behaviors.
>Note: As stated above, the output initially will likely not be fully complete until other changes have been made to support
properly formatted output.

### Accessibility

Accessibility should not be impacted in a negative manner. In some cases, the output may be more complete than winget.exe
and as such would represent potential improvements.

### Security

This should not introduce any _new_ security concerns. Although PowerShell does have different security implications as
compared with the winget.exe client execution.

### Reliability

This is not expected to impact reliability.

### Compatibility

This is not expected to impact compatibility. This will be the first implementation for PowerShell cmdlets and a Windows
Package Manager PowerShell module.

### Performance, Power, and Efficiency

There may be slightly reduced performance as this PowerShell interface via Crescendo will be a wrapper to the .exe, but it
should not be materially noticeable by users.

## Potential Issues

There may still be issues when processing output that has not been converted to provide the complete string values in
the event strings have been truncated by winget.exe.

## Future considerations

In order to have full native PowerShell support, all of the output should be provided. For this set of cmdlet previews, we are
expecting to iterate over time, and we will likely have to design special output capabilities in the COM interface.

## Resources


[Windows PowerShell Cmdlet Concepts](https://docs.microsoft.com/en-us/powershell/scripting/developer/cmdlet/windows-powershell-cmdlet-concepts?view=powershell-7.1)
[Writing a Windows PowerShell Module](https://docs.microsoft.com/en-us/powershell/scripting/developer/module/writing-a-windows-powershell-module?view=powershell-7.1)
[PowerShell Script Module Design: Building Tools to Automate the Process](https://mikefrobbins.com/2018/11/01/powershell-script-module-design-building-tools-to-automate-the-process/)
[Powershell: DSL design patterns](https://powershellexplained.com/2017-03-13-Powershell-DSL-design-patterns/)

[Approved Verbs for PowerShell Commands](https://docs.microsoft.com/en-us/powershell/scripting/developer/cmdlet/approved-verbs-for-windows-powershell-commands?view=powershell-7.1)