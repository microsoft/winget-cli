# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Copies built binaries for a specific platform to an output directory.
    
    .PARAMETER Platform
        The platform we are building for.
    
    .PARAMETER Configuration
        The configuration we are building in.

    .PARAMETER OutDir
        The base output directory where the module manifest will be.
#>

[CmdletBinding()]
param (
    [Parameter(Mandatory)]
    [string]
    $Platform,

    [Parameter(Mandatory)]
    [string]
    $Configuration,

    [Parameter(Mandatory)]
    [string]
    $OutDir
)

# src\x64\Release\Project\net461\Library.Desktop.dll
# src\x64\Release\Project\net5.0-windows10.0.22000.0\Library.Core.dll

# build\Module\x64\Desktop\Library.Desktop.dll
# build\Module\x64\Core\Library.Core.dll

$CoreFramework = 'net5.0-windows10.0.22000.0'
$CoreFolderName = 'Core'
$DesktopFramework = 'net461'
$DesktopFolderName = 'Desktop'
$ProjectName = 'Microsoft.WinGet.Client'

Copy-Item "$PSScriptRoot\..\..\$Platform\$Configuration\$ProjectName\$CoreFramework" "$OutDir\$Platform\$CoreFolderName" -Force -Recurse -ErrorAction Stop
Copy-Item "$PSScriptRoot\..\..\$Platform\$Configuration\$ProjectName\$DesktopFramework" "$OutDir\$Platform\$DesktopFolderName" -Force -Recurse -ErrorAction Stop

Write-Host 'Done!' -ForegroundColor Green
