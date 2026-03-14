---
external help file: Microsoft.WinGet.Client.Cmdlets.dll-Help.xml
Module Name: Microsoft.WinGet.Client
ms.date: 08/01/2024
online version:
schema: 2.0.0
title: Assert-WinGetPackageManager
---

# Assert-WinGetPackageManager

## SYNOPSIS
Verifies that WinGet is installed properly.

## SYNTAX

### IntegrityVersionSet (Default)

```
Assert-WinGetPackageManager [-Version <String>] [<CommonParameters>]
```

### IntegrityLatestSet

```
Assert-WinGetPackageManager [-Latest] [-IncludePrerelease] [<CommonParameters>]
```

## DESCRIPTION

Verifies that WinGet is installed properly.

> [!NOTE]
> The cmdlet doesn't ensure that the latest version of WinGet is installed. It just verifies that
> the installed version of Winget is supported by installed version of the Microsoft.WinGet.Client
> module.

## EXAMPLES

### Example 1: Default usage

```powershell
Assert-WinGetPackageManager
```

If the current version of WinGet is installed correctly, the command returns without error.

### Example 2: Check if latest stable version is installed

```powershell
Assert-WinGetPackageManager -Latest
```

If the latest version of WinGet is compatible with the installed Microsoft.WinGet.Client module, the
command returns without error.

### Example 3: Check if latest preview version is installed

```powershell
Assert-WinGetPackageManager -IncludePreRelease
```

If the prerelease version of WinGet is compatible with the installed Microsoft.WinGet.Client module,
the command returns without error.

### Example 4: Check if specific version is installed

```powershell
Assert-WinGetPackageManager -Version v1.8.1911
```

If the specified version of WinGet is compatible with the installed Microsoft.WinGet.Client module,
the command returns without error.

## PARAMETERS

### -IncludePreRelease

Include preview versions of WinGet.

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

Verify that the latest version of WinGet is compatible with the installed version of the
Microsoft.WinGet.Client module.

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

Verify that a specific version of WinGet is installed correctly.

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

### System.String

### System.Management.Automation.SwitchParameter

## OUTPUTS

### System.Object

## NOTES

When using the `-Latest` or `-Version` parameters, this cmdlet makes GitHub API requests to query
release information. Unauthenticated requests are subject to
[GitHub API rate limits](https://docs.github.com/en/rest/using-the-rest-api/rate-limits-for-the-rest-api),
which can cause failures in CI/CD pipelines. If the `GH_TOKEN` or `GITHUB_TOKEN` environment
variable is set, the cmdlet automatically uses it to authenticate requests, which significantly
increases the rate limit.

`GH_TOKEN` takes precedence over `GITHUB_TOKEN`, matching
[GitHub CLI behavior](https://cli.github.com/manual/gh_help_environment). In GitHub Actions,
you can make `GITHUB_TOKEN` available to the cmdlet by mapping it as an environment variable in
your workflow step (e.g., `env: GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}`).

Use `-Verbose` to see which token source is being used.

## RELATED LINKS

[Get-WinGetVersion](Get-WinGetVersion.md)
