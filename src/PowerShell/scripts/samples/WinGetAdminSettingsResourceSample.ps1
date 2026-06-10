# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Simple sample on how to use WinGetAdminSettings DSC resource.
        Requires PSDesiredStateConfiguration version 2.0.6
        
        IMPORTANT: This will leave LocalManifestFiles enabled
        Run as admin for set.
#>

#Requires -Modules Microsoft.WinGet.Client, Microsoft.WinGet.DSC

using module Microsoft.WinGet.DSC
using namespace System.Collections.Generic

$resource = @{
    Name = 'WinGetAdminSettings'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
    }
}

$getResult = Invoke-DscResource @resource -Method Get
Write-Host "Current sources"
$getResult.Settings

$expectedSources = [List[Hashtable]]::new()
$expectedSources.Add(@{
    Name = "winget"
    Arg = "https://cdn.winget.microsoft.com/cache"
})

# Lets see if LocalManifestFiles is enabled
$resource.Property = @{
    Settings = @{
        LocalManifestFiles = $true
    }
}

$testResult = Invoke-DscResource @resource -Method Test
if (-not $testResult.InDesiredState)
{
    Write-Host "LocalManifestFiles is disabled, enabling"
    Invoke-DscResource @resource -Method Set | Out-Null

    # Now try again
    $testResult2 = Invoke-DscResource @resource -Method Test
    if (-not $testResult.InDesiredState)
    {
        Write-Host "LocalManifestFiles is now enabled"
    }
    else
    {
        Write-Host "Is there a bug somewhere?"
        return
    }
}
else
{
    Write-Host "LocalManifestFiles is already enabled"
}
