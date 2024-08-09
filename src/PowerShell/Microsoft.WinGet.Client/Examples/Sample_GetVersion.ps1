<#
    .SYNOPSIS
        Example for 'Get-WinGetVersion' cmdlet.
        Prints the current client version.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

$version = Get-WinGetVersion

Write-Host($version);