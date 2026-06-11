<#
.SYNOPSIS
    Branches the 'latest' manifest schemas into a frozen, versioned folder.

.DESCRIPTION
    Reads the schema version from the $id field in latest/manifest.installer.latest.json,
    creates schemas/JSON/manifests/v{VERSION}/ with properly named copies of each latest/
    schema file, and updates ManifestSchema.rc, ManifestSchema.vcxitems, and
    ManifestSchema.vcxitems.filters to reference the versioned files.

    C++ source changes (ManifestCommon.h, ManifestSchemaValidation.cpp, ManifestSchema.h,
    ManifestVersion.cs) and test manifests require manual updates - see README.md.

.PARAMETER RepoRoot
    Path to the repository root. Defaults to three directories above this script, which is
    correct when the script lives at schemas/JSON/manifests/Branch-LatestManifestSchema.ps1.

.EXAMPLE
    .\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1

.EXAMPLE
    .\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1 -WhatIf
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [string] $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..') -ErrorAction Stop).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# -- Helper: save XML preserving encoding --------------------------------------
function Save-XmlFile {
    param([xml]$Xml, [string]$Path)
    $settings = [System.Xml.XmlWriterSettings]@{
        Indent      = $true
        IndentChars = '  '
        Encoding    = New-Object System.Text.UTF8Encoding $false
    }
    $writer = [System.Xml.XmlWriter]::Create($Path, $settings)
    try { $Xml.Save($writer) }
    finally { $writer.Close() }
}

# -- Resolve paths --------------------------------------------------------------
$manifestsDir      = Join-Path $RepoRoot 'schemas\JSON\manifests'
$latestDir         = Join-Path $manifestsDir 'latest'
$manifestSchemaDir = Join-Path $RepoRoot 'src\ManifestSchema'
$rcFile            = Join-Path $manifestSchemaDir 'ManifestSchema.rc'
$vcxitemsFile      = Join-Path $manifestSchemaDir 'ManifestSchema.vcxitems'
$filtersFile       = Join-Path $manifestSchemaDir 'ManifestSchema.vcxitems.filters'

foreach ($path in $latestDir, $rcFile, $vcxitemsFile, $filtersFile) {
    if (-not (Test-Path $path)) {
        throw "Required path not found: '$path'. Verify -RepoRoot is correct."
    }
}

# -- Detect version from latest/manifest.installer.latest.json -----------------
$installerSchema = Join-Path $latestDir 'manifest.installer.latest.json'
$schemaJson      = Get-Content $installerSchema -Raw | ConvertFrom-Json
$idField         = $schemaJson.'$id'

if ($idField -notmatch '(\d+\.\d+\.\d+)\.schema\.json$') {
    throw "Could not parse version from `$id field: '$idField'"
}
$version     = $Matches[1]
$versionedDir = Join-Path $manifestsDir "v$version"

Write-Host "Schema version detected: $version"
Write-Host "Target directory:        $versionedDir"
Write-Host ''

if (Test-Path $versionedDir) {
    Write-Warning "Directory '$versionedDir' already exists - files may be overwritten."
}

# -- Schema file types ----------------------------------------------------------
$schemaTypes = @('singleton', 'version', 'installer', 'defaultLocale', 'locale')

# Relative path prefix used in ManifestSchema.rc (relative to src\ManifestSchema\)
$rcRelBase = '..\..\schemas\JSON\manifests'

# Include-path prefix used in ManifestSchema.vcxitems / .vcxitems.filters
$vcxRelBase = '$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests'

$ns = 'http://schemas.microsoft.com/developer/msbuild/2003'

# -- Step 1: Copy and rename schema files ---------------------------------------
Write-Host '=== Step 1: Schema files ==='
if ($PSCmdlet.ShouldProcess($versionedDir, 'Create directory')) {
    New-Item -ItemType Directory -Path $versionedDir -Force | Out-Null
}

foreach ($type in $schemaTypes) {
    $src  = Join-Path $latestDir "manifest.$type.latest.json"
    $dest = Join-Path $versionedDir "manifest.$type.$version.json"
    if ($PSCmdlet.ShouldProcess("manifest.$type.latest.json -> manifest.$type.$version.json", 'Copy')) {
        Copy-Item -Path $src -Destination $dest -Force
        Write-Host "  Copied manifest.$type.$version.json"
    }
}

