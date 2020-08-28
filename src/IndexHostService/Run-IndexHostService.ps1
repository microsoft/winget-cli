<#
.SYNOPSIS
    Runs the IndexHostService.
.PARAMETER BuildRoot
    The root of the build output directory for IndexHostService
.PARAMETER IndexRoot
    The root of the index root to be served through LocalHost
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$BuildRoot,

    [Parameter(Mandatory=$true)]
    [string]$LocalIndexRoot
)

cd $BuildRoot

dotnet dev-certs https -ep cert.pfx -p microsoft

Start-Process -FilePath "dotnet" -ArgumentList "IndexHostService.dll $LocalIndexRoot 5001 cert.pfx microsoft"