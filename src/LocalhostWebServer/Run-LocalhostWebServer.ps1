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
.PARAMETER OutCertFile
    Export cert location.
.PARAMETER LocalSourceJson
    Local source json definition
.PARAMETER SourceCert
    The certificate of the source package.
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$BuildRoot,

    [Parameter(Mandatory=$true)]
    [string]$StaticFileRoot,

    [Parameter(Mandatory=$true)]
    [string]$CertPath,

    [Parameter()]
    [string]$CertPassword,

    [Parameter()]
    [string]$OutCertFile,

    [Parameter()]
    [string]$LocalSourceJson,

    [Parameter()]
    [string]$SourceCert,

    [Parameter()]
    [string]$TestDataPath,

    [Parameter()]
    [switch]$ExitBeforeRun
)

if (-not [System.String]::IsNullOrEmpty($sourceCert))
{
    # Requires admin
    & certutil.exe -addstore -f "TRUSTEDPEOPLE" $sourceCert
}

Push-Location $BuildRoot

$startProcessArguments = @{
    FilePath = Join-Path $BuildRoot "LocalhostWebServer.exe"
    ArgumentList = "StaticFileRoot=$StaticFileRoot CertPath=$CertPath CertPassword=$CertPassword OutCertFile=$OutCertFile LocalSourceJson=$LocalSourceJson TestDataPath=$TestDataPath ExitBeforeRun=$ExitBeforeRun"
    PassThru = $true
}

if (-not [System.string]::IsNullOrEmpty($env:artifactsDir))
{
    $startProcessArguments.RedirectStandardOutput = Join-Path $env:artifactsDir "LocalhostWebServer.out"
    $startProcessArguments.RedirectStandardError = Join-Path $env:artifactsDir "LocalhostWebServer.err"
}

$Local:process = Start-Process @startProcessArguments

if ($ExitBeforeRun)
{
    Wait-Process -InputObject $Local:process
}

Pop-Location
