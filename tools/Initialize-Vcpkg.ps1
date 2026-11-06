<#
.SYNOPSIS
    Enables MSBuild integration for Visual Studio's bundled vcpkg and optionally restores dependencies.
#>

[CmdletBinding()]
param(
    [ValidateNotNullOrEmpty()]
    [string]$Triplet
)

$ErrorActionPreference = 'Stop'

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf))
{
    throw "vswhere.exe not found at $vswhere."
}

$vcpkg = & $vswhere -latest -products '*' `
    -requires Microsoft.VisualStudio.Component.Vcpkg `
    -find 'VC\vcpkg\vcpkg.exe'
if ($LASTEXITCODE -ne 0)
{
    throw "vswhere.exe failed with exit code $LASTEXITCODE."
}

if (-not $vcpkg -or -not (Test-Path -LiteralPath $vcpkg -PathType Leaf))
{
    throw 'No Visual Studio installation with vcpkg.exe found.'
}

Write-Host "Running: `"$vcpkg`" integrate install"
& $vcpkg integrate install
if ($LASTEXITCODE -ne 0)
{
    throw "vcpkg integrate install failed with exit code $LASTEXITCODE."
}

if ($Triplet)
{
    $sourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\src')).Path
    $overlayScript = Join-Path $sourceRoot 'VcpkgPortOverlay\CreatePortOverlay.ps1'
    $overlayStamp = Join-Path $sourceRoot 'VcpkgPortOverlay\.created'

    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $overlayScript -StampFile $overlayStamp
    if ($LASTEXITCODE -ne 0)
    {
        throw "Creating the vcpkg port overlay failed with exit code $LASTEXITCODE."
    }

    # Restore outside MSBuild so recovered download errors are not parsed as build failures.
    Write-Host "Restoring vcpkg dependencies for $Triplet"
    & $vcpkg install --x-wait-for-lock --triplet $Triplet `
        "--x-manifest-root=$sourceRoot" "--x-install-root=$sourceRoot\vcpkg_installed"
    if ($LASTEXITCODE -ne 0)
    {
        throw "vcpkg install failed with exit code $LASTEXITCODE."
    }
}
