# Windows Package Manager PowerShell Module

This project contains the source code for building the Windows Package Manager PowerShell Module.

## Running the PowerShell Module Locally

After building the project solution, the `Microsoft.WinGet.Client` PowerShell module can be found in the output directory in the `PowerShell` folder. For example if you built the project as x64 release, you should expect to find the module files in `$(ProjectDirectory)/src/x64/Release/PowerShell`.

To run the module, make sure you are using the latest version of PowerShell (not Windows PowerShell), then import the module manifest (Microsoft.WinGet.Client.psd1).

```
Import-Module <Path to Microsoft.WinGet.Client.psd1">
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