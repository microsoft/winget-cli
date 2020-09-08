<#
.SYNOPSIS
    Runs the LocalhostWebServer.
.PARAMETER BuildRoot
    The root of the build output directory for LocalhostWebServer
.PARAMETER StaticFileRoot
    The root of the static files to be served through Localhost
.PARAMETER CertPath
    Path to HTTPS Development Certificate File (pfx)
.PARAMETER CertPassword
    Secure Password for HTTPS Certificate
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$BuildRoot,

    [Parameter(Mandatory=$true)]
    [string]$StaticFileRoot,

    [Parameter(Mandatory=$true)]
    [string]$CertPath,

    [Parameter(Mandatory=$true)]
    [string]$CertPassword
)

cd $BuildRoot

Start-Process -FilePath "LocalhostWebServer.exe" -ArgumentList "-d $StaticFileRoot -c $CertPath -p $CertPassword" 

