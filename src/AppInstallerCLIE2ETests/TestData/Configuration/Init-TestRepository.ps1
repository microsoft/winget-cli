# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdletBinding()]
param(
    [string]$ModulesPath,

    [string]$RepositoryPath,

    [string]$RepositoryName,

    [switch]$Force
)

if ([System.String]::IsNullOrEmpty($ModulesPath))
{
    $ModulesPath = Join-Path $PSScriptRoot "Modules"
}

if ([System.String]::IsNullOrEmpty($RepositoryPath))
{
    $RepositoryPath = Join-Path ([System.IO.Path]::GetTempPath()) (New-Guid)
}

if ([System.String]::IsNullOrEmpty($RepositoryName))
{
    $RepositoryName = "AppInstallerCLIE2ETestsRepo"
}

if ($Force) {
    $null = New-Item -Path $RepositoryPath -ItemType Directory -Force
} else {
    $null = New-Item -Path $RepositoryPath -ItemType Directory -ErrorAction Inquire
}

$Local:existingRepository = Get-PSRepository -Name $RepositoryName -ErrorAction Ignore
if ($Local:existingRepository)
{
    if ($Force)
    {
        Unregister-PSRepository -Name $RepositoryName
    }
    else
    {
        throw "Repository named $RepositoryName is already registered. Use -Force to overwrite it."
    }
}

$null = Register-PSRepository -Name $RepositoryName -SourceLocation $RepositoryPath -ScriptSourceLocation $RepositoryPath

$Local:allItems = Get-ChildItem $ModulesPath

$Local:progressActivity = "Publishing modules to $RepositoryPath"
Write-Progress -Activity $Local:progressActivity

[Int32]$Local:modulesPublished = 0

$Local:allItems | ForEach-Object -Process {
    $Local:modulePath = $_.FullName
    Write-Verbose "Publishing $Local:modulePath"
    Publish-Module -Path $Local:modulePath -Repository $RepositoryName -Force
    $Local:modulesPublished += 1
    Write-Progress -Activity $Local:progressActivity -PercentComplete (($Local:modulesPublished * 100) / $Local:allItems.Count)
}

Write-Progress -Activity $Local:progressActivity -Completed
