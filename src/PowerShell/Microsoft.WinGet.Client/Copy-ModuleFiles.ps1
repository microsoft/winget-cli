# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Copies everything but the binary files to an output directory.
    
    .PARAMETER OutDir
        The output directory where the module manifest will live.
#>

[CmdletBinding()]
param (
    [Parameter(Mandatory)]
    [string]
    $OutDir
)

New-Item $OutDir -ItemType Directory -Force -ErrorAction Stop
Copy-Item "$PSScriptRoot\Microsoft.WinGet.Client.psd1" $OutDir -Force -ErrorAction Stop
Copy-Item "$PSScriptRoot\Microsoft.WinGet.Client.psm1" $OutDir -Force -ErrorAction Stop
Copy-Item "$PSScriptRoot\Format.ps1xml" $OutDir -Force -ErrorAction Stop

Write-Host 'Done!' -ForegroundColor Green
