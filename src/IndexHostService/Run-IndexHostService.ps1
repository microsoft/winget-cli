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

dotnet .\IndexHostService.dll $LocalIndexRoot