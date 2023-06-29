# Windows Package Manager PowerShell Module

The Windows Package Manager PowerShell Module is made up on three components

1. Generated functions using `Crescendo`
2. The `Microsoft.WinGet.Client.Cmdlets` project which contains cmdlet implementations.
3. The `Microsoft.WinGet.Client.Engine` project which contain the real logic for the cmdlets.

## Building the PowerShell Module Locally

After building the Microsoft.WinGet.Client.Cmdlets project, the `Microsoft.WinGet.Client` PowerShell module can be found in the output directory in the `PowerShell` folder. For example if you built the project as x64 release, you should expect to find the module files in `$(SolutionDirectory)/src/x64/Release/PowerShell`.

This project has after build targets that will copy all the necessary files in the correct location.

## Adding a new function

We don't have an automatic way of producing the psm1 from Crescendo. If a new function is going to be added:
1. Modify `Crescendo\Crescendo.json`
2. Run `Crescendo\Create-CrescendoFunctions.ps1`
3. Copy the new psm1 in `ModulesFiles\Microsoft.WinGet.Client.psm1`
4. Add new function in `ModulesFiles\Microsoft.WinGet.Client.psd1`

## Adding a new cmdlet

This project uses a custom `AssemblyLoadContext` that handles all dependencies loading. The only two binaries that are loaded in the default context are Microsoft.WinGet.Client.Cmdlets.dll and Microsoft.WinGet.Client.Engine.dll. This is to handle [assembly dependency conflicts](https://learn.microsoft.com/en-us/powershell/scripting/dev-cross-plat/resolving-dependency-conflicts?view=powershell-7.3). Because of that, the cmdlet must be defined in Microsoft.WinGet.Client.Cmdlets but the actual implementation in Microsoft.WinGet.Client.Engine.

If the new cmdlet introduces a new dependency, please make sure to add it in the after build targets to copy it in the Dependencies directory.

## Functions
- Add-WinGetSource
- Disable-WinGetSetting
- Enable-WinGetSetting
- Get-WinGetSettings
- Remove-WinGetSource
- Reset-WinGetSource

## Cmdlets
- Assert-WinGetPackageManager
- Find-WinGetPackage
- Get-WinGetPackage
- Get-WinGetSource
- Get-WinGetUserSettings
- Get-WinGetVersion
- Install-WinGetPackage
- Repair-WinGetPackageManager
- Set-WinGetUserSettings
- Test-WinGetUserSettings
- Uninstall-WinGetPackage
- Update-WinGetPackage

## Quick Start Guide

**To run the module, make sure you are using the latest version of PowerShell (not Windows PowerShell)**. 

```
winget install --id Microsoft.Powershell
```

Import the module manifest (Microsoft.WinGet.Client.psd1) by running the following command:

```
Import-Module <Path to Microsoft.WinGet.Client.psd1>
```
