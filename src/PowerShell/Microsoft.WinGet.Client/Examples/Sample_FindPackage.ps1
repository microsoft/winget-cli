<#
    .SYNOPSIS
        Example for 'Find-WinGetPackage' cmdlet.
        Displays all applications available for installation.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

# Find all available packages
Find-WinGetPackage

# Find package by name
Find-WinGetPackage -Name git

# Find 10 packages by name
Find-WinGetPackage -Name git -Count 10

# Find package by package identifier
Find-WinGetPackage -Id git.git

# Find exact package from a specific source.
Find-WinGetPackage -Id Git.Git -Source winget -Exact