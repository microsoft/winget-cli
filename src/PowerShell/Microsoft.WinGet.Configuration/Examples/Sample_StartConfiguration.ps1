<#
    .SYNOPSIS
        Performs a WinGet Configuration asynchronously.
        Use Start-WinGetConfiguration to perform a WinGet Configuration.
#>

param (
    [Parameter(Mandatory)]
    $configFile
)

if (-not(Get-Module -Name Microsoft.WinGet.Configuration -ListAvailable))
{
    Install-Module Microsoft.WinGet.Configuration -AllowPrerelease
}

Import-Module Microsoft.WinGet.Configuration

# Starts the configuration in the background
$configJob = Get-WinGetConfiguration -File $configFile | Start-WinGetConfiguration

# This will block until the configuration is completed. Or print the results if already done.
Complete-WinGetConfiguration -ConfigurationJob $configJob
