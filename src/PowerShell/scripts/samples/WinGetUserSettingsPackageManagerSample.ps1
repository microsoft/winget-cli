# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Simple sample on how to use WinGetUserSettings DSC resource.
        Requires PSDesiredStateConfiguration version 2.0.6

        IMPORTANT: If you loaded the released modules this will modify your settings.
                   Use the -Restore to get back to your original settings

    .PARAMETER Restore
        Restore back to the original user settings.
#>

#Requires -Modules Microsoft.WinGet.Client, Microsoft.WinGet.DSC

using module Microsoft.WinGet.DSC

[CmdletBinding()]
param (
    [Parameter()]
    [switch]
    $Restore
)

$resource = @{
    Name = 'WinGetUserSettings'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
    }
}

# Get current settings
$getResult = Invoke-DscResource @resource -Method Get
Write-Host "Current Settings"
$settingsBackup = $getResult.Settings
$getResult.Settings | ConvertTo-Json

# Test if telemetry is disabled
$resource.Property = @{
    Settings = @{
        telemetry = @{
            disable = $false
        }
    }
    # If you want to check that this setting is the only setting set use [WinGetAction]::Full
    Action = [WinGetAction]::Partial
}

$testResult = Invoke-DscResource @resource -Method Test
if (-not $testResult.InDesiredState)
{
    Write-Host "Adding telemetry setting"
    Invoke-DscResource @resource -Method Set | Out-Null

    Write-Host "New settings"
    $getResult = Invoke-DscResource @resource -Method Get
    $getResult.Settings | ConvertTo-Json
}
else
{
    Write-Host "Telemetry is already disabled"
}

if ($Restore)
{
    $resource.Property.Settings = $settingsBackup
    $resource.Property.Action = [WinGetAction]::Full
    Invoke-DscResource @resource -Method Set | Out-Null
    Write-Host "Settings restored."
}
