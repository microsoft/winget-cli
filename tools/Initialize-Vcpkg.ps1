<#
.SYNOPSIS
    Enables user-wide MSBuild integration for Visual Studio's bundled vcpkg.
#>

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
