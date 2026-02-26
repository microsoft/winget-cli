---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Enable-WinGetSetting
---

# Enable-WinGetSetting

## SYNOPSIS

Enables WinGet administrative settings.

## SYNTAX

```
Enable-WinGetSetting [-Name] <String> [<CommonParameters>]
```

## DESCRIPTION

This command enables an administrative setting. The command must be run from an elevated session
using the **Run as Administrator** option.

This command support the following administrative settings:

- LocalManifestFiles
- BypassCertificatePinningForMicrosoftStore
- InstallerHashOverride
- LocalArchiveMalwareScanOverride
- ProxyCommandLineOptions

Administrative settings are disabled by default. Administrative settings can also be managed using
Group Policy objects.

## EXAMPLES

### Example 1

```powershell
Enable-WinGetSetting -Name LocalManifestFiles
```

This example shows how to enable the use of local manifest files.

## PARAMETERS

### -Name

The name of the WinGet administrative setting.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: True
Position: 0
Default value: None
Accept pipeline input: True (ByPropertyName, ByValue)
Accept wildcard characters: False
```

### CommonParameters

This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable,
-InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable,
-ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see
[about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.String

## OUTPUTS

### System.Object

## NOTES

## RELATED LINKS

[Get-WinGetSetting](Get-WinGetSetting.md)

[Disable-WinGetSetting](Disable-WinGetSetting.md)
