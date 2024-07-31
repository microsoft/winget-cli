---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Uninstall-WinGetPackage

## SYNOPSIS
Uninstall a WinGet Package.

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
Uninstalls the selected package found by searching the installed packages list.

## EXAMPLES

### Example 1: Uninstall a package using a query
```powershell
PS C:\> Uninstall-WinGetPackage Microsoft.PowerShell
```

This sample uninstalls the Microsoft.PowerShell package. No specific property to identify the query string "Microsoft.PowerShell" as a package identifier. This is a convenient short form not requiring the user to pass "-Id".

### Example 2: Uninstall a package by Id
```powershell
PS C:\> Uninstall-WinGetPackage -Id Microsoft.PowerShell
```

This sample uninstalls the Microsoft.PowerShell package by the specifying the package identifier.

> **If more than one source is configured with the same package identifier, the user must disambiguate**

### Example 3: Uninstall a package by Name
```powershell
PS C:\> Uninstall-WinGetPackage -Name "PowerToys (Preview)"
```

This sample uninstalls the PowerToys package by the specifying the package name.

### Example 4: Uninstall a specific version of a package
```powershell
PS C:\> Uninstall-WinGetPackage Microsoft.PowerShell -Version 7.4.4.0
```

This sample uninstalls the Microsoft.PowerShell package version 7.4.4.0. No specific property to identify the query string "Microsoft.PowerShell" as a package identifier.

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
Specify package identifier

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
Specify log location for uninstall.

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

> **Note: Not all uninstallers support all modes.**

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
Specify the version.

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
