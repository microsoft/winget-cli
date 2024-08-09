# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Check that we are running as an administrator
function Assert-IsAdministrator
{
    $windowsIdentity = [System.Security.Principal.WindowsIdentity]::GetCurrent()
    $windowsPrincipal = New-Object -TypeName 'System.Security.Principal.WindowsPrincipal' -ArgumentList @( $windowsIdentity )

    $adminRole = [System.Security.Principal.WindowsBuiltInRole]::Administrator

    if (-not $windowsPrincipal.IsInRole($adminRole))
    {
        New-InvalidOperationException -Message "This resource must run as an Administrator."
    }
}

# Verify the command is present in the Microsoft.WinGet.Client Module
function Assert-WinGetCommand([string]$cmdletName)
{
    $null = Get-Command -Module "Microsoft.WinGet.Client" -Name $cmdletName -ErrorAction Stop
}
