# Helper functions for dealing with the port overlay

$OverlayRoot = $PSScriptRoot

$ErrorActionPreference = "Stop"


# Gets the versions of a port available from the official registry.
# This is read from the versions JSON in the main branch.
# A version looks like this:
#    {
#      "git-tree": "9f5e160191038cbbd2470e534c43f051c80e7d44",
#      "version": "2.10.19",
#      "port-version": 3
#    }
function Get-PortVersions
{
    param(
        [Parameter(Mandatory)]
        [string]$Port
    )

    $initial = $Port[0]
    $jsonUri = "https://raw.githubusercontent.com/microsoft/vcpkg/heads/master/versions/$initial-/$Port.json"
    $versions = (Invoke-WebRequest -Uri $jsonUri).Content | ConvertFrom-Json -Depth 5
    return $versions.versions
}

# Gets the git-tree associated with a specific version of a port.
# The git-tree is a git object hash that represents the port directory
# from the appropriate version of the registry.
function Get-PortVersionGitTree
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$Version,
        [Parameter(Mandatory)]
        [string]$PortVersion
    )

    $versions = Get-PortVersions $Port
    $versionData = $versions | Where-Object { ($_.version -eq $Version) -and ($_."port-version" -eq $portVersion) }
    return $versionData."git-tree"
}

# Fetches and parses a git-tree as a ZIP file
function Get-GitTreeAsArchive
{
    param(
        [Parameter(Mandatory)]
        [string]$GitTree
    )

    $archiveUri = "https://github.com/microsoft/vcpkg/archive/$gitTree.zip"
    $response = Invoke-WebRequest -Uri $archiveUri
    $zipStream = [System.IO.MemoryStream]::new($response.Content)
    $zipArchive = [System.IO.Compression.ZipArchive]::new($zipStream)
    return $zipArchive
}

# Expands an in-memory archive and writes it to disk
function Expand-ArchiveFromMemory
{
    param(
        [Parameter(Mandatory)]
        [System.IO.Compression.ZipArchive]$Archive,
        [Parameter(Mandatory)]
        [string]$Destination
    )

    # Delete existing directory
    if (Test-Path $Destination)
    {
        Remove-Item -Force -Recurse $Destination
    }

    # Remove length=0 to ignore the directory itself
    $entries = $archive.Entries | Where-Object { $_.Length -ne 0 }
    if (-not $entries)
    {
        throw "Archive is empty"
    }

    New-Item -Type Directory $Destination | Out-Null
    foreach ($entry in $entries)
    {
        $targetPath = Join-Path $Destination $entry.Name
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $targetPath)
    }
}

# Creates a copy of a port version from the official registry in this overlay
function New-PortOverlay
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$Version,
        [Parameter(Mandatory)]
        [string]$PortVersion
    )

    $gitTree = Get-PortVersionGitTree $Port $Version $PortVersion
    $archive = Get-GitTreeAsArchive $gitTree
    $portDir = Join-Path $OverlayRoot $Port
    Expand-ArchiveFromMemory $archive $portDir
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

# Removes all patches from portfile.cmake
function Remove-PortPatches
{
    param(
        [Parameter(Mandatory)]
        [string]$Port
    )

    # Look for the line that says "PATCHES"

    $portFilePath = Join-Path $OverlayRoot $Port "portfile.cmake"
    $originalPortFile = Get-Content $portFilePath

    $modifiedPortFile = @()
    foreach ($line in $originalPortFile)
    {
        if ($line.TrimEnd().EndsWith("PATCHES"))
        {
            $foundPatches = $true
        }
        elseif ($line -eq ")")
        {
            $foundParen = $true
            $modifiedPortFile += $line
        }
        elseif ($foundPatches -and -not $foundParen)
        {
            # Drop line
        }
        else
        {
            $modifiedPortFile += $line
        }
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
        [string]$SourceHash,
        [string]$RefPattern = '[0-9a-f]{40}( #.*)?$'
    )

    # For the REF, we also delete any comments after it that may say the wrong version
    Set-ParameterInPortFile $Port -ParameterName 'REF' -CurrentValuePattern $RefPattern -NewValue "$Commit # Unreleased"
    Set-ParameterInPortFile $Port -ParameterName 'SHA512' -CurrentValuePattern '[0-9a-f]{128}' -NewValue $SourceHash
}

# Updates the port version by one.
function Update-PortVersion
{
    param(
        [Parameter(Mandatory)]
        [string]$Port
    )

    $portJsonPath = Join-Path $OverlayRoot $Port "vcpkg.json"
    $portDefinition = Get-Content $portJsonPath | ConvertFrom-Json
    $portDefinition."port-version" += 1
    $portDefinition | ConvertTo-Json -Depth 5 | Out-File $portJsonPath
}

New-PortOverlay cpprestsdk -Version 2.10.18 -PortVersion 4
Add-PatchToPort cpprestsdk -PatchRepo 'microsoft/winget-cli' -PatchCommit '888b4ed8f4f7d25cb05a47210e083fe29348163b' -PatchName 'add-server-certificate-validation.patch' -PatchRoot 'src/cpprestsdk/cpprestsdk'

New-PortOverlay detours -Version 4.0.1 -PortVersion 8
Update-PortSource detours -RefPattern 'v4.0.1' -Commit '404c153ff390cb14f1787c7feeb4908c6d79b0ab' -SourceHash '1f3f26657927fa153116dce13dbfa3319ea368e6c9017f4999b6ec24d6356c335b3d5326718d3ec707b92832763ffea092088df52596f016d7ca9b8127f7033d'
Remove-PortPatches detours

New-PortOverlay libyaml -Version 0.2.5 -PortVersion 5
Update-PortSource libyaml -Commit '840b65c40675e2d06bf40405ad3f12dec7f35923' -SourceHash 'de85560312d53a007a2ddf1fe403676bbd34620480b1ba446b8c16bb366524ba7a6ed08f6316dd783bf980d9e26603a9efc82f134eb0235917b3be1d3eb4b302'
Update-PortVersion libyaml
