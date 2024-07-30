---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Update-WinGetPackage

## SYNOPSIS
Update a WinGet package.

## SYNTAX

### FoundSet (Default)
```
Update-WinGetPackage [-IncludeUnknown] [-Mode <PSPackageInstallMode>] [-Override <String>] [-Custom <String>]
 [-Location <String>] [-Log <String>] [-Force] [-Header <String>] [-AllowHashMismatch]
 [-Architecture <PSProcessorArchitecture>] [-InstallerType <PSPackageInstallerType>] [-Locale <String>]
 [-Scope <PSPackageInstallScope>] [-SkipDependencies] [-Version <String>] [-Id <String>] [-Name <String>]
 [-Moniker <String>] [-Source <String>] [[-Query] <String[]>] [-MatchOption <PSPackageFieldMatchOption>]
 [-ProgressAction <ActionPreference>] [-WhatIf] [-Confirm] [<CommonParameters>]
```

### GivenSet
```
Update-WinGetPackage [-IncludeUnknown] [-Mode <PSPackageInstallMode>] [-Override <String>] [-Custom <String>]
 [-Location <String>] [-Log <String>] [-Force] [-Header <String>] [-AllowHashMismatch]
 [-Architecture <PSProcessorArchitecture>] [-InstallerType <PSPackageInstallerType>] [-Locale <String>]
 [-Scope <PSPackageInstallScope>] [-SkipDependencies] [[-PSCatalogPackage] <PSCatalogPackage>]
 [-Version <String>] [-ProgressAction <ActionPreference>] [-WhatIf] [-Confirm] [<CommonParameters>]
```

## DESCRIPTION
Updates the selected package found by searching the installed packages list.

## EXAMPLES

### Example 1: Update a package using a query
```powershell
PS C:\> Update-WinGetPackage Microsoft.PowerShell
```

This sample updates the Microsoft.PowerShell package. No specific property to identify the query string "Microsoft.PowerShell" as a package identifier. This is a convenient short form not requiring the user to pass "-Id".

### Example 2: Update a package by Id
```powershell
PS C:\> Update-WinGetPackage -Id Microsoft.PowerShell
```

This sample updates the Microsoft.PowerShell package by the specifying the package identifier. 

> **If more than one source is configured with the same package identifier, the user must disambiguate**

### Example 3: Update a package by Name
```powershell
PS C:\> Update-WinGetPackage -Name "PowerToys (Preview)"
```

This sample updates the PowerToys package by the specifying the package name. 

### Example 4: Update to a specific version of a package
```powershell
PS C:\> Update-WinGetPackage Microsoft.PowerShell -Version 7.4.4.0
```

This sample updates the Microsoft.PowerShell package to version 7.4.4.0. No specific property to identify the query string "Microsoft.PowerShell" as a package identifier.

## PARAMETERS

### -AllowHashMismatch
Update the package even if the installer download SHA256 doesn't match the manifest.

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
Specify the installer architecture.

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
Add additional arguments to the installer command.

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
Specify the package identifier.

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
Attempt to upgrade the package when the installed version is not specified in the registry.

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
Specify the installer type to use.

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
Specify the installer locale.

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
Specify the location to install the package.

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

### -Log
Specify the location for the installer log.

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
Specify the match type to use.

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
Specify the moniker.

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
Specify the name.

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
Override the switches to pass to the installer.

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

### -ProgressAction
Determines how PowerShell responds to progress updates generated by a script, cmdlet, or provider, such as the progress bars generated by the Write-Progress cmdlet.

This parameter was added in PowerShell 7.4.

```yaml
Type: System.Management.Automation.ActionPreference
Parameter Sets: (All)
Aliases: proga

Required: False
Position: Named
Default value: None
Accept pipeline input: False
Accept wildcard characters: False
```

### -PSCatalogPackage
WinGet package object

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
This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable, -InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable, -Verbose, -WarningAction, and -WarningVariable. For more information, see [about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

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
