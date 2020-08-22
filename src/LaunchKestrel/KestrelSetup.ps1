<#
.SYNOPSIS
    Launches Kestrel Server for Serving Static Files for E2E Testing Purposes
.DESCRIPTION
    Updates Hash for Testing Manifests, Recreates Source Package, and Launches Kestrel Server to Begin Hosting Static Files
.PARAMETER TestEXEInstallerPath
    The file path to the EXE Installer
.PARAMETER TestMSIInstallerPath
    The file path to the MSI Installer
.PARAMETER TestMSIXInstallerPath
    The file path to the MSIX Installer
.PARAMETER ManifestsPath
    Path to directory with Testing Manifests
#>

$Local:EXEInstallerHash
$Local:MSIInstallerHash
$Local:MSIXInstallerHash
$Local:PathToManifests = 'src'