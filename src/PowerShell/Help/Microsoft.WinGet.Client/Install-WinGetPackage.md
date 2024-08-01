---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Install-WinGetPackage

## SYNOPSIS
Installs a WinGet Package.

## SYNTAX

### FoundSet (Default)
```
Install-WinGetPackage [-Mode <PSPackageInstallMode>] [-Override <String>] [-Custom <String>]
 [-Location <String>] [-Log <String>] [-Force] [-Header <String>] [-AllowHashMismatch]
 [-Architecture <PSProcessorArchitecture>] [-InstallerType <PSPackageInstallerType>] [-Locale <String>]
 [-Scope <PSPackageInstallScope>] [-SkipDependencies] [-Version <String>] [-Id <String>] [-Name <String>]
 [-Moniker <String>] [-Source <String>] [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>]
 [-WhatIf] [-Confirm] [<CommonParameters>]
```

### GivenSet
```
Install-WinGetPackage [-Mode <PSPackageInstallMode>] [-Override <String>] [-Custom <String>]
 [-Location <String>] [-Log <String>] [-Force] [-Header <String>] [-AllowHashMismatch]
 [-Architecture <PSProcessorArchitecture>] [-InstallerType <PSPackageInstallerType>] [-Locale <String>]
 [-Scope <PSPackageInstallScope>] [-SkipDependencies] [[-PSCatalogPackage] <PSCatalogPackage>]
 [-Version <String>] [-WhatIf] [-Confirm] [<CommonParameters>]
```

## DESCRIPTION
Installs a WinGet package from a configured source.

## EXAMPLES

### Example 1: Install a package using a query
```powershell
PS C:\> Install-WinGetPackage Microsoft.PowerShell
```

This sample installs the Microsoft.PowerShell package. No specific property to identify the query string "Microsoft.PowerShell" as a package identifier. This is a convenient short form not requiring the user to pass "-Id".

### Example 2: Install a package by Id
```powershell
PS C:\> Install-WinGetPackage -Id Microsoft.PowerShell
```

This sample installs the Microsoft.PowerShell package by the specifying the package identifier.

> **If more than one source is configured with the same package identifier, the user must disambiguate**

### Example 3: Install a package by Name
```powershell
PS C:\> Install-WinGetPackage -Name "PowerToys (Preview)"
```

This sample installs the PowerToys package by the specifying the package name.

### Example 4: Install a specific version of a package
```powershell
PS C:\> Install-WinGetPackage Microsoft.PowerShell -Version 7.4.4.0
```

This sample installs the Microsoft.PowerShell package version 7.4.4.0. No specific property to identify the query string "Microsoft.PowerShell" as a package identifier.

## PARAMETERS

### -AllowHashMismatch
Allow the install to continue even if the SHA256 has does not match.

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
Specify the hardware architecture for the installer.

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
Arguments to be passed to the installer in addition to those in the manifest.

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
Force the installer to run even when other checks WinGet would perform would prevent this action.

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
Custom value to be passed via HTTP header to WinGet REST sources.

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
Specifies the package identifier.

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
Specifies the installer type.

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
Specifies the installer locale.

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
Specify the install location. This requires support in the installer.

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
Specify the location for the installer log to be written.

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
Specify the match type.

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
Specify the installer output mode. Options are "Default", "Silent", and "Interactive".

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
Specify the package moniker.

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
Specify the package name.

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
Specify arguments to pass to the installer. This will override arguments in the manifest.

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
WinGet package object.

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
Search string. Wildcards are not supported.

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
Specify installer scope. Valid values are "Any", "User", "System", "UserOrUnknown", "SystemOrUnknown".

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
Skip dependency installation.

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
Specify configured WinGet source.

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

### Microsoft.WinGet.Client.PSObjects.PSPackageInstallMode

### System.String

### System.Management.Automation.SwitchParameter

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

[Find-WinGetPackage](Find-WinGetPackage.md)

[Update-WinGetPackage](Update-WinGetPackage.md)

[Uninstall-WinGetPackage](Uninstall-WinGetPackage.md)
