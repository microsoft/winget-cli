# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$TargetLocation
)

$Local:logsFilePath = Join-Path $env:LOCALAPPDATA "Packages\WinGetDevCLI_8wekyb3d8bbwe\LocalState\DiagOutputDir"
$Local:packages = Get-AppxPackage -Name WinGetDevCLI -ErrorAction Stop

try
{
    New-Item -ItemType Directory -Path $TargetLocation -Force -ErrorAction Stop | Out-Null

    if (Test-Path -LiteralPath $Local:logsFilePath -PathType Container)
    {
        Write-Host "Collecting configuration OOP logs from $Local:logsFilePath"
        Copy-Item -LiteralPath $Local:logsFilePath -Destination $TargetLocation -Recurse -Force -ErrorAction Stop
    }
    else
    {
        Write-Warning "Configuration OOP log directory not found at $Local:logsFilePath"
    }

    $Local:servers = @(Get-CimInstance -ClassName Win32_Process -Filter "Name = 'WindowsPackageManagerServer.exe'" -ErrorAction Stop |
        Select-Object ProcessId, ParentProcessId, CreationDate, ExecutablePath, CommandLine)
    ConvertTo-Json -InputObject $Local:servers -Depth 3 |
        Set-Content -LiteralPath (Join-Path $TargetLocation "ServerProcesses.json") -Encoding UTF8 -ErrorAction Stop
}
finally
{
    # Package removal deletes LocalState, so diagnostics must be collected first.
    $Local:packages | Remove-AppxPackage -ErrorAction Stop
}
