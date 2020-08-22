<#
.SYNOPSIS
    Launches Kestrel Server for Serving Static Files for E2E Testing Purposes
.DESCRIPTION
    Updates Hash for Testing Manifests, Recreates Source Package, and Launches Kestrel Server to Begin Hosting Static Files
.PARAMETER ExeInstallerPath
    The file path to the EXE Installer
.PARAMETER MsiInstallerPath
    The file path to the MSI Installer
.PARAMETER MsixInstallerPath
    The file path to the MSIX Installer
.PARAMETER ManifestsDirectory
    Path to directory with Testing Manifests
#>

param(
    [Parameter(Mandatory=$false)]
    [string]$ExeInstallerPath,
    
    [Parameter(Mandatory=$false)]
    [string]$MsiInstallerPath,
    
    [Parameter(Mandatory=$false)]
    [string]$MsixInstallerPath,
    
    [Parameter(Mandatory=$false)]
    [string]$ManifestsDirectory,
)

LaunchKestrel.exe -d $ManifestsDirectory -e $ExeInstallerPath -m $MsiInstallerPath -x $MsixInstallerPath -s 'src'

---------------------------------------------------------------------------------------
  - task: PowerShell@2
    displayName: Launch Kestrel Local Server x86
    inputs:
      filePath: 'src\LaunchKestrel\KestrelSetup.ps1'
      arguments: '-ExeInstallerPath [] -MsiInstallerPath [] -MsixInstallerPath [] -ManifestsDirectory []'
      workingDirectory: 'src\x86\Release\LaunchKestrel\'
    condition: succeededOrFailed()