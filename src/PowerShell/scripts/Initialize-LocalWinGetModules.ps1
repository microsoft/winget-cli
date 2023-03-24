# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Helper script to setup the modules locally.
        - Copies the PowerShell modules output into this location.
        - Copies the modules files from the project because there's no guarantee they are updated in the module output
          location.
        - Adds the module location to PSModulePath if not there.
        - Import Microsoft.WinGet.* modules.
    
    .PARAMETER Platform
        The platform we are building for.
    
    .PARAMETER Configuration
        The configuration we are building in.
#>

[CmdletBinding()]
param (
    [Parameter(Mandatory)]
    [string]
    $Platform,

    [Parameter(Mandatory)]
    [string]
    $Configuration,

    [switch]
    $SkipImportModule
)

class WinGetModule
{
    [string]$Name
    [string]$ModuleRoot
    [bool]$HasBinary
    [bool]$ForceWinGetDev

    WinGetModule([string]$n, [string]$m, [bool]$b, [bool]$d)
    {
        $this.Name = $n
        $this.ModuleRoot = $m
        $this.HasBinary = $b
        $this.ForceWinGetDev = $d
    }
}

# I know it makes sense, but please don't do a clean up of $moduleRootOutput. When the modules are loaded
# there's no way to tell PowerShell to release the binary dlls that are loaded.
$moduleRootOutput = "$PSScriptRoot\Module\"

# Add here new modules
[WinGetModule[]]$modules = 
    [WinGetModule]::new("Microsoft.WinGet.DSC", "$PSScriptRoot\..\Microsoft.WinGet.DSC\", $false, $false),
    [WinGetModule]::new("Microsoft.WinGet.Client", "$PSScriptRoot\..\Microsoft.WinGet.Client\Module\", $true, $true)

foreach($module in $modules)
{
    # Import-Module with Force just changes functions in the root module, not any nested ones. There's no way to load any
    # updated classes. To ensure that you are running the latest version run Remove-Module
    if (Get-Module -ListAvailable -Name $module.Name)
    {
        Write-Host "Removing module $($module.Name)" -ForegroundColor Green
        Remove-Module $module.Name -Force
    }

    # Use xcopy to copy only files that have changed.
    if ($module.HasBinary)
    {
        # Copy output files from VS.
        Write-Host "Coping binary module $($module.Name)" -ForegroundColor Green
        xcopy "$PSScriptRoot\..\..\$Platform\$Configuration\PowerShell\$($module.Name)\" "$moduleRootOutput\$($module.Name)\" /d /s /f /y
    }

    # Copy PowerShell files even for modules with binary resources.
    # VS won't update the files if there's nothing to build...
    Write-Host "Coping module $($module.Name)" -ForegroundColor Green
    xcopy $module.ModuleRoot "$moduleRootOutput\$($module.Name)\" /d /s /f /y

    if ($module.ForceWinGetDev)
    {
        # This is a terrible and shouldn't be used for real things. We must consider making something smarter and prettier.
        # We could make the build system always take the crescendo json and generated the functions from it. The
        # build system would know if the original name to be winget.exe or wingetdev.exe, set it on the json and produce
        # the psm1 one. We could add a VS after build task that calls powershell and does it, or we could move away
        # from crescendo and let the internal implementation knows which one to use based on the build preprocessor macro.
        $psm1File = "$moduleRootOutput\$($module.Name)\$($module.Name).psm1"
        (Get-Content $psm1File).replace("winget.exe", "wingetdev") | Set-Content $psm1File
    }
}

# Add it to module path if not there.
if (-not $env:PSModulePath.Contains($moduleRootOutput))
{
    Write-Host "Added $moduleRootOutput to PSModulePath" -ForegroundColor Green
    $env:PSModulePath += ";$moduleRootOutput"
}

# Now import modules.
if (-not $SkipImportModule)
{
    foreach($module in $modules)
    {
        Write-Host "Importing module $($module.Name)" -ForegroundColor Green
        Import-Module $module.Name -Force
    }
}
