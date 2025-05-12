# Helper functions for dealing with the port overlay

$OverlayRoot = $PSScriptRoot

$ErrorActionPreference = "Stop"

# Hacky way of getting a single directory from the vcpkg repo:
# - Download the vcpkg repo as a zip to a memory stream
# - Parse the zip archive
# - Extract the files we want
function Get-VcpkgRepoAsZipArchive
{
    $vcpkgZipUri = "https://github.com/microsoft/vcpkg/archive/refs/heads/master.zip"
    $response = Invoke-WebRequest -Uri $vcpkgZipUri
    $zipStream = [System.IO.MemoryStream]::new($response.Content)
    $zipArchive = [System.IO.Compression.ZipArchive]::new($zipStream)
    return $zipArchive
}

$VcpkgAsArchive = Get-VcpkgRepoAsZipArchive

# Copies an port from the official registry to this overlay
function New-PortOverlay
{
    param(
        [Parameter(Mandatory)]
        [string]$Port
    )

    $portDir = Join-Path $OverlayRoot $Port

    # Delete existing port if needed
    if (Test-Path $portDir)
    {
        Remove-Item -Force -Recurse $portDir
    }

    # Remove length=0 to ignore the directory itself
    $portZipEntries = $VcpkgAsArchive.Entries |
        Where-Object { ($_.Length -ne 0) -and $_.FullName.StartsWith("vcpkg-master/ports/$Port/") }

    if (-not $portZipEntries)
    {
        throw "Port $port not found"
    }

    New-Item -Type Directory $portDir | Out-Null
    foreach ($zipEntry in $portZipEntries)
    {
        $targetPath = Join-Path $portDir $zipEntry.Name
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($zipEntry, $targetPath)
    }
}

# Gets a git patch from a GitHub commit
function Get-GitHubPatch
{
    param(
        [Parameter(Mandatory)]
        [string]$Repo,  # as user/repo
        [Parameter(Mandatory)]
        [string]$Commit
    )

    $patchUri = "https://github.com/$repo/commit/$commit.patch"
    $response = Invoke-WebRequest -Uri $patchUri
    return $response.Content
}

# Filters a git patch to only include files from a given directory,
# and modifies the paths to make that directory the root for the patch
function Select-DirectoryInPatch
{
    param(
        [Parameter(Mandatory)]
        [string]$Patch,
        [Parameter(Mandatory)]
        [string]$Directory
    )

    # The patch starts with the commit message, author and other metadata
    # Then, for each modified file there is a line like
    #   diff --git a/the/file/path.txt b/the/file/path.txt
    # Followed by that files diff
    # We split around those lines, and select the ones with what we want
    $parts = $Patch -split '(?m)^(?=diff --git)'

    # Always keep the header/metadata
    $result = $parts[0]

    foreach ($fileDiff in $parts)
    {
        if ($fileDiff -match "^diff --git a/$Directory/.* b/$Directory/.*")
        {
            $result += $fileDiff -replace "(a|b)/$Directory/", '$1/'
        }
    }

    return $result
}

<#
    When updating a portfile, we look for a section that looks like this:

        vcpkg_from_github(
            OUT_SOURCE_PATH SOURCE_PATH
            REPO <user/repo>
            REF <commith hash>
            SHA512 <code .tar.gz hash>
            HEAD_REF master
            PATCHES
                patch-1.patch
                patch-2.patch
        )
#>

# Adds a patch to a portfile.cmake
function Add-PatchToPortFile
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$PatchName
    )

    # Look for the line that says "PATCHES" and add the new patch before the closing parenthesis

    $portFilePath = Join-Path $OverlayRoot $Port "portfile.cmake"
    $originalPortFile = Get-Content $portFilePath

    $modifiedPortFile = @()
    foreach ($line in $originalPortFile)
    {
        if (-not $foundParen)
        {
            if ($line.EndsWith("PATCHES"))
            {
                $foundPatches = $true
            }
            elseif ($line -eq ")")
            {
                $modifiedPortFile += "        $PatchName"
                $foundParen = $true
            }
        }

        $modifiedPortFile += $line
    }

    $modifiedPortFile | Out-File $portFilePath
}

# Adds a patch to a port
function Add-PatchToPort
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$PatchRepo,  # as user/repo
        [Parameter(Mandatory)]
        [string]$PatchCommit,
        [Parameter(Mandatory)]
        [string]$PatchName,
        [string]$PatchRoot
    )

    $patch = Get-GitHubPatch -Repo $PatchRepo -Commit $PatchCommit

    if ($PatchRoot)
    {
        $patch = Select-DirectoryInPatch -Patch $patch -Directory $PatchRoot
    }

    $portDir = Join-Path $OverlayRoot $Port
    $patch | Out-File (Join-Path $portDir $PatchName)

    Add-PatchToPortFile -Port $Port -PatchName $PatchName
}

# Sets the value of an existing function parameter.
# For example, REF in vcpkg_from_github
function Set-ParameterInPortFile
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$ParameterName,
        [Parameter(Mandatory)]
        [string]$CurrentValuePattern,
        [Parameter(Mandatory)]
        [string]$NewValue
    )

    $portFilePath = Join-Path $OverlayRoot $Port 'portfile.cmake'
    $originalPortFile = Get-Content $portFilePath

    # Explanation for the regex:
    # '(?<=)' - lookbehind without matching
    # '^ +'   - the parameter is only preceeded by spaces (and followed by a single space)
    # '(?=)'  - lookahead without matching
    # ' |$'   - the parameter may be the end of the line, or be followed by something else after a space (e.g. a comment)
    $regex = "(?<=^ +$ParameterName )$CurrentValuePattern(?= |$)"

    $modifiedPortFile = $originalPortFile -replace $regex, $NewValue
    $modifiedPortFile | Out-File $portFilePath
}

# Updates the source commit used for a port.
# Takes the commit hash, and the hash of the archive with the code that vcpkg will download.
function Update-PortSource
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$Commit,
        [Parameter(Mandatory)]
        [string]$SourceHash
    )

    $portDir = Join-Path $OverlayRoot $Port

    Set-ParameterInPortFile $Port -ParameterName 'REF' -CurrentValuePattern '[0-9a-f]{40}' -NewValue $Commit
    Set-ParameterInPortFile $Port -ParameterName 'SHA512' -CurrentValuePattern '[0-9a-f]{128}' -NewValue $SourceHash
}


New-PortOverlay cpprestsdk
Add-PatchToPort cpprestsdk -PatchRepo 'microsoft/winget-cli' -PatchCommit '888b4ed8f4f7d25cb05a47210e083fe29348163b' -PatchName 'add-server-certificate-validation.patch' -PatchRoot 'src/cpprestsdk/cpprestsdk'

New-PortOverlay libyaml
Update-PortSource libyaml -Commit '840b65c40675e2d06bf40405ad3f12dec7f35923' -SourceHash 'de85560312d53a007a2ddf1fe403676bbd34620480b1ba446b8c16bb366524ba7a6ed08f6316dd783bf980d9e26603a9efc82f134eb0235917b3be1d3eb4b302'