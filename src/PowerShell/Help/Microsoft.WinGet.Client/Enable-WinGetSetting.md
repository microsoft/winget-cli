---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Enable-WinGetSetting

## SYNOPSIS
Enables an administrative setting.

## SYNTAX

```
Enable-WinGetSetting [-Name] <String> [<CommonParameters>]
```

## DESCRIPTION
Enables an administrative setting. Requires administrator access.

Administrator Settings default to "Disabled" and may be controlled by Group Policy Objects.

Administrator Settings:

* LocalManifestFiles
* BypassCertificatePinningForMicrosoftStore
* InstallerHashOverride
* LocalArchiveMalwareScanOverride
* ProxyCommandLineOptions
* DefaultProxy

## EXAMPLES

### Example 1
```powershell
PS C:\> Enable-WinGetSetting -Name LocalManifestFiles
```

Enable the use of local manifest files.

## PARAMETERS

### -Name
The name of the administrator setting

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
This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable, -InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable, -ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see [about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.String

## OUTPUTS

### System.Object
## NOTES

## RELATED LINKS

[Get-WinGetSettings](Get-WinGetSettings.md)

[Disable-WinGetSetting](Disable-WinGetSetting)
