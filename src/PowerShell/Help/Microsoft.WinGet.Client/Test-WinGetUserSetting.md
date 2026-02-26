---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Test-WinGetUserSetting
---

# Test-WinGetUserSetting

## SYNOPSIS
Tests the current state of WinGet user settings.

## SYNTAX

```
Test-WinGetUserSetting -UserSettings <Hashtable> [-IgnoreNotSet] [<CommonParameters>]
```

## DESCRIPTION

This command tests the current state of WinGet user settings against a provided set of values.

## EXAMPLES

### Example 1: Test for exact match

```powershell
Test-WinGetUserSetting -UserSettings @{
    installBehavior = @{
        preferences = @{
            scope = 'user'
        }
    }
}
```

This example shows how to confirm that your current user settings match specific values. The
command returns `$false` if it is not an exact match. 

### Example 2: Test only progress bar setting

```powershell
Test-WinGetUserSetting -IgnoreNotSet -UserSettings @{
    visual = @{
        progressBar = 'rainbow'
    }
}
```

This examples tests whether the progress bar theme is set to `rainbow`. When you use the
**IgnoreNotSet** parameter, the command only tests the provide values and doesn't include other
WinGet settings in the comparison.

## PARAMETERS

### -IgnoreNotSet

When you use the **IgnoreNotSet** parameter, the command only tests the provide values and doesn't
include other WinGet settings in the comparison.

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

### -UserSettings

A hashtable containing the key value pairs representing the WinGet settings.

```yaml
Type: System.Collections.Hashtable
Parameter Sets: (All)
Aliases:

Required: True
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

### System.Collections.Hashtable

### System.Management.Automation.SwitchParameter

## OUTPUTS

### System.Boolean

## NOTES

## RELATED LINKS

[Get-WinGetUserSetting](Get-WinGetUserSetting.md)

[Set-WinGetUserSetting](Set-WinGetUserSetting.md)
