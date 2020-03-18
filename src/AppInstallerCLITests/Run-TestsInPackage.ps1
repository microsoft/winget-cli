<#
.SYNOPSIS
    Runs the AppInstallerCLI tests within the packaged context.
.DESCRIPTION
    Registers the loose files generated by the AppInstallerCLIPackage project, then runs the
    existing tests from "within" this context.
.PARAMETER BuildRoot
    The root of the build output directory.  If not provided, assumed to be the local default
    location relative to this script.
.PARAMETER PackageRoot
    The root of the package build output directory.  If not provided, assumed to be the local default
    location relative to this script.
.PARAMETER LogTarget
    The file path to log to.
.PARAMETER TestResultsTarget
    The file path to place the test result file in.
.PARAMETER Args
    Additional args to pass to the tests.
#>
param(
    [Parameter(Mandatory=$false)]
    [string]$BuildRoot,
    
    [Parameter(Mandatory=$false)]
    [string]$PackageRoot,
    
    [Parameter(Mandatory=$false)]
    [string]$LogTarget,
    
    [Parameter(Mandatory=$false)]
    [string]$TestResultsTarget,

    [Parameter(Mandatory=$false)]
    [string]$Args,

    [switch]$Wait
)

if ([String]::IsNullOrEmpty($BuildRoot))
{
    $BuildRoot = Split-Path -Parent $PSCommandPath;
    $BuildRoot = Join-Path $BuildRoot "..\x64\Debug";
}
$BuildRoot = Resolve-Path $BuildRoot
Write-Host "Using BuildRoot = $BuildRoot"

if ([String]::IsNullOrEmpty($PackageRoot))
{
    $PackageRoot = Split-Path -Parent $PSCommandPath;
    $PackageRoot = Join-Path $PackageRoot "..\AppInstallerCLIPackage\bin\x64\Debug";
}
$PackageRoot = Resolve-Path $PackageRoot
Write-Host "Using PackageRoot = $PackageRoot"

if (![String]::IsNullOrEmpty($LogTarget))
{
    $Local:temp = Split-Path -Parent $LogTarget
    $Local:temp = Resolve-Path $Local:temp
    $LogTarget = Join-Path $Local:temp (Split-Path -Leaf $LogTarget)
    Write-Host "Using LogTarget = $LogTarget"
}

if (![String]::IsNullOrEmpty($TestResultsTarget))
{
    $Local:temp = Split-Path -Parent $TestResultsTarget
    $Local:temp = Resolve-Path $Local:temp
    $TestResultsTarget = Join-Path $Local:temp (Split-Path -Leaf $TestResultsTarget)
    Write-Host "Using TestResultsTarget = $TestResultsTarget"
}

# Register the package; this requires the local package to have been deployed at least once or it won't be built.
$Local:ManifestPath = Join-Path $PackageRoot "AppxManifest.xml"
if (-not (Test-Path $Local:ManifestPath))
{
    $Local:ManifestPath = Join-Path $PackageRoot "AppX\AppxManifest.xml"
}
Write-Host "Registering manifest at path: $Local:ManifestPath"
Add-AppxPackage -Register $Local:ManifestPath

# Execute the tests from within the package's runtime.
$Local:TestExePath = Join-Path $BuildRoot "AppInstallerCLITests\AppInstallerCLITests.exe"
$Local:TestArgs = $Args

if ([String]::IsNullOrEmpty($LogTarget))
{
    $Local:TestArgs = $Local:TestArgs + " -log"
}
else
{
    $Local:TestArgs = $Local:TestArgs + " -logto ""$LogTarget"""
}

if (![String]::IsNullOrEmpty($TestResultsTarget))
{
    $Local:TestArgs = $Local:TestArgs + " -s -r junit -o ""$TestResultsTarget"""
}

if ($Wait)
{
    $Local:TestArgs = $Local:TestArgs + " -wait"
}

Write-Host "Executing tests at path: $Local:TestExePath"
Write-Host "Executing tests with args: $Local:TestArgs"
Invoke-CommandInDesktopPackage -PackageFamilyName AppInstallerCLI_8wekyb3d8bbwe -AppId AppInst -Command $Local:TestExePath -Args $Local:TestArgs
