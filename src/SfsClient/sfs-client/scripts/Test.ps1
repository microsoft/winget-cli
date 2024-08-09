# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

<#
.SYNOPSIS
Simplifies test commands for the SFS Client.

.DESCRIPTION
This script will contain the test commands for the SFS Client.
Use this on Windows platforms in a PowerShell session.

.EXAMPLE
PS> ./scripts/Test.ps1
#>
param (
    [switch] $OutputOnFailure = $false
)

$GitRoot = (Resolve-Path (&git -C $PSScriptRoot rev-parse --show-toplevel)).Path
$BuildFolder = "$GitRoot/build"

$cmd = "ctest --test-dir ""$BuildFolder/client"""
if ($OutputOnFailure)
{
    $cmd += " --output-on-failure"
}

Invoke-Expression $cmd
