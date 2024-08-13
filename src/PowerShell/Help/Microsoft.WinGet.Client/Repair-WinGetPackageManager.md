---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Repair-WinGetPackageManager
---

# Repair-WinGetPackageManager

## SYNOPSIS
Repairs the installation of the WinGet client on your computer.

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

This command repairs the installation of the WinGet client on your computer by installing the
specified version or the latest version of the client. This command can also install the WinGet 
client if it is not already installed on your machine. It ensures that the client is installed 
in a working state.

## EXAMPLES

### Example 1: Repair the WinGet client

```powershell
Repair-WinGetPackageManager
```

Ensures that the current installed version of WinGet is functioning properly.

### Example 2: Force install the latest version

```powershell
Repair-WinGetPackageManager -Latest -Force
```

This example shows how to repair they WinGet client by installing the latest version and ensuring
it functions properly. The **Force** parameter shuts down the version that is currently running so
that it can update the application files.

## PARAMETERS

### -AllUsers

Use this parameter to repair the WinGet client for all user accounts on the computer. The command
must run the command with administrator permissions.

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

The **Force** parameter shuts down the version that is currently running so that it can update the
application files.

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

Use this parameter to include prerelease versions of the WinGet client.

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

Use this parameter to install the latest available version of the WinGet client.

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

Use this parameter to specify the specific version of the WinGet client to install.

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

This cmdlet supports the common parameters: -Debug, -ErrorAction, -ErrorVariable,
-InformationAction, -InformationVariable, -OutVariable, -OutBuffer, -PipelineVariable,
-ProgressAction, -Verbose, -WarningAction, and -WarningVariable. For more information, see
[about_CommonParameters](http://go.microsoft.com/fwlink/?LinkID=113216).

## INPUTS

### System.Management.Automation.SwitchParameter

### System.String

## OUTPUTS

### System.Int32

## NOTES

## RELATED LINKS

[Get-WinGetVersion](Get-WinGetVersion.md)
