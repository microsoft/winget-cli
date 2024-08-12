﻿---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Get-WinGetUserSettings
---

# Get-WinGetUserSettings

## SYNOPSIS
Gets user settings for WinGet.

## SYNTAX

```
Get-WinGetUserSettings [<CommonParameters>]
```

## DESCRIPTION

This command displays the WinGet settings for the current user. The settings are stored in
`$env:LOCALAPPDATA\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\settings.json`.
This file only exists if you have changed a user setting, for example, using the `Set-WinGetUserSettings` command.

## EXAMPLES

### Example 1: Get the WinGet settings for the current user

```powershell
Get-WinGetUserSettings
```

## PARAMETERS

### CommonParameters

This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable,
-InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable,
-ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see
[about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### None

## OUTPUTS

### System.Collections.Hashtable

## NOTES

## RELATED LINKS

[Get-WinGetSettings](Get-WinGetSettings.md)

[Set-WinGetUserSettings](Set-WinGetUserSettings.md)
