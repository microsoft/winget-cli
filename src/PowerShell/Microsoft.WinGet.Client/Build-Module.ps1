# Copyright (c) Microsoft Corporation. Licensed under the MIT License.

[CmdletBinding()]
param (
    [Parameter()]
    $In = "$PSScriptRoot\bin",

    [Parameter()]
    [string]
    $Out = "$PSScriptRoot\build\bin",

    [Parameter()]
    [string]
    $Configuration = 'Release'
)

$CoreFramework = 'net5.0-windows10.0.22000.0'
$DesktopFramework = 'net461'

if (Test-Path $Out) {
    Remove-Item $Out -Recurse
}

Get-ChildItem $In -ErrorAction SilentlyContinue -Exclude 'Debug','Release' | ForEach-Object {
    Write-Verbose ($_.FullName)

    $coreSrcPath = Join-Path (Join-Path $_.FullName $Configuration) $CoreFramework
    $coreDstPath = Join-Path (Join-Path $Out $_.Name) 'Core'
    $desktopSrcPath = Join-Path (Join-Path $_.FullName $Configuration) $DesktopFramework
    $desktopDstPath = Join-Path (Join-Path $Out $_.Name) 'Desktop'

    Copy-Item $coreSrcPath -Destination $coreDstPath -Recurse -Force
    Copy-Item $desktopSrcPath -Destination $desktopDstPath -Recurse -Force
}

Copy-Item 'Format.ps1xml' -Destination $Out
Copy-Item 'Microsoft.WinGet.Client.psd1' -Destination $Out
Copy-Item 'Microsoft.WinGet.Client.psm1' -Destination $Out
