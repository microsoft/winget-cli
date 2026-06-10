<#
    .SYNOPSIS
        Example for 'Install-WinGetPackage' cmdlet.
        Installs the specified application based on the provided arguments.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

# Install a package by name
Install-WinGetPackage -Name powertoys

# Install a package by version and package identifier.
Install-WinGetPackage -Id Microsoft.PowerToys -Version 0.15.2

# Install a package from a specific source
Install-WinGetPackage -Id Microsoft.PowerToys -Source winget

# Install a package with a specific architecture and scope.
Install-WinGetPackage -Id Microsoft.PowerToys -Architecture X64 -Scope User
