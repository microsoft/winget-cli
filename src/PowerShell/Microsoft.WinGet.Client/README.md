# Windows Package Manager PowerShell Module

This project contains the source code for building the Windows Package Manager PowerShell Module.

## Building the PowerShell Module Locally

After building the project solution, the `Microsoft.WinGet.Client` PowerShell module can be found in the output directory in the `PowerShell` folder. For example if you built the project as x64 release, you should expect to find the module files in `$(SolutionDirectory)/src/x64/Release/PowerShell`.

## Prerequisites

The PowerShell module has a dependency on the latest [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist?view=msvc-170) package which can be downloaded or installed through the Windows Package Manager (WinGet) by running the following command:

```
winget install --id Microsoft.VCRedist.2015+.x64
```

## Quick Start Guide

**To run the module, make sure you are using the latest version of PowerShell (not Windows PowerShell)**. 

```
winget install --id Microsoft.Powershell
```

Import the module manifest (Microsoft.WinGet.Client.psd1) by running the following command:

```
Import-Module <Path to Microsoft.WinGet.Client.psd1>
```

The following cmdlets and functions are available for you to try:

    Get-WinGetVersion
    Enable-WinGetSetting
    Disable-WinGetSetting
    Add-WinGetSource
    Remove-WinGetSource
    Reset-WinGetSource
    Find-WinGetPackage
    Get-WinGetPackage
    Get-WinGetSource
    Install-WinGetPackage
    Uninstall-WinGetPackage
    Update-WinGetPackage