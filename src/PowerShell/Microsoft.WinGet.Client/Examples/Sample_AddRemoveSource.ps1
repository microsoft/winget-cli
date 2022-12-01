<#
    .SYNOPSIS
        Imports the Microsoft.WinGet.Client powershell module and demonstrates how to add, remove, and reset the sources.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

Get-WinGetSource

Remove-WinGetSource -Name winget

Add-WinGetSource -Name 'winget' -Argument 'https://cdn.winget.microsoft.com/cache'

Remove-WinGetSource -Name winget

Reset-WinGetSource

$sources = Get-WinGetSource

Write-Host($sources)