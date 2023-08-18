# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdletBinding()]
param(
    [string]$testModulesPath,
    [string]$outputPath,
    [string]$packageLayoutPath
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
    $local:settingsExport = ConvertFrom-Json (wingetdev.exe settings export)
    $local:settingsFilePath = $local:settingsExport.userSettingsFile
    $local:settingsFileContent = ConvertTo-Json @{ debugging= @{ enableSelfInitiatedMinidump=$true ; keepAllLogFiles=$true } }

    Set-Content -Path $local:settingsFilePath -Value $local:settingsFileContent
}

Invoke-Pester -Script $PSScriptRoot\Microsoft.WinGet.Client.Tests.ps1 -OutputFile $outputPath\Tests-WinGetClient.XML -OutputFormat NUnitXML
Invoke-Pester -Script $PSScriptRoot\Microsoft.WinGet.Configuration.Tests.ps1 -OutputFile $outputPath\Tests-WinGetConfiguration.XML -OutputFormat NUnitXML
