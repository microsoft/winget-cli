<#
    .SYNOPSIS
        Example for 'Uninstall-WinGetPackage' cmdlet.
        Uninstalls the specified application.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

# Uninstall a package by name
Uninstall-WinGetPackage -Name powertoys

# Uninstall a package by version and package identifier.
Uninstall-WinGetPackage -Id Microsoft.PowerToys -Version 0.15.2

# Uninstall a package from a specific source
Uninstall-WinGetPackage -Id Microsoft.PowerToys -Source winget
