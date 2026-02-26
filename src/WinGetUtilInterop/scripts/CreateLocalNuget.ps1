# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Creates a local Microsoft.WindowsPackageManager.Utils nuget package and add it to a local nuget feed.
#>

[CmdletBinding()]
param (
    [Parameter(Mandatory)]
    [ValidateSet("Debug", "Release", "ReleaseStatic")]
    [string]
    $Configuration,

    [Parameter(Mandatory)]
    [string]
    $NugetVersion,

    [string]
    $BuildRoot = "",

    [string]
    $LocalNugetSource = ""
)

if ($BuildRoot -eq "")
{
    $BuildRoot = "$PSScriptRoot\..\..";
}

$local:repoPath = "$PSScriptRoot\..\..\..\"

# Create all directories and copy files in location expected from the nuspec.
# The paths contains 'release' but it whatever configuration the param is.
$local:nugetWorkingDir = "$PSScriptRoot\NugetFiles"
$local:x64NugetPath = "$nugetWorkingDir\Build.x64release\src\x64\Release\WinGetUtil"
$local:x86NugetPath = "$nugetWorkingDir\Build.x86release\src\x86\Release\WinGetUtil"
$local:manifestsNugetPath = "$nugetWorkingDir\Build.x64release\schemas\JSON\manifests"
$local:interopNugetPath = "$nugetWorkingDir\Build.x64release\src\WinGetUtilInterop\bin\Release\netstandard2.1"
$local:targetsNugetPath = "$nugetWorkingDir\Build.x64release\src\WinGetUtilInterop\build"

Write-Host "Prepare nuget files"
if (Test-Path $nugetWorkingDir)
{
    Remove-Item $nugetWorkingDir -Recurse
}
New-Item $nugetWorkingDir -ItemType directory | Out-Null
New-Item $x64NugetPath -ItemType directory | Out-Null
New-Item $x86NugetPath -ItemType directory | Out-Null
New-Item $manifestsNugetPath -ItemType directory | Out-Null
New-Item $interopNugetPath -ItemType directory | Out-Null
New-Item $targetsNugetPath -ItemType directory | Out-Null

function CopyFile([string]$in, [string]$out)
{
    $copyErrors = $null
    Copy-Item $in $out -Force -ErrorVariable copyErrors -ErrorAction SilentlyContinue
    $copyErrors | ForEach-Object { Write-Warning $_ }
}

CopyFile "$BuildRoot\x64\$Configuration\WinGetUtil\WinGetUtil.dll" "$x64NugetPath\WinGetUtil.dll"
CopyFile "$BuildRoot\x64\$Configuration\WinGetUtil\WinGetUtil.pdb" "$x64NugetPath\WinGetUtil.pdb"
CopyFile "$BuildRoot\x86\$Configuration\WinGetUtil\WinGetUtil.dll" "$x86NugetPath\WinGetUtil.dll"
CopyFile "$BuildRoot\x86\$Configuration\WinGetUtil\WinGetUtil.pdb" "$x86NugetPath\WinGetUtil.pdb"
CopyFile "$repoPath\src\WinGetUtilInterop\bin\$Configuration\netstandard2.1\WinGetUtilInterop.dll" "$interopNugetPath\WinGetUtilInterop.dll"
CopyFile "$repoPath\src\WinGetUtilInterop\bin\$Configuration\netstandard2.1\WinGetUtilInterop.pdb" "$interopNugetPath\WinGetUtilInterop.pdb"
CopyFile "$repoPath\src\WinGetUtilInterop\build\Microsoft.WindowsPackageManager.Utils.targets" "$targetsNugetPath\Microsoft.WindowsPackageManager.Utils.targets"
CopyFile "$PSScriptRoot\WinGetUtilDev.nuspec" "$nugetWorkingDir\WinGetUtilDev.nuspec"
Copy-Item "$repoPath\schemas\JSON\manifests" $manifestsNugetPath -Recurse

# Create nuget
Write-Host "Creating nuget package"
$local:result = nuget pack .\NugetFiles\WinGetUtilDev.nuspec -Version $NugetVersion -OutputDirectory NugetOut
$local:outFile = $result -match "nupkg"
$outFile = $outFile[0]
$outFile = $outFile.Substring($outFile.IndexOf("'") + 1, $outFile.LastIndexOf("'") - $outFile.IndexOf("'") - 1)
Write-Host "Created $outFile"

if ($LocalNugetSource -ne "")
{
    Write-Host "Adding $outFile to local nuget feed"
    nuget add $outFile -Source $LocalNugetSource
}

