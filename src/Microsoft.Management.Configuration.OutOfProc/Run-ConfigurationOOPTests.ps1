# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdletBinding()]
param(
    [string]$BuildOutputPath,

    [string]$PackageLayoutPath,

    [string]$TestCaseFilter,

    [string]$Platform = "x64",

    [string]$Configuration = "Debug"
)

# Derive BuildOutputPath from the repo root if not specified
if ([System.String]::IsNullOrEmpty($BuildOutputPath))
{
    $Local:repoRoot = git -C $PSScriptRoot rev-parse --show-toplevel
    $BuildOutputPath = Join-Path $Local:repoRoot "src" $Platform $Configuration
}

# Step 1: Prepare the test output directory
$Local:prepareScript = Join-Path $PSScriptRoot "Prepare-ConfigurationOOPTests.ps1"
& $Local:prepareScript -BuildOutputPath $BuildOutputPath -PackageLayoutPath $PackageLayoutPath

if (-not $?)
{
    Write-Error "Preparation script failed"
    exit 1
}

# Step 2: Find vstest.console.exe via vswhere
$Local:vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $Local:vswhere))
{
    Write-Error "vswhere.exe not found at '$Local:vswhere'. Is Visual Studio installed?"
    exit 1
}

$Local:vsInstallPath = & $Local:vswhere -latest -products * -requires Microsoft.VisualStudio.PackageGroup.TestTools.Core -property installationPath
if ([System.String]::IsNullOrEmpty($Local:vsInstallPath))
{
    Write-Error "Could not find a Visual Studio installation with test tools via vswhere."
    exit 1
}

$Local:vstestPath = Join-Path $Local:vsInstallPath "Common7\IDE\Extensions\TestPlatform\vstest.console.exe"
if (-not (Test-Path $Local:vstestPath))
{
    Write-Error "vstest.console.exe not found at '$Local:vstestPath'."
    exit 1
}

# Step 3: Run the tests with vstest
$Local:testDll = Join-Path $BuildOutputPath "Microsoft.Management.Configuration.UnitTests\net8.0-windows10.0.26100.0\Microsoft.Management.Configuration.UnitTests.dll"

$Local:vstestArgs = @(
    $Local:testDll,
    "--logger:console;verbosity=detailed"
)

if (-not [System.String]::IsNullOrEmpty($TestCaseFilter))
{
    $Local:vstestArgs += "--TestCaseFilter:$TestCaseFilter"
}

& $Local:vstestPath @Local:vstestArgs
exit $LASTEXITCODE
