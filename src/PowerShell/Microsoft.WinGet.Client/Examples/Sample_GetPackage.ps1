<#
    .SYNOPSIS
        Example for 'Get-WinGetPackage' cmdlet.
        Displays a list of the applications currently installed on your computer.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

# List all installed packages
Get-WinGetPackage

# List installed package by name
Get-WinGetPackage -Name git

# List 10 packages by name
Get-WinGetPackage -Name git -Count 10

# List package by package identifier
Get-WinGetPackage -Id git.git

# List exact package from a specific source.
Get-WinGetPackage -Id Git.Git -Source winget -Exact