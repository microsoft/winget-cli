# Windows Package Manager Configuration PowerShell Module

The Windows Package Manager Configuration PowerShell Module is made up on three components

1. The `Microsoft.WinGet.Configuration.Cmdlets` project which contains cmdlet implementations.
2. The `Microsoft.WinGet.Configuration.Engine` project which contain the real logic for the cmdlets.

## Cmdlets
- Get-WinGetConfiguration
- Get-WinGetConfigurationDetails
- Invoke-WinGetConfiguration
- Start-WinGetConfiguration
- Complete-WinGetConfiguration

## Syntax 
```
Get-WinGetConfiguration -File <string> [<CommonParameters>]

Get-WinGetConfigurationDetails -Set <PSConfigurationSet> [<CommonParameters>]

Invoke-WinGetConfiguration -Set <PSConfigurationSet> [-AcceptConfigurationAgreements] [<CommonParameters>]

Start-WinGetConfiguration -Set <PSConfigurationSet> [-AcceptConfigurationAgreements] [<CommonParameters>]

Complete-WinGetConfiguration -ConfigurationJob <PSConfigurationJob> [<CommonParameters>]
```

## Prerequisites

Minimum PowerShell 7 version: 7.2.8

## Telemetry
Telemetry is enabled by default. To disable it one should set the POWERSHELL_TELEMETRY_OPTOUT env variable to “1”, “yes” or “true”.

## Building the PowerShell Module Locally
After building the Microsoft.WinGet.Configuration.Cmdlets project, the `Microsoft.WinGet.Configuration` PowerShell module can be found in the output directory in the `PowerShell` folder. For example if you built the project as x64 release, you should expect to find the module files in `$(SolutionDirectory)/src/x64/Release/PowerShell`.

This project has after build targets that will copy all the necessary files in the correct location.

## Adding a new cmdlet
This project uses a custom `AssemblyLoadContext` that handles all dependencies loading. The only two binaries that are loaded in the default context are Microsoft.WinGet.Configuration.Cmdlets.dll and Microsoft.WinGet.Configuration.Engine.dll. This is to handle [assembly dependency conflicts](https://learn.microsoft.com/en-us/powershell/scripting/dev-cross-plat/resolving-dependency-conflicts?view=powershell-7.3). Because of that, the cmdlet must be defined in Microsoft.WinGet.Configuration.Cmdlets but the actual implementation in Microsoft.WinGet.Configuration.Engine.

If the new cmdlet introduces a new dependency, please make sure to add it in the after build targets to copy it in the Dependencies directory.
