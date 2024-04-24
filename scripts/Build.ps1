# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

<#
.SYNOPSIS
Simplifies build commands for the SFS Client.

.PARAMETER Clean
Use this to clean the build folder before building.

.PARAMETER BuildType
Use this to define the build type between "Debug" and "Release". The default is "Debug".

.PARAMETER EnableTestOverrides
Use this to enable test overrides.

.PARAMETER BuildTests
Use this to enable building tests. On by default.

.PARAMETER BuildSamples
Use this to enable building samples. On by default.

.DESCRIPTION
This script will contain the build commands for the SFS Client. The default build folder will be "<git_root>/build".
Use this on Windows platforms in a PowerShell session.

.EXAMPLE
PS> ./scripts/Build.ps1
#>
param (
    [switch] $Clean = $false,
    # BuildType is a build time parameter for Visual Studio generator in CMake
    [ValidateSet("Debug", "Release")]
    [string] $BuildType = "Debug",
    # Make sure when adding a new switch below to check if it requires CMake regeneration
    [switch] $EnableTestOverrides = $false,
    [bool] $BuildTests = $true,
    [bool] $BuildSamples = $true
)

$GitRoot = (Resolve-Path (&git -C $PSScriptRoot rev-parse --show-toplevel)).Path
if (!(Test-Path "$GitRoot\vcpkg"))
{
    throw "vcpkg not found at $GitRoot\vcpkg. Run the Setup.ps1 script first."
}

$BuildFolder = "$GitRoot/build"
if ($Clean -and (Test-Path $BuildFolder))
{
    Write-Host -ForegroundColor Yellow "Cleaning build folder before build..."
    Remove-Item -Recurse -Force $BuildFolder
}

$Regenerate = $false
$CMakeCacheFile = "$BuildFolder\CMakeCache.txt"
$EnableTestOverridesStr = if ($EnableTestOverrides) {"ON"} else {"OFF"}
$BuildTestsOverridesStr = if ($BuildTests) {"ON"} else {"OFF"}
$BuildSamplesOverridesStr = if ($BuildSamples) {"ON"} else {"OFF"}

function Test-CMakeCacheValueNoMatch($CMakeCacheFile, $Pattern, $ExpectedValue)
{
    $Match = Select-String -Path $CMakeCacheFile -Pattern $Pattern
    if ($null -ne $Match -and $null -ne $Match.Matches.Groups[1])
    {
        return $ExpectedValue -ne $Match.Matches.Groups[1].Value
    }
    return $true
}

if (Test-Path $CMakeCacheFile)
{
    # Regenerate if one of the build options is set to a different value than the one passed in
    $Regenerate = Test-CMakeCacheValueNoMatch $CMakeCacheFile "^SFS_ENABLE_TEST_OVERRIDES:BOOL=(.*)$" $EnableTestOverridesStr
    $Regenerate = Test-CMakeCacheValueNoMatch $CMakeCacheFile "^SFS_BUILD_TESTS:BOOL=(.*)$" $BuildTestsOverridesStr
    $Regenerate = Test-CMakeCacheValueNoMatch $CMakeCacheFile "^SFS_BUILD_SAMPLES:BOOL=(.*)$" $BuildSamplesOverridesStr
}

# Configure cmake if build folder doesn't exist or if the build must be regenerated.
# This creates build targets that will be used by the build command
if (!(Test-Path $BuildFolder) -or $Regenerate)
{
    $Options = "-DSFS_ENABLE_TEST_OVERRIDES=$EnableTestOverridesStr";
    $Options += " -DSFS_BUILD_TESTS=$BuildTestsOverridesStr";
    $Options += " -DSFS_BUILD_SAMPLES=$BuildSamplesOverridesStr";
    $Options += " -DSFS_WINDOWS_STATIC_ONLY=ON";
    Invoke-Expression "cmake -S $GitRoot -B $BuildFolder $Options"
}

# This is the build command. If any CMakeLists.txt files change, this will also reconfigure before building
cmake --build $BuildFolder --config $BuildType
