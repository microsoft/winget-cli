<#
    .SYNOPSIS
        Performs a WinGet Configuration asynchronously.
        Use Start-WinGetConfiguration to perform a WinGet Configuration.
#>

param (
    [Parameter(Mandatory)]
    $configFile
)

# TODO: Call install module if not installed once is published.
Import-Module Microsoft.WinGet.Configuration

# Starts the configuration in the background
$configJob = Get-WinGetConfiguration -File $configFile | Start-WinGetConfiguration -Set $configSet

# This will block until the configuration is completed. Or print the results if already one.
Complete-WinGetConfiguration -ConfigurationJob $job
