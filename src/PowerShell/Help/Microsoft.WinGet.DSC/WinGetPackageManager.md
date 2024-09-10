---
external help file: Microsoft.WinGet.DSC.psm1-Help.xml
Module Name: Microsoft.WinGet.DSC
ms.date: 08/28/2024
online version:
schema: 2.0.0
title: WinGetPackageManager
---

# WinGetPackageManager

## SYNOPSIS
Configures the WinGet package manager package itself.

## DESCRIPTION

Allows the WinGet package manager package version to be configured. If multiple of the version targeting parameters are True/non-empty, they will take precedence in the following order: `UseLatest`, `UseLatestPreRelease`, `Version`.

## PARAMETERS

**Parameter**|**Attribute**|**DataType**|**Description**|**Allowed Values**
:-----|:-----|:-----|:-----|:-----
`SID`|Key|String|The SID of the user to target, or an empty string to target the current user. **Only the empty string is currently supported.**|See [security identifiers](https://learn.microsoft.com/en-us/windows-server/identity/ad-ds/manage/understand-security-identifiers)
`Version`|Optional|String|A version of the WinGet package manager package.|See [the WinGet release tags](https://github.com/microsoft/winget-cli/releases)
`UseLatest`|Optional|Boolean|The latest publicly available release should be used.|`True`, `False` (default)
`UseLatestPreRelease`|Optional|Boolean|The latest preview release should be used.|`True`, `False` (default)
