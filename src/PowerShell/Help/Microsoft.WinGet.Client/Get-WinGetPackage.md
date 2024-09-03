---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Get-WinGetPackage
---

# Get-WinGetPackage

## SYNOPSIS
Lists installed packages.

## SYNTAX

```
Get-WinGetPackage [-Tag <String>] [-Command <String>] [-Count <UInt32>] [-Id <String>] [-Name <String>]
 [-Moniker <String>] [-Source <String>] [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>]
 [<CommonParameters>]
```

## DESCRIPTION

This command lists all of the packages installed on your system. The output includes packages
installed from WinGet sources and packages installed by other methods. Packages that have package
identifiers starting with `MSIX` or `ARP` could not be correlated to a WinGet source.

## EXAMPLES

### Example 1: Default example

```powershell
Get-WinGetPackage
```

This example shows how to list all packages installed on your system.

### Example 2: Get package by Id

```powershell
Get-WinGetPackage -Id "Microsoft.PowerShell"
```

This example shows how to get an installed package by its package identifier.

### Example 3: Get package(s) by name

```powershell
Get-WinGetPackage -Name "PowerShell"
```

This example shows how to get installed packages that match a name value. The command does a substring comparison of the provided name with
installed package names.

### Example 4: List all packages with an available update

```powershell
Get-WinGetPackage | Where-Object IsUpdateAvailable
```

This example shows how to list all packages that have an available upgrade from one of the configured sources.

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

Limits the number of items returned by the command.

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

Specify the package identifier for the package you want to list. By default, the command does a
case-insensitive substring comparison on the package identifier.

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

Specify the moniker of the package you want to list.

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

Specify the name of the package to list. By default, the command does a case-insensitive comparison
of the package name.

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

Specify the name of the WinGet source of the package.

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

### Microsoft.WinGet.Client.Engine.PSObjects.PSInstalledCatalogPackage

## NOTES

## RELATED LINKS

[Uninstall-WinGetPackage](Uninstall-WinGetPackage.md)

[Update-WinGetPackage](Update-WinGetPackage.md)
