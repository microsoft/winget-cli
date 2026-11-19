---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
---

# Remove-WinGetPin

## SYNOPSIS
Removes a WinGet package pin.

## SYNTAX

### FoundSet (Default)
```
Remove-WinGetPin [-Id <String>] [-Name <String>] [-Moniker <String>] [-Source <String>]
 [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>] [-ProgressAction <ActionPreference>]
 [-WhatIf] [-Confirm] [<CommonParameters>]
```

### GivenSet
```
Remove-WinGetPin [[-PSCatalogPackage] <PSCatalogPackage>] [-ProgressAction <ActionPreference>]
 [-WhatIf] [-Confirm] [<CommonParameters>]
```

### PinSet
```
Remove-WinGetPin [[-PSPackagePin] <PSPackagePin>] [-ProgressAction <ActionPreference>]
 [-WhatIf] [-Confirm] [<CommonParameters>]
```

## DESCRIPTION
This command removes a pin for a WinGet package. Mirrors the behavior of `winget pin remove`.
The command accepts a **PSPackagePin** object from the pipeline (e.g., from `Get-WinGetPin`),
a **PSCatalogPackage** object from `Get-WinGetPackage` or `Find-WinGetPackage`, or package
search criteria. By default, all string-based searches are case-insensitive exact matches.
Wildcards are not supported. You can change the search behavior using the **MatchOption** parameter.

## EXAMPLES

### Example 1: Remove a pin by package Id
```powershell
Remove-WinGetPin -Id "Microsoft.PowerShell"
```
This example removes the pin for the package with the identifier `Microsoft.PowerShell`.

### Example 2: Remove a pin using the pipeline from Get-WinGetPin
```powershell
Get-WinGetPin -Id "Microsoft.PowerShell" | Remove-WinGetPin
```
This example removes a pin passed from `Get-WinGetPin`.

### Example 3: Remove all pins
```powershell
Get-WinGetPin | Remove-WinGetPin
```
This example removes all pins by piping the output of `Get-WinGetPin` to `Remove-WinGetPin`.

### Example 4: Remove a pin using the pipeline from Get-WinGetPackage
```powershell
Get-WinGetPackage -Id "Microsoft.PowerShell" | Remove-WinGetPin
```
This example removes the pin for an installed package passed from `Get-WinGetPackage`.

## PARAMETERS

### -Confirm

Prompts you for confirmation before running the cmdlet.

```yaml
Type: System.Management.Automation.SwitchParameter
Parameter Sets: (All)
Aliases: cf

Required: False
Position: Named
Default value: None
Accept pipeline input: False
Accept wildcard characters: False
```

### -Id

Specify the package identifier to search for. By default, the command does a case-insensitive
exact match.

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
Default value: EqualsCaseInsensitive
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

### -PSPackagePin

Provide a **PSPackagePin** object. You can get a **PSPackagePin** object by using the
`Get-WinGetPin` command.

```yaml
Type: Microsoft.WinGet.Client.Engine.PSObjects.PSPackagePin
Parameter Sets: PinSet
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

### -WhatIf

Shows what would happen if the cmdlet runs. The cmdlet is not run.

```yaml
Type: System.Management.Automation.SwitchParameter
Parameter Sets: (All)
Aliases: wi

Required: False
Position: Named
Default value: None
Accept pipeline input: False
Accept wildcard characters: False
```

### CommonParameters
This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable, -InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable, -Verbose, -WarningAction, and -WarningVariable. For more information, see [about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSPackagePin

### Microsoft.WinGet.Client.Engine.PSObjects.PSCatalogPackage

### System.String

### System.String[]

### Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSPinResult

## NOTES

## RELATED LINKS

[Get-WinGetPin](Get-WinGetPin.md)

[Add-WinGetPin](Add-WinGetPin.md)

[Reset-WinGetPin](Reset-WinGetPin.md)

[Find-WinGetPackage](Find-WinGetPackage.md)

[Get-WinGetPackage](Get-WinGetPackage.md)
