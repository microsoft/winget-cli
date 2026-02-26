---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Reset-WinGetSource
---

# Reset-WinGetSource

## SYNOPSIS
Resets WinGet sources.

## SYNTAX

### DefaultSet (Default)

```
Reset-WinGetSource -Name <String> [<CommonParameters>]
```

### OptionalSet

```
Reset-WinGetSource -All [<CommonParameters>]
```

## DESCRIPTION

Resets a named WinGet source by removing the source configuration. You can reset all configured sources and add the default source configurations using the **All** switch parameter.
This command must be executed with administrator permissions.

## EXAMPLES

### Example 1: Reset the msstore source

```powershell
Reset-WinGetSource -Name msstore
```

This example resets the configured source named 'msstore' by removing it.

### Example 2: Reset all sources

```powershell
Reset-WinGetSource -All
```

This example resets all configured sources and adds the default sources.

## PARAMETERS

### -All

Reset all sources and add the default sources.

```yaml
Type: System.Management.Automation.SwitchParameter
Parameter Sets: (OptionalSet)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Name

The name of the source.

```yaml
Type: System.String
Parameter Sets: (DefaultSet)
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

[Remove-WinGetSource](Remove-WinGetSource.md)
