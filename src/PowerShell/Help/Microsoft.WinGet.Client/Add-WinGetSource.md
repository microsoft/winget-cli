﻿---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Add-WinGetSource

## SYNOPSIS
Add a new source.

## SYNTAX

```
Add-WinGetSource -Name <String> -Argument <String> [-Type <String>] [<CommonParameters>]
```

## DESCRIPTION
Add a new WinGet source.

## EXAMPLES

### Example 1: Add new REST source named mysource

```powershell
PS C:\> Add-WinGetSource -Name mysource -Argument https://contoso.com/ -Type Microsoft.Rest
```

This example adds a new REST based source to WinGet named "mysource" with the root URL https://contoso.com/.

> **This example is illustrative and will not succeed because WinGet will not add a source that does not properly respond to the WinGet REST API**

## PARAMETERS

### -Argument

The URL or UNC of the source.

> **Before a source can be added, WinGet will verify the URL or UNC is reachable and provides the proper response.**

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
The name to identify the source by.

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
Type of the source. Most sources will be "Microsoft.Rest". The WinGet community repository is "Microsoft.PreIndexed.Package".

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
This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable, -InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable, -ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see [about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.String

## OUTPUTS

### System.Object
## NOTES

## RELATED LINKS

[Remove-WinGetSource](Remove-WinGetSource.md)

[Reset-WinGetSource](Reset-WinGetSource.md)
