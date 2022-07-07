---
external help file: Microsoft.WinGet.Client.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Install-WinGetPackage

## SYNOPSIS
{{ Fill in the Synopsis }}

## SYNTAX

### FoundSet (Default)
```
Install-WinGetPackage [-Scope <PackageInstallScope>] [-Architecture <ProcessorArchitecture>]
 [-Mode <PackageInstallMode>] [-Override <String>] [-Location <String>] [-Force] [-Header <String>]
 [-Version <String>] [-Log <String>] [-Id <String>] [-Name <String>] [-Moniker <String>] [-Source <String>]
 [[-Query] <String[]>] [-Exact] [-WhatIf] [-Confirm] [<CommonParameters>]
```

### GivenSet
```
Install-WinGetPackage [-Scope <PackageInstallScope>] [-Architecture <ProcessorArchitecture>]
 [-Mode <PackageInstallMode>] [-Override <String>] [-Location <String>] [-Force] [-Header <String>]
 [[-CatalogPackage] <CatalogPackage>] [-Version <String>] [-Log <String>] [-WhatIf] [-Confirm]
 [<CommonParameters>]
```

## DESCRIPTION
{{ Fill in the Description }}

## EXAMPLES

### Example 1
```powershell
PS C:\> {{ Add example code here }}
```

{{ Add example description here }}

## PARAMETERS

### -Architecture
{{ Fill Architecture Description }}

```yaml
Type: ProcessorArchitecture
Parameter Sets: (All)
Aliases:
Accepted values: X86, Arm, X64, Neutral, Arm64, X86OnArm64, Unknown

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -CatalogPackage
{{ Fill CatalogPackage Description }}

```yaml
Type: CatalogPackage
Parameter Sets: GivenSet
Aliases: InputObject

Required: False
Position: 0
Default value: None
Accept pipeline input: True (ByPropertyName, ByValue)
Accept wildcard characters: False
```

### -Confirm
Prompts you for confirmation before running the cmdlet.

```yaml
Type: SwitchParameter
Parameter Sets: (All)
Aliases: cf

Required: False
Position: Named
Default value: None
Accept pipeline input: False
Accept wildcard characters: False
```

### -Exact
{{ Fill Exact Description }}

```yaml
Type: SwitchParameter
Parameter Sets: FoundSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Force
{{ Fill Force Description }}

```yaml
Type: SwitchParameter
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Header
{{ Fill Header Description }}

```yaml
Type: String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Id
{{ Fill Id Description }}

```yaml
Type: String
Parameter Sets: FoundSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Location
{{ Fill Location Description }}

```yaml
Type: String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Log
{{ Fill Log Description }}

```yaml
Type: String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Mode
{{ Fill Mode Description }}

```yaml
Type: PackageInstallMode
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
{{ Fill Moniker Description }}

```yaml
Type: String
Parameter Sets: FoundSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Name
{{ Fill Name Description }}

```yaml
Type: String
Parameter Sets: FoundSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Override
{{ Fill Override Description }}

```yaml
Type: String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Query
{{ Fill Query Description }}

```yaml
Type: String[]
Parameter Sets: FoundSet
Aliases:

Required: False
Position: 0
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Scope
{{ Fill Scope Description }}

```yaml
Type: PackageInstallScope
Parameter Sets: (All)
Aliases:
Accepted values: Any, User, System

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Source
{{ Fill Source Description }}

```yaml
Type: String
Parameter Sets: FoundSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Version
{{ Fill Version Description }}

```yaml
Type: String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -WhatIf
Shows what would happen if the cmdlet runs.
The cmdlet is not run.

```yaml
Type: SwitchParameter
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

### Microsoft.Management.Deployment.PackageInstallScope

### Windows.System.ProcessorArchitecture

### Microsoft.Management.Deployment.PackageInstallMode

### System.String

### System.Management.Automation.SwitchParameter

### Microsoft.Management.Deployment.CatalogPackage

### System.String[]

## OUTPUTS

### Microsoft.Management.Deployment.InstallResult

## NOTES

## RELATED LINKS
