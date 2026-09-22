---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
---

# Get-WinGetPin

## SYNOPSIS
Gets WinGet package pins.

## SYNTAX

### AllSet (Default)
```
Get-WinGetPin [<CommonParameters>]
```

### GivenSet
```
Get-WinGetPin [[-PSCatalogPackage] <PSCatalogPackage>] [<CommonParameters>]
```

### FoundSet
```
Get-WinGetPin [-Id <String>] [-Name <String>] [-Moniker <String>] [-Source <String>]
 [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>] [<CommonParameters>]
```

## DESCRIPTION
This command retrieves WinGet package pins. When called without parameters, it returns all pins.
When called with package search criteria or a catalog package, it returns pins for the matching
package. By default, all string-based searches are case-insensitive substring searches.
Wildcards are not supported. You can change the search behavior using the **MatchOption** parameter.

## EXAMPLES

### Example 1: Get all pins
```powershell
Get-WinGetPin
```
This example gets all package pins configured in WinGet.

### Example 2: Get the pin for a specific package by Id
```powershell
Get-WinGetPin -Id "Microsoft.PowerShell"
```
This example gets the pin for the package with the identifier `Microsoft.PowerShell`.

### Example 3: Get the pin for a package using a query
```powershell
Get-WinGetPin -Query "PowerShell"
```
This example gets pins for packages matching the query `PowerShell`.

### Example 4: Get the pin for an installed package using the pipeline
```powershell
Get-WinGetPackage -Id "Microsoft.PowerShell" | Get-WinGetPin
```
This example gets the pin for an installed package passed from `Get-WinGetPackage`.

## PARAMETERS

### -Id

Specify the package identifier to search for. By default, the command does a case-insensitive
substring match.

```yaml
Type: System.String
Parameter Sets: FoundSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -MatchOption

Specify the match option for a WinGet package query. This parameter accepts the following values:

```yaml
Type: Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption
Parameter Sets: FoundSet
Aliases:
Accepted values: Equals, EqualsCaseInsensitive, StartsWithCaseInsensitive, ContainsCaseInsensitive

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Moniker

Specify the moniker of the WinGet package to search for. For example, the moniker for the
Microsoft.PowerShell package is `pwsh`.

```yaml
Type: System.String
Parameter Sets: FoundSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Name

Specify the name of the WinGet package to search for. If the name contains spaces, enclose the
name in quotes.

```yaml
Type: System.String
Parameter Sets: FoundSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -PSCatalogPackage

Provide a **PSCatalogPackage** object. You can get a **PSCatalogPackage** object by using the
`Find-WinGetPackage` or `Get-WinGetPackage` commands.

```yaml
Type: Microsoft.WinGet.Client.Engine.PSObjects.PSCatalogPackage
Parameter Sets: GivenSet
Aliases: InputObject

Required: False
Position: 0
Default value: None
Accept pipeline input: True (ByPropertyName, ByValue)
Accept wildcard characters: False
```

### -Query

Specify one or more strings to search for. By default, the command searches all configured sources.
Wildcards are not supported. The command compares the value provided to the following package
manifest properties:

	- `PackageIdentifier`
	- `PackageName`
	- `Moniker`
	- `Tags`

The command does a case-insensitive substring comparison of these properties.

```yaml
Type: System.String[]
Parameter Sets: FoundSet
Aliases:

Required: False
Position: 0
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Source

Specify the name of a configured WinGet source.

```yaml
Type: System.String
Parameter Sets: FoundSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### CommonParameters
This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable, -InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable, -Verbose, -WarningAction, and -WarningVariable. For more information, see [about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSCatalogPackage

### System.String

### System.String[]

### Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSPackagePin

## NOTES

## RELATED LINKS

[Add-WinGetPin](Add-WinGetPin.md)

[Remove-WinGetPin](Remove-WinGetPin.md)

[Reset-WinGetPin](Reset-WinGetPin.md)

[Find-WinGetPackage](Find-WinGetPackage.md)

[Get-WinGetPackage](Get-WinGetPackage.md)
