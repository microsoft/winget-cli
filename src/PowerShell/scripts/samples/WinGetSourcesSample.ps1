# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Simple sample on how to use WinGetSourcesResource DSC resource.
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
    Name = 'WinGetSourcesResource'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
    }
}

$getResult = Invoke-DscResource @resource -Method Get
Write-Host "Current sources"

foreach ($source in $getResult.Sources)
{
    Write-Host "Name '$($source.Name)' Arg '$($source.Arg)' Type '$($source.Type)'"
}

$expectedSources = [List[Hashtable]]::new()
$expectedSources.Add(@{
    Name = "winget"
    Arg = "https://cdn.winget.microsoft.com/cache"
})

$resource.Property = @{
    Sources = $expectedSources
    Action = [WinGetAction]::Partial
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

# A full match will fail if there are more sources.
$resource.Property = @{
    Sources = $expectedSources
    Action = [WinGetAction]::Full
}
$testResult = Invoke-DscResource @resource -Method Test
if (-not $testResult.InDesiredState)
{
    Write-Host "winget source is not the only source"
}
else
{
    Write-Host "winget source is the only source"
}

# Breaking winget. Note this will fail if not run as admin.
$resource.Property = @{
    Sources = $expectedSources
    Action = [WinGetAction]::Partial
    Ensure = [Ensure]::Absent
}

Invoke-DscResource @resource -Method Set | Out-Null
Write-Host "winget source removed"

# Test again
$testResult = Invoke-DscResource @resource -Method Test
if (-not $testResult.InDesiredState)
{
    Write-Host "winget source is gone."

    # Add it again
    $resource.Property.Command = [SourceCommand]::Add
    Invoke-DscResource @resource -Method Set | Out-Null
}
else
{
    # TODO: debug. Basically when `winget source remove winget` happens if the
    # commands prints the progress bar the source was not removed. I think that
    # it was actually removed but readded updating the package.
    Write-Host "winget was not removed."
}
