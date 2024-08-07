﻿---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Add-WinGetSource
---

# Add-WinGetSource

## SYNOPSIS
Adds a new source.

## SYNTAX

```
Add-WinGetSource -Name <String> -Argument <String> [-Type <String>] [<CommonParameters>]
```

## DESCRIPTION

Adds a new source. A source provides the data for you to discover and install packages. Only add a
new source if you trust it as a secure location.

## EXAMPLES

### Example 1: Add new REST source named mysource

```powershell
Add-WinGetSource -Name mysource -Argument https://contoso.com/ -Type Microsoft.Rest
```

This example adds a new REST source to WinGet named `mysource` with the root URL
`https://contoso.com/`. The source must respond with the WinGet REST source API.

## PARAMETERS

### -Argument

The URL or UNC of WinGet source. The source must respond with the WinGet REST source API.

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

### -Name

The name used to identify the WinGet source.

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

### -Type

The type of the WinGet source. Most sources are `Microsoft.Rest`. The WinGet community repository
is `Microsoft.PreIndexed.Package`.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
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

[Remove-WinGetSource](Remove-WinGetSource.md)

[Reset-WinGetSource](Reset-WinGetSource.md)