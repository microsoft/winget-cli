---
external help file: Microsoft.WinGet.DSC.psm1-Help.xml
Module Name: Microsoft.WinGet.DSC
ms.date: 08/28/2024
online version:
schema: 2.0.0
title: WinGetSource
---

# WinGetSource

## SYNOPSIS
Configures a WinGet source.

## DESCRIPTION

Allows WinGet sources to be configured or retrieved.

## PARAMETERS

**Parameter**|**Attribute**|**DataType**|**Description**|**Allowed Values**
:-----|:-----|:-----|:-----|:-----
`Name`|Key, Mandatory|String|The name of the source.|Any string using valid characters for Windows file names
`Argument`|Mandatory|String|The primary data defining the source; typically the URI prefix of the source.|Source type specific value
`Type`|Optional|String|The type of the source.|`Microsoft.PreIndexed.Package`, `Microsoft.Rest`
`TrustLevel`|Optional|WinGetTrustLevel|The trust level of the source, which determines how much scrutiny is placed on it when used. If undefined and the source needs to be configured, the default value is `None`.|`Undefined` (default), `None`, `Trusted`
`Explicit`|Optional|WinGetOptionalBool|Whether the source is included (`False`) or not (`True`) in commands that do not specify a source. If undefined and the source needs to be configured, the default value is `False`.|`Undefined` (default), `False`, `True`
`Ensure`|Optional|WinGetEnsure|Whether the source should exist (`Present`) or not (`Absent`).|`Present` (default), `Absent`
