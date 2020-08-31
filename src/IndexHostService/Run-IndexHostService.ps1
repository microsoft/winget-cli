<#
.SYNOPSIS
    Runs the IndexHostService.
.PARAMETER BuildRoot
    The root of the build output directory for IndexHostService
.PARAMETER IndexRoot
    The root of the index root to be served through LocalHost
.PARAMETER CertPath
    Path to HTTPS Development Certificate File (pfx)
.PARAMETER CertPassword
    Secure Password for HTTPS Certificate
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$BuildRoot,

    [Parameter(Mandatory=$true)]
    [string]$LocalIndexRoot,

    [Parameter(Mandatory=$true)]
    [string]$CertPath,

    [Parameter(Mandatory=$true)]
    [string]$CertPassword
)

cd $BuildRoot

Start-Process -FilePath 'dotnet' -ArgumentList 'run -d $LocalIndexRoot -c $CertPath -p $CertPassword'
