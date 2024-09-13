# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Simple sample on how to use WinGetSourceResource DSC resource.
        Requires PSDesiredStateConfiguration version 2.0.6

        IMPORTANT: This deletes the main winget source and add it again.
        Run as admin for set.
#>

#Requires -Modules Microsoft.WinGet.Client, Microsoft.WinGet.DSC

using module Microsoft.WinGet.DSC
using namespace System.Collections.Generic

[CmdletBinding()]
param (
    [Parameter()]
    [string]
    $SourceName = "winget",

    [Parameter()]
    [string]
    $Argument = "https://cdn.winget.microsoft.com/cache",

    [Parameter()]
    [string]
    $Type = ""
)

$resource = @{
    Name = 'WinGetSourceResource'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        Name = $SourceName
    }
}

$getResult = Invoke-DscResource @resource -Method Get
Write-Host "Current sources"
Format-List $getResult -Force

$resource.Property = @{
    Argument = $Argument
    Type = $Type
}

# The default value comparison for test is Partial, so if you have the winget source this should succeed.
$testResult = Invoke-DscResource @resource -Method Test
if ($testResult.InDesiredState)
{
    Write-Host "winget source is present"
}
else
{
    Write-Host "winget source is not present"
    return
}

# Removing winget. Note this will fail if not run as admin.
$resource.Property = @{
    Ensure = [Ensure]::Absent
}

Invoke-DscResource @resource -Method Set | Out-Null
Write-Host "winget source removed"

# Test again
$testResult = Invoke-DscResource @resource -Method Test
if (-not $testResult.InDesiredState)
{
    Write-Host "winget source is still present."
}
else
{
    Write-Host "winget was removed."
}
