---
author: Demitrius Nelon @denelon
created on: 2021-08-06
last updated: 2021-08-06
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
>Note: Naming things is hard.

### `winget install`

Install-WinGetPackage

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

### `winget show`

Get-WinGetPackage

### `winget source`

Add-WinGetSource

Remove-WinGetSource

### `winget search`

Get-WinGetPackage

### `winget list`

Get-WinGetInstalledPackages

### `winget upgrade`

Update-WinGetPackage

### `winget uninstall`

Uninstall-WinGetPackage

### `winget hash`

Get-WinGetPackageSha256

Get-WinGetSignatureSha256

### `winget validate`

Test-WinGetManifest

### `winget settings`

Set-WinGetSettings

### `winget features`

Get-WinGetFeatures

### `winget export`

Export-WinGetPackages

### `winget import`

Import-WinGetPackages

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