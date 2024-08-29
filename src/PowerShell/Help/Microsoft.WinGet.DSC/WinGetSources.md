---
external help file: Microsoft.WinGet.DSC.psm1-Help.xml
Module Name: Microsoft.WinGet.DSC
ms.date: 08/28/2024
online version:
schema: 2.0.0
title: WinGetSources
---

# WinGetSources

## SYNOPSIS
Configures WinGet sources.

## DESCRIPTION

Allows some or all of the WinGet sources to be configured or retrieved.

## PARAMETERS

**Parameter**|**Attribute**|**DataType**|**Description**|**Allowed Values**
:-----|:-----|:-----|:-----|:-----
`SID`|Key|String|The SID of the user to target, or an empty string to target the current user. **Only the empty string is currently supported.**|See [security identifiers](https://learn.microsoft.com/en-us/windows-server/identity/ad-ds/manage/understand-security-identifiers)
`Sources`|Mandatory|Hashtable[]|The source definitions as a hastable.|Each key in the `Sources` 
`Action`|Optional|WinGetAction|Determines whether to overwrite (Full) or merge (Partial) the value provided in `Settings` with the current user settings. The default behavior is Full.|Full, Partial
