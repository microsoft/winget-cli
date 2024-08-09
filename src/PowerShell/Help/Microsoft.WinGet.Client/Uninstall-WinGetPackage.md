---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Uninstall-WinGetPackage
---

# Uninstall-WinGetPackage

## SYNOPSIS
Uninstalls a WinGet Package.

## SYNTAX

### FoundSet (Default)

```
Uninstall-WinGetPackage [-Mode <PSPackageUninstallMode>] [-Force] [-Log <String>] [-Version <String>]
 [-Id <String>] [-Name <String>] [-Moniker <String>] [-Source <String>] [[-Query] <String[]>]
 [-MatchOption <PSPackageFieldMatchOption>] [-WhatIf] [-Confirm] [<CommonParameters>]
```

### GivenSet

```
Uninstall-WinGetPackage [-Mode <PSPackageUninstallMode>] [-Force] [-Log <String>]
 [[-PSCatalogPackage] <PSCatalogPackage>] [-Version <String>] [-WhatIf]
 [-Confirm] [<CommonParameters>]
```

## DESCRIPTION

This command uninstalls a WinGet package from your computer. The command includes parameters to
specify values used to search for installed packages. By default, all string-based searches are
case-insensitive substring searches. Wildcards are not supported. You can change the search behavior
using the **MatchOption** parameter.

## EXAMPLES

### Example 1: Uninstall a package using a query

```powershell
Uninstall-WinGetPackage Microsoft.PowerShell
```

This example show how to uninstall a package using a query. The **Query** parameter is positional,
so you don't need to include the parameter name before the query string.

### Example 2: Uninstall a package by Id

```powershell
Uninstall-WinGetPackage -Id Microsoft.PowerShell
```

This example shows how to uninstall a package by the specifying the package identifier.

If the package identifier is available from more than one source, you must provide additional search
criteria to select a specific instance of the package.

### Example 3: Uninstall a package by Name

```powershell
Uninstall-WinGetPackage -Name "PowerToys (Preview)"
```

This sample uninstalls the PowerToys package by the specifying the package name.

### Example 4: Uninstall a specific version of a package

```powershell
Uninstall-WinGetPackage Microsoft.PowerShell -Version 7.4.4.0
```

This example shows how to uninstall a specific version of a package using a query. The command does
a query search for packages matching `Microsoft.PowerShell`. The results of the search a limited to
matches with the version of `7.4.4.0`.

## PARAMETERS

### -Force

Force the uninstall to run.

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

### -Log

Specify the location for the uninstaller log. The value can be a fully-qualified or relative path and must include the file name. For example: `$env:TEMP\package.log`.

> **Note: Not all uninstallers support this option.**

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

Specify the match option for a WinGet package query. This parameter accepts the following values:

- `Equals`
- `EqualsCaseInsensitive`
- `StartsWithCaseInsensitive`
- `ContainsCaseInsensitive`

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

### -Mode

Specify the output mode for the installer. The parameter accepts the following values:
	
- `Default`
- `Silent`
- `Interactive`

> [!NOTE]
> Not all uninstallers support all modes.

```yaml
Type: Microsoft.WinGet.Client.PSObjects.PSPackageUninstallMode
Parameter Sets: (All)
Aliases:
Accepted values: Default, Silent, Interactive

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Moniker

Specify the moniker of the WinGet package to download. For example, the moniker for the
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

Specify the name of the WinGet package name. The name contains space, you must enclose the name in
quotes.

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

Provide **PSCatalogPackage** object. You can get a **PSCatalogPackage** object by using the
`Find-WinGetPackage` or `Get-WingetPackage` commands.

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

### -Version

Specify the version of the package to be uninstalled.

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

### -WhatIf

Shows what would happen if the cmdlet runs. The cmdlet isn't run.

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

This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable,
-InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable,
-ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see
[about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### Microsoft.WinGet.Client.PSObjects.PSPackageUninstallMode

### System.Management.Automation.SwitchParameter

### System.String

### Microsoft.WinGet.Client.Engine.PSObjects.PSCatalogPackage

### System.String[]

### Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSUninstallResult

## NOTES

## RELATED LINKS

[Get-WinGetPackage](Get-WinGetPackage.md)

[Install-WinGetPackage](Install-WinGetPackage.md)
