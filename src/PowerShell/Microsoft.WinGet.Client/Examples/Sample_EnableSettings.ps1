<#
    .SYNOPSIS
        Example for 'Enable-WinGetSetting' and 'Disable-WinGetSetting' cmdlets.
        Cmdlet for enabling/disabling a specified WinGet setting. May require elevation.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

# Enables the 'LocalManifestFiles' setting.
Enable-WinGetSetting -Name LocalManifestFiles

# Disables the 'LocalManifestFiles' setting.
Disable-WinGetSetting -Name LocalManifestFiles 