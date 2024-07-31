﻿---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Set-WinGetUserSettings

## SYNOPSIS
Set WinGet settings.

## SYNTAX

```
Set-WinGetUserSettings -UserSettings <Hashtable> [-Merge] [<CommonParameters>]
```

## DESCRIPTION
Set the behavior of various WinGet settings. For more information about WinGet settings, visit https://aka.ms/winget-settings.

## EXAMPLES

### Example 1: Set progress bar theme.
```powershell
PS C:\> Set-WinGetUserSettings -UserSettings @{ visual= @{ progressBar="rainbow"} }
```
Sets the theme of the progress bar to rainbow.

### Example 2: Merge install behavior settings.
```powershell
PS C:\> Set-WinGetUserSettings -UserSettings @{ installBehavior= @{ preferences= @{ scope = "user"}} } -Merge
```

Appends the user scope preference setting to the existing WinGet settings configuration.

## PARAMETERS

### -Merge
Appends the provided UserSettings input to the existing settings configuration.

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
This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable, -InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable, -ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see [about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.Collections.Hashtable

### System.Management.Automation.SwitchParameter

## OUTPUTS

### System.Collections.Hashtable

## NOTES

## RELATED LINKS

[Get-WinGetUserSettings](Get-WinGetUserSettings.md)

[Test-WinGetUserSettings](Test-WinGetUserSettings.md)
