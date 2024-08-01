---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
---

# Assert-WinGetPackageManager

## SYNOPSIS
Verifies WinGet is installed properly.

## SYNTAX

### IntegrityVersionSet (Default)

```
Assert-WinGetPackageManager [-Version <String>] [<CommonParameters>]
```

### IntegrityLatestSet

```
Assert-WinGetPackageManager [-Latest] [-IncludePreRelease] [<CommonParameters>]
```

## DESCRIPTION

Verifies WinGet is installed properly.

> [!NOTE]
> This does not ensure the latest version of WinGet is installed. This just verifies the current
> installed version correlated with the Microsoft.WinGet.Client module is functional.

## EXAMPLES

### Example 1: Default usage

```powershell
Assert-WinGetPackageManager
```

If the current version of WinGet is installed correctly this will return without error. This does
not upgrade WinGet to the latest version.

### Example 2: Check if latest stable version is installed

```powershell
Assert-WinGetPackageManager -Latest
```

If the latest version of WinGet correlated with the Microsoft.WinGet.Client module is installed
correctly this will return without error.

### Example 3: Check if latest preview version is installed

```powershell
Assert-WinGetPackageManager -IncludePreRelease
```

If a preview version of WinGet correlated with the Microsoft.WinGet.Client module is installed
correctly this will return without error.

### Example 4: Check if specific version is installed

```powershell
Assert-WinGetPackageManager -Version v1.8.1911
```

If the specified version of WinGet correlated with the Microsoft.WinGet.Client module is installed
correctly this will return without error.

## PARAMETERS

### -IncludePreRelease

Includes preview versions of WinGet.

```yaml
Type: System.Management.Automation.SwitchParameter
Parameter Sets: IntegrityLatestSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Latest

Verifies the latest version of WinGet correlated with the Microsoft.WinGet.Client module is
installed.

```yaml
Type: System.Management.Automation.SwitchParameter
Parameter Sets: IntegrityLatestSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -Version

Verifies a specific version of WinGet is installed.

```yaml
Type: System.String
Parameter Sets: IntegrityVersionSet
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
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

## OUTPUTS

### System.Object

## NOTES

## RELATED LINKS

[Get-WinGetVersion](Get-WinGetVersion.md)
