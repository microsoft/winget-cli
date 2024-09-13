# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Simple sample on how to use WinGetPackage DSC resource.
        Requires PSDesiredStateConfiguration v2 and enabling the
        PSDesiredStateConfiguration.InvokeDscResource experimental feature
        `Enable-ExperimentalFeature -Name PSDesiredStateConfiguration.InvokeDscResource`
        IMPORTANT: This will install Microsoft.PowerToys.
#>

#Requires -Modules Microsoft.WinGet.Client, Microsoft.WinGet.DSC

using module Microsoft.WinGet.DSC

$resource = @{
    Name = 'WinGetPackage'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        Id = 'Microsoft.PowerToys'
        Ensure = "Absent"
    }
}

$testResult = Invoke-DscResource @resource -Method Test
if ($testResult.InDesiredState)
{
    Write-Host "PowerToys is not installed."
}
else
{
    Write-Host "PowerToys is installed."
}

# Default value of Ensure is present.
$resource = @{
    Name = 'WinGetPackage'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        Id = 'Microsoft.PowerToys'
        Version = '0.65.0'
    }
}

$testResult = Invoke-DscResource @resource -Method Test
if ($testResult.InDesiredState)
{
    Write-Host "PowerToys 0.65.0 is installed."
}
else
{
    Write-Host "PowerToys 0.65.0 is not installed."

    Invoke-DscResource @resource -Method Set
}

$resource = @{
    Name = 'WinGetPackage'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        Id = 'Microsoft.PowerToys'
    }
}
$testResult = Invoke-DscResource @resource -Method Test
if ($testResult.InDesiredState)
{
    Write-Host "PowerToys latest version is installed."
}
else
{
    Write-Host "PowerToys latest version is no installed."

    Invoke-DscResource @resource -Method Set
}