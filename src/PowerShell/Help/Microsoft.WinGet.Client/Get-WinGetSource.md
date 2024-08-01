---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Get-WinGetSource
---

# Get-WinGetSource

## SYNOPSIS
Lists configured WinGet sources.

## SYNTAX

```
Get-WinGetSource [[-Name] <String>] [<CommonParameters>]
```

## DESCRIPTION

List configured WinGet sources.

## EXAMPLES

### Example 1: Default example

```powershell
Get-WinGetSource
```

This will list the configured WinGet sources. The two default sources are "msstore" and "winget".

## PARAMETERS

### -Name

Filter by WinGet source name

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
Position: 0
Default value: None
Accept pipeline input: True (ByPropertyName, ByValue)
Accept wildcard characters: False
```

### CommonParameters

This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable,
-InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable,
-ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see
[about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.String

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSSourceResult

## NOTES

## RELATED LINKS

[Add-WinGetSource](Add-WinGetSource.md)

[Remove-WinGetSource](Remove-WinGetSource.md)