# -- Step 2: Update ManifestSchema.rc ------------------------------------------
Write-Host ''
Write-Host '=== Step 2: ManifestSchema.rc ==='
$rcContent  = Get-Content $rcFile -Raw
$rcModified = $false

foreach ($type in $schemaTypes) {
    # The RC file uses double-backslash escaping inside quoted strings; use
    # String.Replace (literal) rather than -replace (regex) to avoid re-escaping.
    $latestPath    = ("$rcRelBase\latest\manifest.$type.latest.json").Replace('\', '\\')
    $versionedPath = ("$rcRelBase\v$version\manifest.$type.$version.json").Replace('\', '\\')

    if ($rcContent.Contains($latestPath)) {
        if ($PSCmdlet.ShouldProcess($rcFile, "Redirect manifest.$type from latest/ to v$version/")) {
            $rcContent  = $rcContent.Replace($latestPath, $versionedPath)
            $rcModified = $true
            Write-Host "  Updated path for manifest.$type"
        }
    } elseif ($rcContent.Contains($versionedPath)) {
        Write-Host "  manifest.$type already points to v$version/ - no change needed."
    } else {
        Write-Warning "  manifest.${type}: neither latest/ nor v${version}/ path found in RC file. Manual edit required."
    }
}

if ($rcModified -and $PSCmdlet.ShouldProcess($rcFile, 'Save')) {
    [System.IO.File]::WriteAllText($rcFile, $rcContent, (New-Object System.Text.UTF8Encoding $false))
    Write-Host "  Saved $rcFile"
}

# -- Step 3: Update ManifestSchema.vcxitems ------------------------------------
Write-Host ''
Write-Host '=== Step 3: ManifestSchema.vcxitems ==='

[xml]$vcxXml = Get-Content $vcxitemsFile -Raw
$nsm = New-Object System.Xml.XmlNamespaceManager($vcxXml.NameTable)
$nsm.AddNamespace('msb', $ns)

$noneGroup = $vcxXml.SelectSingleNode('//msb:ItemGroup[msb:None]', $nsm)
if (-not $noneGroup) {
    throw 'Could not locate <ItemGroup> containing <None> elements in ManifestSchema.vcxitems.'
}

$vcxModified = $false
foreach ($type in $schemaTypes) {
    $versionedInclude = "$vcxRelBase\v$version\manifest.$type.$version.json"

    if ($vcxXml.SelectSingleNode("//msb:None[@Include='$versionedInclude']", $nsm)) {
        Write-Host "  manifest.$type v$version already present - no change needed."
        continue
    }

    if ($PSCmdlet.ShouldProcess($vcxitemsFile, "Add <None> for manifest.$type v$version")) {
        $el = $vcxXml.CreateElement('None', $ns)
        $el.SetAttribute('Include', $versionedInclude)
        $noneGroup.AppendChild($el) | Out-Null
        $vcxModified = $true
        Write-Host "  Added <None> for manifest.$type.$version.json"
    }
}

if ($vcxModified -and $PSCmdlet.ShouldProcess($vcxitemsFile, 'Save')) {
    Save-XmlFile $vcxXml $vcxitemsFile
    Write-Host "  Saved $vcxitemsFile"
}

# -- Step 4: Update ManifestSchema.vcxitems.filters ----------------------------
Write-Host ''
Write-Host '=== Step 4: ManifestSchema.vcxitems.filters ==='

[xml]$filtersXml = Get-Content $filtersFile -Raw
$fnsm = New-Object System.Xml.XmlNamespaceManager($filtersXml.NameTable)
$fnsm.AddNamespace('msb', $ns)

$filterName = "schema\v$version"

# Add the <Filter> entry if missing
$filterGroup      = $filtersXml.SelectSingleNode('//msb:ItemGroup[msb:Filter]', $fnsm)
$existingFilter   = $filtersXml.SelectSingleNode("//msb:Filter[@Include='$filterName']", $fnsm)
$filtersModified  = $false

if (-not $existingFilter) {
    if ($PSCmdlet.ShouldProcess($filtersFile, "Add <Filter> for $filterName")) {
        $newGuid  = [System.Guid]::NewGuid().ToString().ToLower()
        $filterEl = $filtersXml.CreateElement('Filter', $ns)
        $filterEl.SetAttribute('Include', $filterName)
        $guidEl   = $filtersXml.CreateElement('UniqueIdentifier', $ns)
        $guidEl.InnerText = "{$newGuid}"
        $filterEl.AppendChild($guidEl) | Out-Null
        $filterGroup.AppendChild($filterEl) | Out-Null
        $filtersModified = $true
        Write-Host "  Added <Filter> for $filterName (GUID: {$newGuid})"
    }
} else {
    Write-Host "  <Filter> for $filterName already present."
}

# Add <None> entries with Filter children
$noneFilterGroup = $filtersXml.SelectSingleNode('//msb:ItemGroup[msb:None]', $fnsm)
if (-not $noneFilterGroup) {
    $noneFilterGroup = $filtersXml.CreateElement('ItemGroup', $ns)
    $filtersXml.DocumentElement.AppendChild($noneFilterGroup) | Out-Null
}

foreach ($type in $schemaTypes) {
    $versionedInclude = "$vcxRelBase\v$version\manifest.$type.$version.json"
    if ($filtersXml.SelectSingleNode("//msb:None[@Include='$versionedInclude']", $fnsm)) {
        Write-Host "  manifest.$type v$version filter entry already present."
        continue
    }

    if ($PSCmdlet.ShouldProcess($filtersFile, "Add <None> filter entry for manifest.$type v$version")) {
        $noneEl     = $filtersXml.CreateElement('None', $ns)
        $noneEl.SetAttribute('Include', $versionedInclude)
        $filterChild = $filtersXml.CreateElement('Filter', $ns)
        $filterChild.InnerText = $filterName
        $noneEl.AppendChild($filterChild) | Out-Null
        $noneFilterGroup.AppendChild($noneEl) | Out-Null
        $filtersModified = $true
        Write-Host "  Added <None> filter entry for manifest.$type.$version.json"
    }
}

if ($filtersModified -and $PSCmdlet.ShouldProcess($filtersFile, 'Save')) {
    Save-XmlFile $filtersXml $filtersFile
    Write-Host "  Saved $filtersFile"
}

# -- Summary --------------------------------------------------------------------
$major, $minor = $version -split '\.' | Select-Object -First 2
$cppSuffix = "V${major}_${minor}"

Write-Host ''
Write-Host '=== Done ==='
Write-Host ''
Write-Host 'Manual steps remaining (if not already done in a prior PR):'
Write-Host ''
Write-Host "  1. src\AppInstallerCommonCore\Public\winget\ManifestCommon.h"
Write-Host "       Add: constexpr std::string_view s_ManifestVersion${cppSuffix} = `"$version`"sv;"
Write-Host ''
Write-Host "  2. src\AppInstallerCommonCore\Manifest\ManifestSchemaValidation.cpp"
Write-Host "       Prepend a new 'if (manifestVersion >= ManifestVer{ s_ManifestVersion${cppSuffix} })'"
Write-Host "       block at the top of the version-check chain, using IDX_MANIFEST_SCHEMA_${cppSuffix}_* constants."
Write-Host ''
Write-Host "  3. src\ManifestSchema\ManifestSchema.h"
Write-Host "       Add five IDX_MANIFEST_SCHEMA_${cppSuffix}_* constants (next available IDs, must stay < 300)."
Write-Host ''
Write-Host "  4. src\WinGetUtilInterop\Manifest\ManifestVersion.cs"
Write-Host "       Add: public const string ManifestVersion${cppSuffix} = `"$version`";"
Write-Host ''
Write-Host "  5. src\AppInstallerCLITests\TestData\"
Write-Host "       Add ManifestV${major}_${minor}-Singleton.yaml and ManifestV${major}_${minor}-MultiFile-*.yaml."
Write-Host ''
Write-Host "  6. src\AppInstallerCLITests\AppInstallerCLITests.vcxproj and .vcxproj.filters"
Write-Host "       Reference the new test manifest files."
Write-Host ''
Write-Host "  7. src\AppInstallerCLITests\YamlManifest.cpp"
Write-Host "       Add test cases for manifest version $version."
