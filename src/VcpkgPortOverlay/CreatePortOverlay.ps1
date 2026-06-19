# Helper functions for dealing with the port overlay

[CmdletBinding()]
param(
    # When provided, serializes concurrent invocations via a named mutex and skips work
    # if the stamp file is already up-to-date. Used by VcpkgPortOverlay.proj.
    [string]$StampFile
)

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
        [string]$GitTree,
        [string]$Repo = 'microsoft/vcpkg'
    )

    $archiveUri = "https://github.com/$Repo/archive/$gitTree.zip"
    $response = Invoke-WebRequest -Uri $archiveUri
    $zipStream = [System.IO.MemoryStream]::new($response.Content)
    $zipArchive = [System.IO.Compression.ZipArchive]::new($zipStream)
    return $zipArchive
}

# Expands an in-memory archive and writes it to disk.
# If SubPath is specified, only files under that path are extracted.
function Expand-ArchiveFromMemory
{
    param(
        [Parameter(Mandatory)]
        [System.IO.Compression.ZipArchive]$Archive,
        [Parameter(Mandatory)]
        [string]$Destination,
        [string]$SubPath
    )

    # Delete existing directory
    if (Test-Path $Destination)
    {
        Remove-Item -Force -Recurse $Destination
    }

    # Remove length=0 to ignore the directory itself
    $entries = $archive.Entries | Where-Object { $_.Length -ne 0 }
    if ($SubPath)
    {
        $entries = $entries | Where-Object { $_.FullName -like "*/$SubPath/*" }
    }
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

# Creates a copy of a port from a GitHub repository in this overlay,
# by downloading the repository at a specific commit and extracting a subdirectory.
function New-PortOverlayFromGitHub
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$Repo,  # as user/repo
        [Parameter(Mandatory)]
        [string]$Commit,
        [Parameter(Mandatory)]
        [string]$SubPath
    )

    $archive = Get-GitTreeAsArchive -GitTree $Commit -Repo $Repo
    $portDir = Join-Path $OverlayRoot $Port
    Expand-ArchiveFromMemory -Archive $archive -Destination $portDir -SubPath $SubPath
}

# Expands a portfile.cmake that uses a "commented-option" template format, where the active
# function call is commented out. Uncomments the specified function block, strips remaining
# comment lines, and collapses blank lines.
function Expand-PortfileTemplate
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [string]$CommentedFunction = 'vcpkg_from_github'
    )

    $portFilePath = Join-Path $OverlayRoot $Port 'portfile.cmake'
    $lines = Get-Content $portFilePath

    $result = [System.Collections.Generic.List[string]]::new()
    $inCommentedBlock = $false
    $prevWasBlank = $false

    foreach ($line in $lines)
    {
        if ($line -match "^# $([regex]::Escape($CommentedFunction))\(")
        {
            $inCommentedBlock = $true
            $result.Add("$CommentedFunction(")
            $prevWasBlank = $false
            continue
        }

        if ($inCommentedBlock)
        {
            if ($line.TrimEnd() -eq '# )')
            {
                $result.Add(')')
                $inCommentedBlock = $false
            }
            else
            {
                $uncommented = $line -replace '^# ?', ''
                # Strip trailing inline hint comments from template lines
                $uncommented = $uncommented -replace ' # .*$', ''
                $result.Add($uncommented)
            }
            $prevWasBlank = $false
            continue
        }

        # Skip comment-only lines (template instructions, disabled options, etc.)
        if ($line -match '^#') { continue }

        if ($line -eq '')
        {
            if (-not $prevWasBlank -and $result.Count -gt 0)
            {
                $result.Add('')
            }
            $prevWasBlank = $true
        }
        else
        {
            $result.Add($line)
            $prevWasBlank = $false
        }
    }

    # Remove trailing blank lines
    while ($result.Count -gt 0 -and $result[$result.Count - 1] -eq '')
    {
        $result.RemoveAt($result.Count - 1)
    }

    $result | Out-File $portFilePath
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

# Adds a patch from the local patches/ directory to a port.
# Patches stored in patches/<Port>/ are committed to the repo and survive port regeneration.
function Add-LocalPatch
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$PatchName
    )

    Copy-Item (Join-Path $OverlayRoot 'patches' $Port $PatchName) (Join-Path $OverlayRoot $Port)

    $portFilePath = Join-Path $OverlayRoot $Port 'portfile.cmake'
    $lines = Get-Content $portFilePath
    $hasPatchesKeyword = $lines | Where-Object { $_ -match '\bPATCHES\b' }

    if ($hasPatchesKeyword)
    {
        Add-PatchToPortFile -Port $Port -PatchName $PatchName
        return
    }

    # Add PATCHES keyword and the patch name before the closing paren of vcpkg_from_github
    $result = @()
    $foundParen = $false
    foreach ($line in $lines)
    {
        if (-not $foundParen -and $line -eq ')')
        {
            $result += '    PATCHES'
            $result += "        $PatchName"
            $foundParen = $true
        }
        $result += $line
    }
    $result | Out-File $portFilePath
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

