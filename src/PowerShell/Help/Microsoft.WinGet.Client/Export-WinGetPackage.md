﻿---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Export-WinGetPackage

## SYNOPSIS
Downloads a package.

## SYNTAX

### FoundSet (Default)
```
Export-WinGetPackage [-DownloadDirectory <String>] [-AllowHashMismatch]
 [-Architecture <PSProcessorArchitecture>] [-InstallerType <PSPackageInstallerType>] [-Locale <String>]
 [-Scope <PSPackageInstallScope>] [-SkipDependencies] [-Version <String>] [-Id <String>] [-Name <String>]
 [-Moniker <String>] [-Source <String>] [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>]
 [-WhatIf] [-Confirm] [<CommonParameters>]
```

### GivenSet
```
Export-WinGetPackage [-DownloadDirectory <String>] [-AllowHashMismatch]
 [-Architecture <PSProcessorArchitecture>] [-InstallerType <PSPackageInstallerType>] [-Locale <String>]
 [-Scope <PSPackageInstallScope>] [-SkipDependencies] [[-PSCatalogPackage] <PSCatalogPackage>]
 [-Version <String>] [-WhatIf] [-Confirm] [<CommonParameters>]
```

## DESCRIPTION
Downloads a package installer from the pipeline or from a configured source. The package manifest as well as dependencies and their manifests will be included. The default directory if not specified is the Downloads folder.

## EXAMPLES

### Example 1: Download Microsoft.PowerShell to the default location
```powershell
PS C:\> Export-WinGetPackage -Id Microsoft.PowerShell
```

This example will download the latest available version of the Microsoft.PowerShell installer (for the current system and default scope) and the manifest to a folder (named by concatenating the package identifier with an underscore and the version number) in the Downloads directory.

## PARAMETERS

### -AllowHashMismatch
Allow the download even if the SHA256 hash does not match

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
Specify the architecture for the installer

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

### -DownloadDirectory
Specify the directory for the download

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
Specify the package identifier

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

### -InstallerType
Specify the installer type

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
Specify the installer locale

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
Specify the match option for a package query

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
Specify the moniker

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
Specify the package name

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
Specify PSCatalogPackage

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
Search string for package

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
Specify installer scope

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
Specify main package without dependencies

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
Specify WinGet source

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
Specify package version

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
Shows what would happen if the cmdlet runs.
The cmdlet is not run.

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
This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable, -InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable, -ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see [about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.String

### System.Management.Automation.SwitchParameter

### Microsoft.WinGet.Client.PSObjects.PSProcessorArchitecture

### Microsoft.WinGet.Client.PSObjects.PSPackageInstallerType

### Microsoft.WinGet.Client.PSObjects.PSPackageInstallScope

### Microsoft.WinGet.Client.Engine.PSObjects.PSCatalogPackage

### System.String[]

### Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSDownloadResult

## NOTES

## RELATED LINKS

[Find-WinGetPackage](Find-WinGetPackage.md)
