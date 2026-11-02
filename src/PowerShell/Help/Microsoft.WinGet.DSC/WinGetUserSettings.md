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

By default this resource manages the ordinary user settings file. Set `Admin` to `$true`
to manage the protected per-user administrator settings file. Get and Test do not require
elevation; Set requires elevation and creates the protected file if needed. This manages
raw file contents, not the effective overlay or the capability controls managed by
`WinGetAdminSettings`. Partial updates retain the existing merge behavior, which ignores
input nulls; use `Full` to write an explicit null.

## PARAMETERS

**Parameter**|**Attribute**|**DataType**|**Description**|**Allowed Values**
:-----|:-----|:-----|:-----|:-----
`SID`|Key|String|The SID of the user to target, or an empty string to target the current user. **Only the empty string is currently supported.**|See [security identifiers](https://learn.microsoft.com/en-us/windows-server/identity/ad-ds/manage/understand-security-identifiers)
`Settings`|Mandatory|Hashtable|The user settings as a hashtable.|See [the schema](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/settings/settings.schema.0.2.json)
`Action`|Optional|WinGetAction|Determines whether to overwrite (`Full`) or merge (`Partial`) the value provided in `Settings` with the current user settings.|`Full` (default), `Partial`
`Admin`|Optional|Boolean|Selects the protected administrator settings file. Mutations require administrator privileges.|`$false` (default), `$true`
