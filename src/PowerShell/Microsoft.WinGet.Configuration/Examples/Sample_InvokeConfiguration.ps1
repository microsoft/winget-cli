<#
    .SYNOPSIS
        Performs a WinGet Configuration and synchronously wait for its completion.
        Use Invoke-WinGetConfiguration to perform a WinGet Configuration.
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

$configSet = Get-WinGetConfiguration -File $configFile

# Calling this cmdlet is not required, but it will performed by Invoke/Start
# if not done before.
$configSet = Get-WinGetConfigurationDetails -Set $configSet

# Optionally pass -AcceptConfigurationAgreements to accept the agreements.
Invoke-WinGetConfiguration -Set $configSet