# Sets cmake configure options for a port by expanding the vcpkg_cmake_configure call.
function Set-CmakeConfigureOptions
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string[]]$Options
    )

    $portFilePath = Join-Path $OverlayRoot $Port 'portfile.cmake'
    $originalPortFile = Get-Content $portFilePath

    $modifiedPortFile = @()
    foreach ($line in $originalPortFile)
    {
        if ($line -match '^vcpkg_cmake_configure\(SOURCE_PATH')
        {
            $modifiedPortFile += 'vcpkg_cmake_configure('
            $modifiedPortFile += '    SOURCE_PATH "${SOURCE_PATH}"'
            $modifiedPortFile += '    OPTIONS'
            foreach ($option in $Options)
            {
                $modifiedPortFile += "        $option"
            }
            $modifiedPortFile += ')'
        }
        else
        {
            $modifiedPortFile += $line
        }
    }

    $modifiedPortFile | Out-File $portFilePath
}

# Replaces a <PLACEHOLDER> string in a file within a port directory.
function Set-PortFilePlaceholder
{
    param(
        [Parameter(Mandatory)]
        [string]$Port,
        [Parameter(Mandatory)]
        [string]$File,
        [Parameter(Mandatory)]
        [string]$Placeholder,
        [Parameter(Mandatory)]
        [string]$Value
    )

    $filePath = Join-Path $OverlayRoot $Port $File
    $content = (Get-Content -Raw $filePath) -replace "<$Placeholder>", $Value
    [System.IO.File]::WriteAllText($filePath, $content, [System.Text.Encoding]::UTF8)
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

# Acquire mutex if running from MSBuild (StampFile provided) to serialize parallel project builds
$_mutex = $null
if ($StampFile) {
    $_mutex = [System.Threading.Mutex]::new($false, 'Local\WingetVcpkgPortOverlay')
    try { $_mutex.WaitOne() | Out-Null } catch [System.Threading.AbandonedMutexException] {}

    # Another process may have already rebuilt the overlay while we waited; skip if so.
    if (Test-Path $StampFile) {
        $stampTime = (Get-Item $StampFile).LastWriteTime

        $overlayInputs = @($PSCommandPath)
        if (Test-Path $OverlayRoot\patches) {
            $overlayInputs += Get-ChildItem -Path $OverlayRoot\patches -Recurse -File
        }

        if (-not ($overlayInputs | Where-Object { $_.LastWriteTime -gt $stampTime } | Select-Object -First 1)) {
            $_mutex.ReleaseMutex()
            return
        }
    }
}

try {
    New-PortOverlay cpprestsdk -Version 2.10.18 -PortVersion 4
    Add-LocalPatch cpprestsdk 'fix-msvc-checked-array-iterator.patch'
    Add-LocalPatch cpprestsdk 'add-server-certificate-validation.patch'

    New-PortOverlay detours -Version 4.0.1 -PortVersion 8
    Update-PortSource detours -RefPattern 'v4.0.1' -Commit '404c153ff390cb14f1787c7feeb4908c6d79b0ab' -SourceHash '1f3f26657927fa153116dce13dbfa3319ea368e6c9017f4999b6ec24d6356c335b3d5326718d3ec707b92832763ffea092088df52596f016d7ca9b8127f7033d'
    Remove-PortPatches detours

    New-PortOverlay libyaml -Version 0.2.5 -PortVersion 5
    Update-PortSource libyaml -Commit '840b65c40675e2d06bf40405ad3f12dec7f35923' -SourceHash 'de85560312d53a007a2ddf1fe403676bbd34620480b1ba446b8c16bb366524ba7a6ed08f6316dd783bf980d9e26603a9efc82f134eb0235917b3be1d3eb4b302'
    Update-PortVersion libyaml

    # sfs-client is not in the official vcpkg registry.
    # The port is based on the template from the sfs-client repository.
    # See: https://github.com/microsoft/sfs-client/tree/main/sfs-client-vcpkg-port/sfs-client
    $SfsClientCommit = '0e27525d597c730e71646fd0b15bdc8c8503f24d'
    $SfsClientSha512 = 'd926d7fdbbd120cbcbd9732a3300cccfeed4a90d6b94456d73a70675df3578a91127f7e9f310fe68d18fa34bb997c29c8455e586d81a2ba404cf19193a80ca6e'
    $SfsClientVersion = '1.1.0'

    New-PortOverlayFromGitHub 'sfs-client' -Repo 'microsoft/sfs-client' -Commit $SfsClientCommit -SubPath 'sfs-client-vcpkg-port/sfs-client'
    Expand-PortfileTemplate 'sfs-client'
    Set-PortFilePlaceholder 'sfs-client' 'portfile.cmake' -Placeholder 'commit-id' -Value $SfsClientCommit
    Set-ParameterInPortFile 'sfs-client' -ParameterName 'SHA512' -CurrentValuePattern '0' -NewValue $SfsClientSha512
    Set-CmakeConfigureOptions 'sfs-client' -Options @('-DSFS_BUILD_TESTS=OFF', '-DSFS_BUILD_SAMPLES=OFF')
    Set-PortFilePlaceholder 'sfs-client' 'vcpkg.json' -Placeholder 'VERSION' -Value $SfsClientVersion

    Add-LocalPatch 'sfs-client' 'remove-unconditional-toolchain-override.patch'

    if ($StampFile) {
        $null = New-Item -ItemType File -Path $StampFile -Force
    }
} finally {
    if ($_mutex) { $_mutex.ReleaseMutex() }
}
