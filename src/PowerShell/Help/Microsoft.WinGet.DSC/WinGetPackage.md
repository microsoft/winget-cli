---
external help file: Microsoft.WinGet.DSC.psm1-Help.xml
Module Name: Microsoft.WinGet.DSC
ms.date: 08/28/2024
online version:
schema: 2.0.0
title: WinGetPackage
---

# WinGetPackage

## SYNOPSIS
Configures packages through WinGet.

## DESCRIPTION

Allows WinGet package state to be configured.

## PARAMETERS

**Parameter**|**Attribute**|**DataType**|**Description**|**Allowed Values**
:-----|:-----|:-----|:-----|:-----
`Id`|Key, Mandatory|String|The identifier of a WinGet package.|Use `Find-WinGetPackage` to search for packages
`Source`|Key|String|The name of the source that provides the package. If not provided, all configured sources will be searched for the `Id`.|Use the `WinGetSources` resource to configure a source or `Get-WinGetSource` to discover the default sources
`Version`|Optional|String|The version of the package.|See the `AvailableVersions` property of output from `Find-WinGetPackage`
`Ensure`|Optional|WinGetEnsure|Whether the package should be installed (`Present`) or not (`Absent`).|`Present` (default), `Absent`
`MatchOption`|Optional|WinGetMatchOption|The method used to compare the `Id` parameter with available packages.|`Equals`, `EqualsCaseInsensitive` (default), `StartsWithCaseInsensitive`, `ContainsCaseInsensitive`
`UseLatest`|Optional|Boolean|Whether the package should updated to the latest available version. If true, takes precedence over `Version`.|`True`, `False` (default)
`InstallMode`|Optional|WinGetInstallMode|The interactivity level requested when installing.|`Default`, `Silent` (default), `Interactive`
