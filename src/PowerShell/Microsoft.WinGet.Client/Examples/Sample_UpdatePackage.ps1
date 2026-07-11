<#
    .SYNOPSIS
        Example for 'Update-WinGetPackage' cmdlet.
        Updates the specified application.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

# Update a package by name
Update-WinGetPackage -Name powertoys

# Update a package by version and package identifier.
Update-WinGetPackage -Id Microsoft.PowerToys -Version 0.15.2

# Update a package with silent mode
Update-WinGetPackage -Id Microsoft.PowerToys -Mode Silent

# Force update a package
Update-WinGetPackage -Id Microsoft.PowerToys -Force