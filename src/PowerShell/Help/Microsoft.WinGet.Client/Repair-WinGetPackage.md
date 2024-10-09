---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/26/2024
online version:
schema: 2.0.0
---

# Repair-WinGetPackage

## SYNOPSIS
Repairs a WinGet Package.

## SYNTAX

### FoundSet (Default)
```
Repair-WinGetPackage [-Mode <PSPackageRepairMode>] [-Log <String>] [-Version <String>] [-Id <String>]
 [-Name <String>] [-Moniker <String>] [-Source <String>] [[-Query] <String[]>]
 [-MatchOption <PSPackageFieldMatchOption>] [-ProgressAction <ActionPreference>] [-WhatIf] [-Confirm]
 [<CommonParameters>]
```

### GivenSet
```
Repair-WinGetPackage [-Mode <PSPackageRepairMode>] [-Log <String>] [[-PSCatalogPackage] <PSCatalogPackage>]
 [-Version <String>] [-ProgressAction <ActionPreference>] [-WhatIf] [-Confirm] [<CommonParameters>]
```

## DESCRIPTION
This command repairs a WinGet package from your computer, provided the package includes repair support.
The command includes parameters to specify values used to search for installed packages. By default,
all string-based searches are case-insensitive substring searches. Wildcards are not supported.
You can change the search behavior using the **MatchOption** parameter.

> **Note: Not all packages support repair.**

## EXAMPLES

### Example 1: Repair a package using a query
```powershell
Repair-WinGetPackage -Query "Microsoft.GDK.2406"
```
This example  shows how to repair a package using a query. The **Query** parameter is positional, so you 
don't need to include the parameter name before the query string.

### Example 3 : Repair a package by Id
```powershell
Repair-WinGetPackage -Id "Microsoft.GDK.2406"
```
This example shows how to repair a package by specifying the package identifier.

If the package identifier is available from more than one source, you must provide additional search
criteria to select a specific instance of the package.

### Example 3: Repair a package using by Name
```powershell
Repair-WinGetPackage -Name "Microsoft Game Development Kit - 240602 (June 2024 Update 2)"
```
This example shows how to repair a package using the package name. 

> **Note: Please note that the examples mentioned above are mainly reference examples for the repair cmdlet and may not be operational as is, since many installers don't support repair as a standard functionality. For the Microsoft.GDK.2406 example, the assumption is that Microsoft.GDK.2406 supports repair capability and the author of the installer has provided the necessary repair context/switches in the Package Manifest in the Package Source referenced by the WinGet Client.**

## PARAMETERS

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

Specify the location for the installer repair log. The value can be a fully-qualified or relative path and must include the file name. For example: `$env:TEMP\package.log`.

> **Note: Not all installers support this option.**

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: False
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
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Mode

Specify the output mode for the installer. The parameter accepts the following values:

```yaml
Type: Microsoft.WinGet.Client.PSObjects.PSPackageRepairMode
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

Specify the moniker of the WinGet package to repair. For example, the moniker for the
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

Specify the version of the package to be repaired.

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

### Microsoft.WinGet.Client.PSObjects.PSPackageRepairMode

### Microsoft.WinGet.Client.Engine.PSObjects.PSCatalogPackage

### System.String

### System.String[]

### Microsoft.WinGet.Client.PSObjects.PSPackageFieldMatchOption

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSRepairResult

## NOTES

## RELATED LINKS

[Find-WinGetPackage](Find-WinGetPackage.md)

[Install-WinGetPackage](Install-WinGetPackage.md)

[Uninstall-WinGetPackage](Uninstall-WinGetPackage.md)
