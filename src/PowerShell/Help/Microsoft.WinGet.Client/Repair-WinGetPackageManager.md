---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Repair-WinGetPackageManager

## SYNOPSIS
Repairs the WinGet client.

## SYNTAX

### IntegrityVersionSet (Default)
```
Repair-WinGetPackageManager [-AllUsers] [-Force] [-Version <String>] [<CommonParameters>]
```

### IntegrityLatestSet
```
Repair-WinGetPackageManager [-AllUsers] [-Force] [-Latest] [-IncludePreRelease]
 [<CommonParameters>]
```

## DESCRIPTION
Repairs the WinGet client by installing or updating the client to either the latest or a specified version. This cmdlet also checks to make sure that the client is in a working state.

## EXAMPLES

### Example 1: Repair the WinGet client.
```powershell
PS C:\> Repair-WinGetPackageManager
```

Ensures the current installed version of WinGet is functional.

### Example 2: Force install the latest version.
```powershell
PS C:\> Repair-WinGetPackageManager -Latest -Force
```

Installs the latest version of WinGet and ensures it functions properly.

## PARAMETERS

### -AllUsers
Indicates that this cmdlet repairs the WinGet client for all user accounts on the computer. To use this parameter, you must run the command with administrator permissions.

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

### -Force
Force the repair even when other checks WinGet would perform would prevent this action.

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

### -IncludePreRelease
Includes prerelease versions of the WinGet client.

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
Installs the latest available version of the WinGet client.

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
Specify the WinGet client version.

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
This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable, -InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable, -ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see [about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.Management.Automation.SwitchParameter

### System.String

## OUTPUTS

### System.Int32

## NOTES

## RELATED LINKS
