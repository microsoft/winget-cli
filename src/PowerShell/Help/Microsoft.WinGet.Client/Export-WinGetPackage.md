---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Export-WinGetPackage
---

# Export-WinGetPackage

## SYNOPSIS
Downloads a WinGet package and its dependencies.

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

This command downloads a WinGet package from a configured source. The command downloads the package,
its manifest, as well as its dependencies and their manifests. By default, the packages are
downloaded to your `Downloads` folder. You can use the **DownloadDirectory** parameter to change
the location.

For Microsoft Azure users, if the package is coming from the Microsoft Store, the command also
downloads the package license file. The user needs to have one of the following Roles in Azure:

- Global Administrator
- User Administrator
- License Administrator

## EXAMPLES

### Example 1: Download Microsoft.PowerShell to the default location

```powershell
Export-WinGetPackage -Id Microsoft.PowerShell
```

```Output
Id                   Name       Source Status ExtendedErrorCode CorrelationData
--                   ----       ------ ------ ----------------- ---------------
Microsoft.PowerShell PowerShell winget Ok
```

```powershell
dir ~\Downloads\Microsoft.PowerShell_7.4.4.0
```

```Output
    Directory: C:\Users\user1\Downloads\Microsoft.PowerShell_7.4.4.0

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a---            8/7/2024  2:05 PM      108404736 PowerShell_7.4.4.0_Machine_X64_wix_en-US.msi
-a---            8/7/2024  2:05 PM           3838 PowerShell_7.4.4.0_Machine_X64_wix_en-US.yaml
```

This example shows how to download a package for the current machine and scope. For this example,
the command downloads the latest version of the installer package and manifest for
Microsoft.PowerShell. The command puts the files a new folder in your `Downloads` location. The
folder name is created by concatenating the package identifier and version number.

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

Specify the processor architecture for the WinGet package installer.

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

Specify the location where you want to store the downloaded files.

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

Specify the WinGet package identifier

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

Specify the type of installer package to download. This parameter accepts the following values:

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

> [!NOTE]
> The installer type must be available in the WinGet package manifest.

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

> [!NOTE]
> The locale must be available in the WinGet package manifest.

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

Specify the name of the WinGet package.

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
`Find-WinGetPackage` command.

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

One or more strings to search for in the WinGet package. The command searches for matching strings
in the following properties of the package manifest:

- `PackageName`
- `PackageIdentifier`
- `Moniker`
- `Tags`

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

Specifies that the command shouldn't download the WinGet package dependencies.

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

Specifies WinGet source.

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

Specifies the version of the WinGet package to download.

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
