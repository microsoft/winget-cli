# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdletBinding()]
param(
    [string]$testModulesPath,
    [string]$outputPath,
    [string]$packageLayoutPath,
    [switch]$TargetProduction,
    [string]$ConfigurationTestDataPath
)

# This updates pester not always necessary but worth noting
Install-Module -Name Pester -Force -SkipPublisherCheck
Import-Module Pester

if (-not [System.String]::IsNullOrEmpty($testModulesPath))
{
    $env:PSModulePath += ";$testModulesPath"
}

if (-not (Test-Path $outputPath))
{
    New-Item -Path $outputPath -ItemType Directory
}
else
{
    Remove-Item $outputPath\* -Recurse -Force
}

# Register the package
if (-not [System.String]::IsNullOrEmpty($packageLayoutPath))
{
    $local:packageManifestPath = Join-Path $packageLayoutPath "AppxManifest.xml"

    Import-Module Appx -UseWindowsPowerShell
    Add-AppxPackage -Register $local:packageManifestPath

    # Configure crash dump and log file settings
    if ($TargetProduction)
    {
        $local:wingetExeName = "winget.exe"
    }
    else
    {
        $local:wingetExeName = "wingetdev.exe"
    }
    
    $local:settingsExport = ConvertFrom-Json (& $local:wingetExeName settings export)
    $local:settingsFilePath = $local:settingsExport.userSettingsFile
    $local:settingsFileContent = ConvertTo-Json @{ debugging= @{ enableSelfInitiatedMinidump=$true ; keepAllLogFiles=$true } }

    Set-Content -Path $local:settingsFilePath -Value $local:settingsFileContent
}

$clientConfig = New-PesterConfiguration
$clientConfig.TestResult.OutputFormat = "NUnitXML"
$clientConfig.TestResult.OutputPath = "$outputPath\Tests-WinGetClient.XML"
$clientConfig.TestResult.Enabled = $true
$clientConfig.Run.Container = New-PesterContainer -Path "$PSScriptRoot\Microsoft.WinGet.Client.Tests.ps1" -Data @{ TargetProduction = $TargetProduction }

Invoke-Pester -Configuration $clientConfig

if ($PSEdition -eq "Core")
{
    $configConfig = New-PesterConfiguration
    $configConfig.TestResult.OutputFormat = "NUnitXML"
    $configConfig.TestResult.OutputPath = "$outputPath\Tests-WinGetConfiguration.XML"
    $configConfig.TestResult.Enabled = $true
    $configConfig.Run.Container = New-PesterContainer -Path "$PSScriptRoot\Microsoft.WinGet.Configuration.Tests.ps1" -Data @{ ConfigurationTestDataPath = $ConfigurationTestDataPath }

    Invoke-Pester -Configuration $configConfig

    $dscConfig = New-PesterConfiguration
    $dscConfig.TestResult.OutputFormat = "NUnitXML"
    $dscConfig.TestResult.OutputPath = "$outputPath\Tests-WinGetDSC.XML"
    $dscConfig.TestResult.Enabled = $true
    $dscConfig.Run.Container = New-PesterContainer -Path "$PSScriptRoot\Microsoft.WinGet.DSC.Tests.ps1"

    Invoke-Pester -Configuration $dscConfig
}
