﻿---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Remove-WinGetSource
---

# Remove-WinGetSource

## SYNOPSIS
Removes a configured source.

## SYNTAX

```
Remove-WinGetSource -Name <String> [<CommonParameters>]
```

## DESCRIPTION

Removes a configured source.

## EXAMPLES

### Example 1: Remove a single source by name

```powershell
Remove-WinGetSource -Name msstore
```

Removes the "msstore" WinGet source.

> [!NOTE]
> You can run Reset-WinGetSource to re-add the default sources back to WinGet.

## PARAMETERS

### -Name

The name of the source.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: True
Position: Named
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

### System.Object

## NOTES

## RELATED LINKS

[Add-WinGetSource](Add-WinGetSource.md)

[Reset-WinGetSource](Reset-WinGetSource.md)