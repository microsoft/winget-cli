---
external help file: Microsoft.WinGet.DSC.psm1-Help.xml
Module Name: Microsoft.WinGet.DSC
ms.date: 08/28/2024
online version:
schema: 2.0.0
title: WinGetAdminSettings
---

# WinGetAdminSettings

## SYNOPSIS
Configures WinGet administrator settings.

## DESCRIPTION

Allows the administrator settings to be configured or retrieved. Setting administrator settings requires administrator privileges.

## PARAMETERS

**Parameter**|**Attribute**|**DataType**|**Description**|**Allowed Values**
:-----|:-----|:-----|:-----|:-----
`SID`|Key|String|The SID of the user to target, or an empty string to target the current user. **Only the empty string is currently supported.**|See [security identifiers](https://learn.microsoft.com/en-us/windows-server/identity/ad-ds/manage/understand-security-identifiers)
`Settings`|Mandatory|Hashtable|The administrator settings as a hashtable.|Inspect the `adminSettings` property of the output from `Get-WinGetSettings`
