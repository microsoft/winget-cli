# Windows Package Manager PowerShell Module

The Windows Package Manager PowerShell Module is made up on two components

1. The `Microsoft.WinGet.Client.Cmdlets` project which contains cmdlet implementations.
2. The `Microsoft.WinGet.Client.Engine` project which contain the real logic for the cmdlets.

## Building the PowerShell Module Locally

After building the Microsoft.WinGet.Client.Cmdlets project, the `Microsoft.WinGet.Client` PowerShell module can be found in the output directory in the `PowerShell` folder. For example if you built the project as x64 release, you should expect to find the module files in `$(SolutionDirectory)/src/x64/Release/PowerShell`.

This project has after build targets that will copy all the necessary files in the correct location.

## Adding a new cmdlet
In order to avoid [assembly dependency conflicts](https://learn.microsoft.com/en-us/powershell/scripting/dev-cross-plat/resolving-dependency-conflicts?view=powershell-7.3) this project uses a custom `AssemblyLoadContext` that load all dependencies.

Microsoft.WinGet.Client.Cmdlets.dll is the binary that gets loaded when the module is imported. When Microsoft.WinGet.Client.Engine.dll is getting loaded the resolving handler use the custom ALC to load it. Then all the dependencies of that binary will be loaded using that custom context.

The dependencies are laid out in two directories: `DirectDependencies` and `SharedDependencies`. The resolving handler looks for binaries under `DirectDependencies` and uses the custom ALC to load them. The custom ALC load any binaries in `DirectDependencies` and `SharedDependencies`.

Exception: WinRT.Runtime.dll doesn't support getting loaded in multiple times in the same process, because it affects static state in the CLR itself. We special case it to get loaded in by the default loader.

If the new cmdlet introduces a new dependency, please make sure to add it in the after build targets to copy it in the Dependencies directory.

## Cmdlets
- Assert-WinGetPackageManager
- Find-WinGetPackage
- Get-WinGetPackage
- Get-WinGetSource
- Get-WinGetUserSetting
- Get-WinGetVersion
- Install-WinGetPackage
- Repair-WinGetPackageManager
- Set-WinGetUserSetting
- Test-WinGetUserSetting
- Uninstall-WinGetPackage
- Update-WinGetPackage
- Add-WinGetSource
- Disable-WinGetSetting
- Enable-WinGetSetting
- Get-WinGetSetting
- Remove-WinGetSource
- Reset-WinGetSource
- Repair-WinGetPackage

## Quick Start Guide

**To run the module, make sure you are using the latest version of PowerShell (not Windows PowerShell)**. 

```
winget install --id Microsoft.Powershell
```

Import the module manifest (Microsoft.WinGet.Client.psd1) by running the following command:

```
Import-Module <Path to Microsoft.WinGet.Client.psd1>
```
