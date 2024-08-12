---
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

This command removes a configured WinGet source. By default, there are two sources registered:
`msstore` and `winget`. You can add more sources using `Add-WinGetSource`. This command must be executed with administrator permissions.

## EXAMPLES

### Example 1: Remove a single source by name

```powershell
Remove-WinGetSource -Name msstore
```

The example shows how to remove a WinGet source by name.

## PARAMETERS

### -Name

Specify the name of the source to be removed.

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
