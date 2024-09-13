---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Get-WinGetSetting
---

# Get-WinGetSetting

## SYNOPSIS
Gets WinGet configuration settings.

## SYNTAX

```
Get-WinGetSetting [-AsPlainText] [<CommonParameters>]
```

## DESCRIPTION

This command gets the WinGet configuration settings. The settings include:
- the schema
- administrative settings
- the location of the user settings file
For more information about WinGet settings, see
[WinGet CLI Settings](https://aka.ms/winget-settings).

## EXAMPLES

### Example 1 - Display the WinGet configuration settings

```powershell
Get-WinGetSetting
```

```Output

Name              Value
----              -----
$schema           https://aka.ms/winget-settings-export.schema.json
userSettingsFile  C:\Users\user1\AppData\Local\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\Local…
adminSettings     {[ProxyCommandLineOptions, False], [LocalArchiveMalwareScanOverride, False], [InstallerH…
```

### Example 2 - Display the administrative settings in WinGet configuration

```powershell
Get-WinGetSetting | Select-Object -ExpandProperty adminSettings
```

```Output
Name                                      Value
----                                      -----
InstallerHashOverride                     False
ProxyCommandLineOptions                   False
BypassCertificatePinningForMicrosoftStore False
LocalArchiveMalwareScanOverride           False
LocalManifestFiles                        True
```

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

[Get-WinGetUserSetting](Get-WinGetUserSetting.md)

[Set-WinGetUserSetting](Set-WinGetUserSetting.md)
