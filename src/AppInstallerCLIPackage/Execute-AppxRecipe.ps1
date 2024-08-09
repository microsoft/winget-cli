# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true,Position=0)]
    [string]$AppxRecipePath,

    [string]$LayoutPath,

    [switch]$Force
)

[xml]$Local:recipe = Get-Content $AppxRecipePath

if ([System.String]::IsNullOrEmpty($LayoutPath))
{
    $LayoutPath = $Local:recipe.Project.PropertyGroup.LayoutDir
}

$Local:namespace = @{
    ns = "http://schemas.microsoft.com/developer/msbuild/2003"
}

$Local:manifestElement = Select-Xml -Xml $Local:recipe -Namespace $Local:namespace -XPath "//ns:AppXManifest" | Select-Object -ExpandProperty Node
$Local:packageFileElements = Select-Xml -Xml $Local:recipe -Namespace $Local:namespace -XPath "//ns:AppxPackagedFile" | Select-Object -ExpandProperty Node

$Local:allItems = $Local:packageFileElements + $Local:manifestElement

$Local:progressActivity = "Copying files to $LayoutPath"
Write-Progress -Activity $Local:progressActivity

if ($Force) {
    $null = New-Item -Path $LayoutPath -ItemType Directory -Force
} else {
    New-Item -Path $LayoutPath -ItemType Directory -ErrorAction Inquire
}

[Int32]$Local:filesCopied = 0

$Local:allItems | ForEach-Object -Process {
    $Local:sourcePath = $_.Include
    $Local:destinationPath = Join-Path $LayoutPath $_.PackagePath
    Write-Verbose "$Local:sourcePath => $Local:destinationPath"
    $null = New-Item -Path ([System.IO.Path]::GetDirectoryName($Local:destinationPath)) -ItemType Directory -Force
    Copy-Item -Path $Local:sourcePath -Destination $Local:destinationPath
    $Local:filesCopied += 1
    Write-Progress -Activity $Local:progressActivity -PercentComplete (($Local:filesCopied * 100) / $Local:allItems.Count)
}

Write-Progress -Activity $Local:progressActivity -Completed
