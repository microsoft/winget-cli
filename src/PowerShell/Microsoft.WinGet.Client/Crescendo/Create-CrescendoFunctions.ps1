# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Creates crescendo module for Microsoft.WinGet.Client and merge module manifests.
    
    .PARAMETER ConfigurationFile
        The crescendo configuration file.
    
    .PARAMETER ModuleName
        The name of the module to be created.

    .PARAMETER ModuleOutputDirectory
        Where to output the crescendo output files.
#>

[CmdletBinding()]
param (
    [Parameter(Mandatory)]
    [string]
    $ConfigurationFile,

    [Parameter(Mandatory)]
    [string]
    $ModuleName,

    [Parameter(Mandatory)]
    [string]
    $ModuleOutputDirectory
)

if (-not (Get-Module Microsoft.PowerShell.Crescendo))
{
    Install-Module Microsoft.PowerShell.Crescendo -Force
}

$dir = $pwd
Set-Location $PSScriptRoot
Write-Host "Generating crescendo module"
Export-CrescendoModule -ConfigurationFile $ConfigurationFile -ModuleName $ModuleName -Force
Set-Location $dir

Copy-Item "$PSScriptRoot\$ModuleName.psm1" "$ModuleOutputDirectory\Microsoft.WinGet.Client.psm1" -Force -ErrorAction Stop

# In a perfect world we would check if $ModuleOutputDirectory\$ModuleName.psd1 exists and if it does then load the data
# via Import-PowerShellDataFile and make sure FunctionsToExport contains all the exported functions from the generated
# psd1 file of the Export-CrescendoModule command. We have dynamic expressions on ..\Module\Microsoft.WinGet.Client.psd1
# so that can't happen easily, so we will just nicely remind you :(
$config = Import-PowerShellDataFile -Path "$PSScriptRoot\$ModuleName.psd1"

Write-Host "Crescendo module generated. Please verify the FunctionsToExport is updated in ..\Module\Microsoft.WinGet.Client.psd1 if needed"
Write-Host "Generated FunctionsToExport $($config.FunctionsToExport)"
