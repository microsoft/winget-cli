---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Find-WinGetPackage

## SYNOPSIS
Searches configured sources for packages.

## SYNTAX

```
Find-WinGetPackage [-Tag <String>] [-Command <String>] [-Count <UInt32>] [-Id <String>] [-Name <String>]
 [-Moniker <String>] [-Source <String>] [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>]
 [<CommonParameters>]
```

## DESCRIPTION

Searches configured sources for packages.

## EXAMPLES

### Example 1: Search for PowerShell

```powershell
Find-WinGetPackage PowerShell
```

Searches for PowerShell. The default behavior spans all configured sources and performs a
case-insensitive substring match across package identifier, package name, package moniker, and
package tags.

### Example 2: Search for Microsoft.PowerShell by id

```powershell
Find-WinGetPackage -Id Microsoft.PowerShell
```

Search for Microsoft.PowerShell by package identifier. The default behavior spans all configured
sources and performs a case-insensitive substring match across package identifiers.

### Example 3: Search for Microsoft.PowerShell by exact id

```powershell
Find-WinGetPackage -Id Microsoft.PowerShell -MatchOption Equals
```

Search for Microsoft.PowerShell by exact package identifier. This search spans all configured
sources and performs a case-sensitive match across package identifiers.

## PARAMETERS

### -Command

Specifies command used to run the package

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

Specifies maximum number of results to return

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

Specifies search by package identifier

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

Specifies matching logic for search

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

Specifies package moniker

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

Specifies package name

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

Specifies query string to search for

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

Specifies WinGet source for search

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

Specifies tag to search for

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
