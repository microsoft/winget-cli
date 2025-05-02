# Helper functions for dealing with the port overlay

$OverlayRoot = $PSScriptRoot

$ErrorActionPreference = "Stop"

# Copies an port from the official registry to this overlay
function New-PortOverlay
{
    param(
        [Parameter(Mandatory)]
        $Port
    )

    $portDir = Join-Path $OverlayRoot $Port

    # Delete existing port if needed
    if (Test-Path $portDir)
    {
        Remove-Item -Force -Recurse $portDir
    }

    New-Item -Type Directory $portDir | Out-Null

    # Hacky way of getting a single directory from the vcpkg repo:
    # - Download the vcpkg repo as a zip to a memory stream
    # - Parse the zip archive
    # - Extract the files we want
    $vcpkgZipUri = "https://github.com/microsoft/vcpkg/archive/refs/heads/master.zip"
    $response = Invoke-WebRequest -Uri $vcpkgZipUri
    $zipStream = [System.IO.MemoryStream]::new($response.Content)
    $zipArchive = [System.IO.Compression.ZipArchive]::new($zipStream)

    # Remove length=0 to ignore the directory itself
    $portZipEntries = $zipArchive.Entries |
        Where-Object { ($_.Length -ne 0) -and $_.FullName.StartsWith("vcpkg-master/ports/$Port/") }

    if (-not $portZipEntries)
    {
        throw "Port $port not found"
    }
    
    foreach ($zipEntry in $portZipEntries)
    {
        $targetPath = Join-Path $portDir $zipEntry.Name
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($zipEntry, $targetPath)
    }
}