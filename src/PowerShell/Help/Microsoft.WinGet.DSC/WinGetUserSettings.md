---
external help file: Microsoft.WinGet.DSC.psm1-Help.xml
Module Name: Microsoft.WinGet.DSC
ms.date: 08/28/2024
online version:
schema: 2.0.0
title: WinGetUserSettings
---

# WinGetUserSettings

## SYNOPSIS
Configures WinGet user settings.

## DESCRIPTION

Allows part or all of the user settings to be configured or retrieved.

## PARAMETERS

**Parameter**|**Attribute**|**DataType**|**Description**|**Allowed Values**
:-----|:-----|:-----|:-----|:-----
`SID`|Key|String|The SID of the user to target, or an empty string to target the current user. **Only the empty string is currently supported.**|See [security identifiers](https://learn.microsoft.com/en-us/windows-server/identity/ad-ds/manage/understand-security-identifiers)
`Settings`|Mandatory|Hashtable|The user settings as a hashtable.|See [the schema](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/settings/settings.schema.0.2.json)
`Action`|Optional|WinGetAction|Determines whether to overwrite (`Full`) or merge (`Partial`) the value provided in `Settings` with the current user settings.|`Full` (default), `Partial`
