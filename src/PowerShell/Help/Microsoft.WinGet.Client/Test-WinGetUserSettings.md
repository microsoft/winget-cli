---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
online version:
schema: 2.0.0
---

# Test-WinGetUserSettings

## SYNOPSIS
Tests WinGet settings.

## SYNTAX

```
Test-WinGetUserSettings -UserSettings <Hashtable> [-IgnoreNotSet] [<CommonParameters>]
```

## DESCRIPTION
Tests whether the provided UserSettings matches the current WinGet settings configuration.

## EXAMPLES

### Example 1: Test for exact match.
```powershell
PS C:\> Test-WinGetUserSettings -UserSettings @{ installBehavior= @{ preferences= @{ scope = "user"}} }
```

Tests if the provided input matches the current WinGet settings configuration. This command will return False if it is not an exact match..

### Example 2: Test only progress bar setting.
```powershell
PS C:\> Test-WinGetUserSettings -UserSettings @{ visual= @{ progressBar="rainbow"} } -IgnoreNotSet
```

Tests if the progress bar theme is set to rainbow. Including the -IgnoreNotSet argument does not include other WinGet settings in the comparison.

## PARAMETERS

### -IgnoreNotSet
Ignore settings that are not specified in the input.

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

### System.Boolean

## NOTES

## RELATED LINKS

[Get-WinGetUserSettings](Get-WinGetUserSettings.md)

[Set-WinGetUserSettings](Set-WinGetUserSettings.md)
