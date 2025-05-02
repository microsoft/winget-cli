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

# Adds a patch to a portfile.cmake
function Add-PatchToPortFile
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$PatchName
    )

    <#
        We're looking for a section that looks like this:

            vcpkg_from_github(
                OUT_SOURCE_PATH SOURCE_PATH
                REPO <user/repo>
                REF <commith hash>
                SHA512 <hash>
                HEAD_REF master
                PATCHES
                    patch-1.patch
                    patch-2.patch
            )

        We look for the line that says "PATCHES" and add the new patch before the closing parenthesis
    #>

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

New-PortOverlay cpprestsdk
Add-PatchToPort cpprestsdk -PatchRepo "microsoft/winget-cli" -PatchCommit "888b4ed8f4f7d25cb05a47210e083fe29348163b" -PatchName "add-server-certificate-validation.patch" -PatchRoot "src/cpprestsdk/cpprestsdk"

New-PortOverlay libyaml
Add-PatchToPort libyaml -PatchRepo "yaml/libyaml" -PatchCommit "51843fe48257c6b7b6e70cdec1db634f64a40818" -PatchName "fix-parser-nesting.patch"