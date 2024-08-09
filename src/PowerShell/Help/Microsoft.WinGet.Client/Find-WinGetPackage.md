---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Find-WinGetPackage
---

# Find-WinGetPackage

## SYNOPSIS
Searches for packages from configured sources.

## SYNTAX

```
Find-WinGetPackage [-Tag <String>] [-Command <String>] [-Count <UInt32>] [-Id <String>] [-Name <String>]
 [-Moniker <String>] [-Source <String>] [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>]
 [<CommonParameters>]
```

## DESCRIPTION

Searches for packages from configured sources.

## EXAMPLES

### Example 1: Search for PowerShell

```powershell
Find-WinGetPackage PowerShell
```

This example shows how to search for packages related to PowerShell. By default, the command
searches all configured sources. The command compares the value provided to the following package
manifest properties:

- `PackageIdentifier`
- `PackageName`
- `Moniker`
- `Tags`

The command does a case-insensitive substring comparison of these properties.

### Example 2: Search for Microsoft.PowerShell by id

```powershell
Find-WinGetPackage -Id Microsoft.PowerShell
```

This example shows how to search for packages by package identifier. By default, the command
searches all configured sources. The command performs a case-insensitive substring match against the
**PackageIdentifier** property of the packages.

### Example 3: Search for Microsoft.PowerShell by exact id

```powershell
Find-WinGetPackage -Id Microsoft.PowerShell -MatchOption Equals
```

This example shows how to search for packages by exact package identifier. By default, the command
searches all configured sources. The command performs a case-sensitive full text match against the
**PackageIdentifier** property of the packages.

## PARAMETERS

### -Command

Specify the name of the command defined in the package manifest.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Count

Specify the maximum number of results to return.

```yaml
Type: System.UInt32
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Id

Specify the package identifier to search for. By default, the command does a case-insensitive substring match.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -MatchOption

Specify matching logic used for search. The parameter accepts the following values:

- `Equals`
- `EqualsCaseInsensitive`
- `StartsWithCaseInsensitive`
- `ContainsCaseInsensitive`

```yaml
Type: Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption
Parameter Sets: (All)
Aliases:
Accepted values: Equals, EqualsCaseInsensitive, StartsWithCaseInsensitive, ContainsCaseInsensitive

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Moniker

Specify the moniker of the package you are searching for.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Name

Specify the name of the package to search for.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Query

Specify one or more strings to search for. By default, the command searches all configured sources.
The command compares the value provided to the following package manifest properties:
- `PackageIdentifier`
- `PackageName`
- `Moniker`
- `Tags`

The command does a case-insensitive substring comparison of these properties.

```yaml
Type: System.String[]
Parameter Sets: (All)
Aliases:

Required: False
Position: 0
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Source

Specify the name of the WinGet source to search. The most common sources are `msstore` and `winget`.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Tag

Specify a package tag to search for.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### CommonParameters

This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable,
-InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable,
-ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see
[about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.String

### System.UInt32

### System.String[]

### Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSFoundCatalogPackage

## NOTES

## RELATED LINKS

[Export-WinGetPackage](Export-WinGetPackage.md)

[Install-WinGetPackage](Install-WinGetPackage.md)
