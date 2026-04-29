---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
---

# Add-WinGetPin

## SYNOPSIS
Adds a WinGet package pin.

## SYNTAX

### FoundSet (Default)
```
Add-WinGetPin [-PinType <PSPackagePinType>] [-GatedVersion <String>] [-PinInstalledPackage]
 [-Force] [-Note <String>] [-Id <String>] [-Name <String>] [-Moniker <String>] [-Source <String>]
 [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>] [-ProgressAction <ActionPreference>]
 [-WhatIf] [-Confirm] [<CommonParameters>]
```

### GivenSet
```
Add-WinGetPin [-PinType <PSPackagePinType>] [-GatedVersion <String>] [-PinInstalledPackage]
 [-Force] [-Note <String>] [[-PSCatalogPackage] <PSCatalogPackage>]
 [-ProgressAction <ActionPreference>] [-WhatIf] [-Confirm] [<CommonParameters>]
```

## DESCRIPTION
This command adds a pin for a WinGet package, preventing automatic updates. Mirrors the behavior
of `winget pin add`. By default, all string-based searches are case-insensitive exact matches.
Wildcards are not supported. You can change the search behavior using the **MatchOption** parameter.

Pin types:
- **Pinning** (default): Prevents automatic updates but allows manual upgrades.
- **Blocking**: Prevents all upgrades, including manual ones.
- **Gating**: Allows upgrades only to versions below the specified **GatedVersion**.

## EXAMPLES

### Example 1: Pin a package by Id
```powershell
Add-WinGetPin -Id "Microsoft.PowerShell"
```
This example adds a Pinning pin for `Microsoft.PowerShell`, preventing automatic updates.

### Example 2: Add a blocking pin
```powershell
Add-WinGetPin -Id "Microsoft.PowerShell" -PinType Blocking
```
This example adds a Blocking pin for `Microsoft.PowerShell`, preventing all upgrades.

### Example 3: Add a gating pin
```powershell
Add-WinGetPin -Id "Microsoft.PowerShell" -PinType Gating -GatedVersion "<7.5"
```
This example adds a Gating pin that allows upgrades only to versions below 7.5.

### Example 4: Pin a package using the pipeline
```powershell
Find-WinGetPackage -Id "Microsoft.PowerShell" | Add-WinGetPin
```
This example pins a package passed from `Find-WinGetPackage`.

### Example 5: Pin an installed package
```powershell
Get-WinGetPackage -Id "Microsoft.PowerShell" | Add-WinGetPin -PinInstalledPackage
```
This example adds a pin for the currently installed version of `Microsoft.PowerShell`.

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

### -Force

Forces the pin operation even if an existing pin is already present.

```yaml
Type: System.Management.Automation.SwitchParameter
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -GatedVersion

Specify the version range for a Gating pin. This parameter is required when **PinType** is
`Gating`. The value uses WinGet version range syntax (e.g., `<7.5`, `>=7.0,<8.0`).

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

### -Note

Specify an optional note to attach to the pin.

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

### -PinInstalledPackage

When specified, pins the currently installed version of the package rather than all versions.

```yaml
Type: System.Management.Automation.SwitchParameter
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -PinType

Specify the type of pin to add. Accepted values:

- **Pinning** (default): Prevents automatic upgrades.
- **Blocking**: Prevents all upgrades.
- **Gating**: Limits upgrades to versions below **GatedVersion**.

```yaml
Type: Microsoft.WinGet.Client.PSObjects.PSPackagePinType
Parameter Sets: (All)
Aliases:
Accepted values: Pinning, Blocking, Gating

Required: False
Position: Named
Default value: Pinning
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

### Microsoft.WinGet.Client.Engine.PSObjects.PSCatalogPackage

### System.String

### System.String[]

### Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption

### Microsoft.WinGet.Client.PSObjects.PSPackagePinType

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSPinResult

## NOTES

## RELATED LINKS

[Get-WinGetPin](Get-WinGetPin.md)

[Remove-WinGetPin](Remove-WinGetPin.md)

[Reset-WinGetPin](Reset-WinGetPin.md)

[Find-WinGetPackage](Find-WinGetPackage.md)

[Get-WinGetPackage](Get-WinGetPackage.md)
