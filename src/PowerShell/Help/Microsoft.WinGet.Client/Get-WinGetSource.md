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

Lists the configured WinGet sources.

## EXAMPLES

### Example 1: List all configured WinGet sources

```powershell
Get-WinGetSource
```

```Output
Name    Argument                                      Type
----    --------                                      ----
msstore https://storeedgefd.dsx.mp.microsoft.com/v9.0 Microsoft.Rest
winget  https://cdn.winget.microsoft.com/cache        Microsoft.PreIndexed.Package
```

## PARAMETERS

### -Name

Specify the name of the WinGet source to be displayed.

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
