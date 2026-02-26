---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Set-WinGetUserSetting
---

# Set-WinGetUserSetting

## SYNOPSIS
Sets configuration settings of the WinGet client for the current user.

## SYNTAX

```
Set-WinGetUserSetting -UserSettings <Hashtable> [-Merge] [<CommonParameters>]
```

## DESCRIPTION

This command sets configuration settings of the WinGet client for the current user. The user
settings file doesn't exist until you set a specific value. The file is stored in
`$env:LOCALAPPDATA\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\settings.json`.
For more information about WinGet settings, see
[WinGet CLI Settings](https://aka.ms/winget-settings).

## EXAMPLES

### Example 1: Set progress bar theme

```powershell
Set-WinGetUserSetting -UserSettings @{
    visual = @{
        progressBar = 'rainbow'
    }
}
```

Sets the theme of the progress bar to rainbow.

### Example 2: Merge install behavior settings

```powershell
Set-WinGetUserSetting  -Merge -UserSettings @{
    installBehavior = @{
        preferences = @{
            scope = 'user'
        }
    }
}
```

Appends the user scope preference setting to the existing WinGet settings configuration.

### Example 3: Change multiple settings

```powershell
Set-WinGetUserSetting -UserSettings @{
    visual = @{
        progressBar = 'rainbow'
        anonymizeDisplayedPaths = $true
    }
}
```

## PARAMETERS

### -Merge

By default, the command overwrites the current setting with the values provided. Use this parameter
to append the new settings to the existing configuration.

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

### System.Collections.Hashtable

## NOTES

## RELATED LINKS

[Get-WinGetUserSetting](Get-WinGetUserSetting.md)

[Test-WinGetUserSetting](Test-WinGetUserSetting.md)
