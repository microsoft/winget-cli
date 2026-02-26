---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Update-WinGetPackage
---

# Update-WinGetPackage

## SYNOPSIS
Installs a newer version of a previously installed WinGet package.

## SYNTAX

### FoundSet (Default)

```
Update-WinGetPackage [-IncludeUnknown] [-Mode <PSPackageInstallMode>] [-Override <String>] [-Custom <String>]
 [-Location <String>] [-Log <String>] [-Force] [-Header <String>] [-AllowHashMismatch]
 [-Architecture <PSProcessorArchitecture>] [-InstallerType <PSPackageInstallerType>] [-Locale <String>]
 [-Scope <PSPackageInstallScope>] [-SkipDependencies] [-Version <String>] [-Id <String>] [-Name <String>]
 [-Moniker <String>] [-Source <String>] [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>]
 [-WhatIf] [-Confirm] [<CommonParameters>]
```

### GivenSet

```
Update-WinGetPackage [-IncludeUnknown] [-Mode <PSPackageInstallMode>] [-Override <String>] [-Custom <String>]
 [-Location <String>] [-Log <String>] [-Force] [-Header <String>] [-AllowHashMismatch]
 [-Architecture <PSProcessorArchitecture>] [-InstallerType <PSPackageInstallerType>] [-Locale <String>]
 [-Scope <PSPackageInstallScope>] [-SkipDependencies] [[-PSCatalogPackage] <PSCatalogPackage>]
 [-Version <String>] [-WhatIf] [-Confirm] [<CommonParameters>]
```

## DESCRIPTION

This command searches the packages installed on your system and installs a newer version of the
matching WinGet package. The command includes parameters to specify values used to search for
packages in the configured sources. By default, the command searches all sources. By default, all string-based searches are case-insensitive substring searches. Wildcards are not supported. You can change the search behavior using the **MatchOption** parameter.

## EXAMPLES

### Example 1: Update a package using a query

```powershell
Update-WinGetPackage Microsoft.PowerShell
```

This example show how to update a package using a query. The **Query** parameter is positional, so
you don't need to include the parameter name before the query string.

### Example 2: Update a package by Id

```powershell
Update-WinGetPackage -Id Microsoft.PowerShell
```

This example shows how to update a package by the specifying the package identifier.
	
If the package identifier is available from more than one source, you must provide additional search
criteria to select a specific instance of the package.

### Example 3: Update a package by Name

```powershell
Update-WinGetPackage -Name "PowerToys (Preview)"
```

This sample updates the PowerToys package by the specifying the package name.

### Example 4: Update to a specific version of a package

```powershell
Update-WinGetPackage Microsoft.PowerShell -Version 7.4.4.0
```

This example shows how to update a specific version of a package using a query. The command does a
query search for packages matching `Microsoft.PowerShell`. The results of the search a limited to
matches with the version of `7.4.4.0`.

### Example 5: Update all packages

```powershell
Get-WinGetPackage | Where-Object IsUpdateAvailable | Update-WinGetPackage
```

This example shows how to update all packages that have an available upgrade from one of the configured sources.

## PARAMETERS

### -AllowHashMismatch

Allows you to download package even when the SHA256 hash for an installer or a dependency does not
match the SHA256 hash in the WinGet package manifest.

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

### -Architecture

Specify the processor architecture for the WinGet package installer. The parameter accepts the
following values:

- `Default`
- `X86`
- `Arm`
- `X64`
- `Arm64`

```yaml
Type: Microsoft.WinGet.Client.PSObjects.PSProcessorArchitecture
Parameter Sets: (All)
Aliases:
Accepted values: Default, X86, Arm, X64, Arm64

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Custom

Use this parameter to pass additional arguments to the installer. The parameter takes a single string value. To add multiple arguments, include the arguments in the string. The arguments must be provided in the format expected by the installer. If the string contains spaces, it must be enclosed in quotes. This string is added to the arguments defined in the package manifest.

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

### -Force

Force the update to run the installer.

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

### -Header

Specify a custom value in the HTTP header to the REST source.

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

### -IncludeUnknown

Use this parameter to upgrade the package when the installed version is not specified in the
registry.

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

### -InstallerType

A package may contain multiple installer types. Use this parameter to select the installer you want
to use. The parameter accepts the following values:
	
- `Default`
- `Inno`
- `Wix`
- `Msi`
- `Nullsoft`
- `Zip`
- `Msix`
- `Exe`
- `Burn`
- `MSStore`
- `Portable`

```yaml
Type: Microsoft.WinGet.Client.PSObjects.PSPackageInstallerType
Parameter Sets: (All)
Aliases:
Accepted values: Default, Inno, Wix, Msi, Nullsoft, Zip, Msix, Exe, Burn, MSStore, Portable

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Locale

Specify the locale of the installer package. The locale must provided in the BCP 47 format, such as
`en-US`. For more information, see
[Standard locale names](/globalization/locale/standard-locale-names).

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

### -Location

Specify the file path where you want the packed to be installed. The installer must be able to
support alternate install locations.

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

### -Log

Specify the location for the installer log. The value can be a fully-qualified or relative path and must include the file name. For example: `$env:TEMP\package.log`.

> **Note: Not all installers support this property.**

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

Not all installers support all modes.

```yaml
Type: Microsoft.WinGet.Client.PSObjects.PSPackageInstallMode
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

Specify the moniker of the WinGet package to update. For example, the moniker for the
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

Specify the name of the package to be updated.

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

### -Override

Use this parameter to override the existing arguments passed to the installer. The parameter takes a single string value. To add multiple arguments, include the arguments in the string. The arguments must be provided in the format expected by the installer. If the string contains spaces, it must be enclosed in quotes. This string overrides the arguments specified in the package manifest.

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

### -PSCatalogPackage

Provide **PSCatalogPackage** object. You can get a **PSCatalogPackage** object by using the
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

### -Scope

Specify WinGet package installer scope. The parameter accepts the following values:

- `Any`
- `User`
- `System`
- `UserOrUnknown`
- `SystemOrUnknown`

> [!NOTE]
> The installer scope must be available in the WinGet package manifest.

```yaml
Type: Microsoft.WinGet.Client.PSObjects.PSPackageInstallScope
Parameter Sets: (All)
Aliases:
Accepted values: Any, User, System, UserOrUnknown, SystemOrUnknown

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -SkipDependencies

Specifies that the command shouldn't install the WinGet package dependencies.

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

Specify package version.

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

### System.Management.Automation.SwitchParameter

### Microsoft.WinGet.Client.PSObjects.PSPackageInstallMode

### System.String

### Microsoft.WinGet.Client.PSObjects.PSProcessorArchitecture

### Microsoft.WinGet.Client.PSObjects.PSPackageInstallerType

### Microsoft.WinGet.Client.PSObjects.PSPackageInstallScope

### Microsoft.WinGet.Client.Engine.PSObjects.PSCatalogPackage

### System.String[]

### Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSInstallResult

## NOTES

## RELATED LINKS

[Get-WinGetPackage](Get-WinGetPackage.md)

[Uninstall-WinGetPackage](Uninstall-WinGetPackage.md)
