---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
---

# Get-WinGetSettings

## SYNOPSIS
Gets WinGet settings.

## SYNTAX

```
Get-WinGetSettings [-AsPlainText] [<CommonParameters>]
```

## DESCRIPTION

Get WinGet settings. This includes the user settings file, administrator settings, and the settings
schema. For more information about WinGet settings, visit `https://aka.ms/winget-settings`.

## EXAMPLES

### Example 1

```powershell
Get-WinGetSettings
```

This will display the user settings file location, the administrator settings, and the settings
schema URL.

## PARAMETERS

### -AsPlainText

Output results as plain text

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

### CommonParameters

This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable,
-InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable,
-ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see
[about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.Management.Automation.SwitchParameter

## OUTPUTS

### System.Object

## NOTES

## RELATED LINKS

[Get-WinGetUserSettings](Get-WinGetUserSettings.md)

[Set-WinGetUserSettings](Set-WinGetUserSettings.md)
