# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdletBinding()]
param(
    [string]$BuildOutputPath,

    [string]$PackageLayoutPath
)

# Copy the winmd into the unit test directory since it will be needed for marshalling
$Local:winmdSourcePath = Join-Path $BuildOutputPath "Microsoft.Management.Configuration\Microsoft.Management.Configuration.winmd"
$Local:winmdTargetPath = Join-Path $BuildOutputPath "Microsoft.Management.Configuration.UnitTests\net8.0-windows10.0.22000.0\Microsoft.Management.Configuration.winmd"

Copy-Item $Local:winmdSourcePath $Local:winmdTargetPath -Force

# Copy the OOP helper dll into the unit test directory to make activation look the same as in-proc
$Local:dllSourcePath = Join-Path $BuildOutputPath "Microsoft.Management.Configuration.OutOfProc\Microsoft.Management.Configuration.OutOfProc.dll"
$Local:dllTargetPath = Join-Path $BuildOutputPath "Microsoft.Management.Configuration.UnitTests\net8.0-windows10.0.22000.0\Microsoft.Management.Configuration.dll"

Copy-Item $Local:dllSourcePath $Local:dllTargetPath -Force

# Register the package
if (-not [System.String]::IsNullOrEmpty($PackageLayoutPath))
{
    $Local:packageManifestPath = Join-Path $PackageLayoutPath "AppxManifest.xml"

    Add-AppxPackage -ForceApplicationShutdown -Register $Local:packageManifestPath

    # Configure crash dump and log file settings
    $Local:settingsExport = ConvertFrom-Json (wingetdev.exe settings export)
    $Local:settingsFilePath = $Local:settingsExport.userSettingsFile
    $Local:settingsFileContent = ConvertTo-Json @{ debugging= @{ enableSelfInitiatedMinidump=$true ; keepAllLogFiles=$true } ; experimentalFeatures= @{ configuration03=$true } }

    Set-Content -Path $Local:settingsFilePath -Value $Local:settingsFileContent
}
