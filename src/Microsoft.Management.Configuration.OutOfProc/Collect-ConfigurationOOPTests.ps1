# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdletBinding()]
param(
    [string]$TargetLocation
)

$Local:settingsExport = ConvertFrom-Json (wingetdev.exe settings export)
$Local:logsFilePath = Join-Path (Split-Path $Local:settingsExport.userSettingsFile -Parent) "DiagOutputDir"

Get-AppxPackage WinGetDevCLI | Remove-AppxPackage

Copy-Item $Local:logsFilePath $TargetLocation -Recurse -Force -ErrorAction Ignore
