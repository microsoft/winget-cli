<#
    .SYNOPSIS
        Example for 'Get-WinGetSource', 'Add-WinGetSource', 'Remove-WinGetSource', and 'Reset-WinGetSource' cmdlet.
        Cmdlets to allow you to manage sources for the Windows Package Manager.
#>

# TODO: Replace parameter with actual module name from PSGallery once module is released. 
Param (
    [Parameter(Mandatory)]
    $ModulePath
)

Import-Module -Name $ModulePath

# List current sources.
Get-WinGetSource

# Add REST source
Add-WinGetSource -Name 'Contoso' -Argument 'https://www.contoso.com/cache' -Type 'Microsoft.Rest'

# Remove source by name
Remove-WinGetSource -Name 'Contoso'

# Reset to default sources
Reset-WinGetSource