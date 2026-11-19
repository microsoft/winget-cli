---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
---

# Reset-WinGetPin

## SYNOPSIS
Resets all WinGet package pins.

## SYNTAX

```
Reset-WinGetPin [-Source <String>] [-Force] [-ProgressAction <ActionPreference>] [-WhatIf] [-Confirm]
 [<CommonParameters>]
```

## DESCRIPTION
This command resets all WinGet package pins, removing all pin records. Mirrors the behavior of
`winget pin reset`. You can scope the reset to a specific source by providing the **Source**
parameter. Use the **Force** parameter to skip the confirmation prompt.

## EXAMPLES

### Example 1: Reset all pins
```powershell
Reset-WinGetPin
```
This example resets all package pins across all configured sources.

### Example 2: Reset pins for a specific source
```powershell
Reset-WinGetPin -Source "winget"
```
This example resets all package pins for the `winget` source.

### Example 3: Reset all pins without confirmation
```powershell
Reset-WinGetPin -Force
```
This example resets all pins without prompting for confirmation.

## PARAMETERS

### -Confirm

Prompts you for confirmation before running the cmdlet.

```yaml
Type: System.Management.Automation.SwitchParameter
Parameter Sets: (All)
Aliases: cf

Required: False
Position: Named
Default value: None
Accept pipeline input: False
Accept wildcard characters: False
```

### -Force

When specified, skips the confirmation prompt and resets pins immediately.

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

### -Source

Specify the name of a configured WinGet source to scope the reset. If not specified, pins
across all sources are reset.

```yaml
Type: System.String
Parameter Sets: (All)
Aliases:

Required: False
Position: Named
Default value: None
Accept pipeline input: True (ByPropertyName)
Accept wildcard characters: False
```

### -WhatIf

Shows what would happen if the cmdlet runs. The cmdlet is not run.

```yaml
Type: System.Management.Automation.SwitchParameter
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

### System.String

## OUTPUTS

### Microsoft.WinGet.Client.Engine.PSObjects.PSPinResult

## NOTES

## RELATED LINKS

[Get-WinGetPin](Get-WinGetPin.md)

[Add-WinGetPin](Add-WinGetPin.md)

[Remove-WinGetPin](Remove-WinGetPin.md)
