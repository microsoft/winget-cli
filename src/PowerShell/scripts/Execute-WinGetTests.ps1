# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
[CmdletBinding()]
param(
    # The version of the client PS module to use.
    [string]$ClientModuleVersion,

    # The version of the configuration PS module to use.
    [string]$ConfigurationModuleVersion,
    
    # The version of the client to use. Use 'existing' to skip updating.
    [string]$ClientVersion,
    
    # The version of Powershell to use. Use 'existing' to skip updating.
    [string]$PwshVersion,
    
    # The path to the binaries to run the web server.
    [string]$LocalhostWebServerPath = (Join-Path $PSScriptRoot "..\LocalhostWebServer"),
    
    # The path to the files to be hosted by the web server.
    [string]$HostedFilePath = (Join-Path $PSScriptRoot "..\TestLocalIndex"),
    
    # The path to the certificate that signed the source.msix package.
    [string]$SourceCertPath = (Join-Path $PSScriptRoot "..\TestData\AppInstallerTest.cer"),

    # The path to the configuration test data.
    [string]$ConfigurationTestDataPath = (Join-Path $PSScriptRoot "..\TestData\Configuration"),

    # The path to pwsh.exe
    [string]$PwshPath = 'C:\Program Files\PowerShell\7\pwsh.exe',

    # Switches to prevent installing various runtimes required for the tests
    [switch]$SkipVCRuntime,
    [switch]$SkipDotNetRuntime,
    [switch]$SkipAspNetRuntime,
    
    # The path to write results to.
    [string]$ResultsPath = (Join-Path ([System.IO.Path]::GetTempPath()) (New-Guid))
)

# Ensure we can connect to PS Gallery
Install-PackageProvider -Name NuGet -Force | Out-Null

# Get the client module
if ([System.String]::IsNullOrEmpty($ClientModuleVersion))
{
    Install-Module Microsoft.WinGet.Client -Force
}
else
{
    Install-Module Microsoft.WinGet.Client -RequiredVersion $ClientModuleVersion -Force
}

# Get the client
if ([System.String]::IsNullOrEmpty($ClientVersion))
{
    Repair-WingetPackageManager -Latest
}
elseif ($ClientVersion -eq "existing")
{
    # Use version already present
}
else
{
    Repair-WingetPackageManager -Version $ClientVersion
}

# Get pwsh
if ([System.String]::IsNullOrEmpty($PwshVersion))
{
    winget install Microsoft.PowerShell -s winget
}
elseif ($PwshVersion -eq "existing")
{
    # Use version already present
}
else
{
    winget install Microsoft.PowerShell -s winget -v $PwshVersion
}

# Get VC Runtime
if (-not $SkipVCRuntime)
{
    winget install 'Microsoft.VCRedist.2015+.x64' -s winget
}

# Install .NET 6 for the local web server
if (-not $SkipDotNetRuntime)
{
    winget install Microsoft.DotNet.Runtime.6 -s winget
}

if (-not $SkipAspNetRuntime)
{
    winget install Microsoft.DotNet.AspNetCore.6 -s winget
}

# Generate a new TLS certificate and trust it
$TLSCertificate = New-SelfSignedCertificate -CertStoreLocation "Cert:\LocalMachine\My" -DnsName "localhost"
$CertTempPath = Join-Path $env:TEMP New-Guid
Export-Certificate -Cert $TLSCertificate -FilePath $CertTempPath
Import-Certificate -FilePath $CertTempPath -CertStoreLocation "Cert:\LocalMachine\Root"

# Start local host web server
.\Run-LocalhostWebServer.ps1 -BuildRoot $LocalhostWebServerPath -StaticFileRoot $HostedFilePath -CertPath $TLSCertificate.PSPath -SourceCert $SourceCertPath

# Get the configuration module using pwsh since AllowPrerelease doesn't working in Windows PowerShell baseline
if ([System.String]::IsNullOrEmpty($ConfigurationModuleVersion))
{
    & $PwshPath -Command "Install-Module Microsoft.WinGet.Configuration -Force -AllowPrerelease"
}
else
{
    & $PwshPath -Command "Install-Module Microsoft.WinGet.Configuration -RequiredVersion $ConfigurationModuleVersion -Force -AllowPrerelease"
}

# Create local PS repo
$InitRepositoryPath = Join-Path $ConfigurationTestDataPath Init-TestRepository.ps1
& $PwshPath -ExecutionPolicy Unrestricted -Command "$InitRepositoryPath -Force"

# Run tests
& $PwshPath -ExecutionPolicy Unrestricted -Command ".\RunTests.ps1 -TargetProduction -ConfigurationTestDataPath $ConfigurationTestDataPath -outputPath $ResultsPath"

# Terminate the local web server
Get-Process LocalhostWebServer -ErrorAction Ignore | Stop-Process -ErrorAction Ignore

Write-Host "Results: $ResultsPath"
