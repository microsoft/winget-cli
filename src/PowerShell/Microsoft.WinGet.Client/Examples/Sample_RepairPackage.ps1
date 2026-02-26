<#
    .SYNOPSIS
        Example for 'Repair-WinGetPackage' cmdlet.
        Repairs the specified application.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

# Repair a package by name
Repair-WinGetPackage -Name "PowerToys FileLocksmith Context Menu"

# Repair a package by version and package identifier.
Repair-WinGetPackage -Id "MSIX\Microsoft.PowerToys.FileLocksmithContextMenu_1.0.0.0_neutral__8wekyb3d8bbwe"

# Repair a package from a specific source
Repair-WinGetPackage -Id "MSIX\Microsoft.PowerToys.FileLocksmithContextMenu_1.0.0.0_neutral__8wekyb3d8bbwe" -Source winget
