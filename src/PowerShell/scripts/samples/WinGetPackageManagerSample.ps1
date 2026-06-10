# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Simple sample on how to use WinGetIntegrity DSC resource.
        Requires PSDesiredStateConfiguration version 2.0.6

        IMPORTANT: This will modify the winget you have installed
#>

#Requires -Modules Microsoft.WinGet.Client, Microsoft.WinGet.DSC

using module Microsoft.WinGet.DSC

$resource = @{
    Name = 'WinGetPackageManager'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
    }
}

# This just demonstrate the Get command.
$getResult = Invoke-DscResource @resource -Method Get

if (-not([string]::IsNullOrWhiteSpace($getResult.Version)))
{
    Write-Host "Current winget version $($getResult.Version)"
}
else
{
    # If the result of get contains an empty version, it means that winget is not installed. This is not the right
    # way to do it though, is better to call Test with an empty Version property.
    if (-not (Invoke-DscResource @resource -Method Test).InDesiredState)
    {
        Write-Host "winget is not installed"
    }
    else
    {
        Write-Error "BUG BUG!!"
        return
    }
}

# At the time I'm doing this the second latest released winget version is v1.3.2091. Lets assume you want to stay there forever.
$v132091 = "v1.3.2091"
$resource = @{
    Name = 'WinGetPackageManager'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        Version = $v132091
    }
}

$testResult = Invoke-DscResource @resource -Method Test
if ($testResult.InDesiredState)
{
    Write-Host "winget is already in a good state (aka in version $v132091)"
}
else
{
    # Oh no, we are not in a good state. Lets get you there.
    # Internally, Set calls Repair-WinGet -Version v1.3.2691 which means that it will try to repair winget
    # by downloading v1.3.2691 and installing it if needed. For example, if your AppInstaller is not registered
    # it will register the package and then verify the specified version is installed.
    Invoke-DscResource @resource -Method Set | Out-Null

    # Now this should work.
    $testResult = Invoke-DscResource @resource -Method Test
    if ($testResult.InDesiredState)
    {
        Write-Host "winget is in a good state (aka in version $v132091)"
    }
    else
    {
        Write-Error "BUG BUG!!"
        return
    }
}

# Now, lets say that you want to have always the latest winget installed. You can specify UseLatest.
$resource = @{
    Name = 'WinGetPackageManager'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        UseLatest = $true
    }
}
$testResult = Invoke-DscResource @resource -Method Test
if ($testResult.InDesiredState)
{
    Write-Host "winget version is the latest version"
}
else
{
    Write-Host "winget version is not latest version"
}

# You can also do UseLatestPreRelease
$resource = @{
    Name = 'WinGetPackageManager'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        UseLatestPreRelease = $true
    }
}
$testResult = Invoke-DscResource @resource -Method Test
if ($testResult.InDesiredState)
{
    Write-Host "winget version is the latest prerelease version"
}
else
{
    # Get the latest prerelease.
    Invoke-DscResource @resource -Method Set | Out-Null

    $testResult = Invoke-DscResource @resource -Method Test
    if ($testResult.InDesiredState)
    {
        Write-Host "winget version is the latest prerelease version"
    }
    else
    {
        Write-Error "BUG BUG!!"
        return
    }
}
