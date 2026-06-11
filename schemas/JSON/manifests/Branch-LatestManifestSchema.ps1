<#
.SYNOPSIS
    Branches the 'latest' manifest schemas and/or bumps the schema version to match the binary.

.DESCRIPTION
    Without -BumpVersion:
      Freezes the current 'latest/' schemas into a versioned folder (e.g. v1.28.0/), and
      updates ManifestSchema.rc, ManifestSchema.vcxitems, and ManifestSchema.vcxitems.filters
      to reference the now-versioned files. Use this when a WinGet release is approaching and
      the in-flight schema needs to be locked in.

    With -BumpVersion:
      Reads the target version from src/binver/binver/version.h (MAJOR.MINOR.0) and compares
      it against the version embedded in the 'latest/' schema $id fields. When they differ:
        1. Branches the current 'latest/' schema if a versioned folder does not yet exist.
        2. Updates every $id and description string in the 'latest/' JSON files to the new version.
        3. Adds the new version constant to ManifestCommon.h.
        4. Adds five new IDX_MANIFEST_SCHEMA_* constants to ManifestSchema.h.
        5. Appends new resource entries (pointing to 'latest/') to ManifestSchema.rc.
        6. Prepends a new version block to the if-chain in ManifestSchemaValidation.cpp.
        7. Adds the new version constant to ManifestVersion.cs.
      When they already match, reports that no bump is needed and exits.

    C++ test manifests (ManifestV*-Singleton.yaml etc.), the test project files, and
    YamlManifest.cpp always require manual updates -- see README.md.

.PARAMETER BumpVersion
    Also advance the 'latest/' schema version to match src/binver/binver/version.h and
    update all associated source files.

.PARAMETER RepoRoot
    Path to the repository root. Defaults to three directories above this script, which is
    correct when the script lives at schemas/JSON/manifests/Branch-LatestManifestSchema.ps1.

.EXAMPLE
    # Freeze the in-flight schema at its current version number.
    .\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1

.EXAMPLE
    # Freeze then advance the schema version to match the binary.
    .\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1 -BumpVersion

.EXAMPLE
    .\schemas\JSON\manifests\Branch-LatestManifestSchema.ps1 -BumpVersion -WhatIf
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [switch] $BumpVersion,
    [string] $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..') -ErrorAction Stop).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

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

function Save-TextFile {
    param([string]$Content, [string]$Path)
    [System.IO.File]::WriteAllText($Path, $Content, (New-Object System.Text.UTF8Encoding $false))
}

# Derive the C++ identifier suffix from a version string: "1.29.0" -> "V1_29"
function Get-CppSuffix {
    param([string]$Version)
    $parts = $Version -split '\.'
    return "V$($parts[0])_$($parts[1])"
}

# ---------------------------------------------------------------------------
# Resolve paths
# ---------------------------------------------------------------------------

$manifestsDir       = Join-Path $RepoRoot 'schemas\JSON\manifests'
$latestDir          = Join-Path $manifestsDir 'latest'
$manifestSchemaDir  = Join-Path $RepoRoot 'src\ManifestSchema'
$rcFile             = Join-Path $manifestSchemaDir 'ManifestSchema.rc'
$vcxitemsFile       = Join-Path $manifestSchemaDir 'ManifestSchema.vcxitems'
$filtersFile        = Join-Path $manifestSchemaDir 'ManifestSchema.vcxitems.filters'
$manifestCommonH    = Join-Path $RepoRoot 'src\AppInstallerCommonCore\Public\winget\ManifestCommon.h'
$schemaValidationCpp = Join-Path $RepoRoot 'src\AppInstallerCommonCore\Manifest\ManifestSchemaValidation.cpp'
$manifestSchemaH    = Join-Path $RepoRoot 'src\ManifestSchema\ManifestSchema.h'
$manifestVersionCs  = Join-Path $RepoRoot 'src\WinGetUtilInterop\Manifest\ManifestVersion.cs'
$binverH            = Join-Path $RepoRoot 'src\binver\binver\version.h'

foreach ($path in $latestDir, $rcFile, $vcxitemsFile, $filtersFile) {
    if (-not (Test-Path $path)) {
        throw "Required path not found: '$path'. Verify -RepoRoot is correct."
    }
}

# ---------------------------------------------------------------------------
# Read schema version from latest/
# ---------------------------------------------------------------------------

$installerSchema = Join-Path $latestDir 'manifest.installer.latest.json'
$schemaJson      = Get-Content $installerSchema -Raw | ConvertFrom-Json
$idField         = $schemaJson.'$id'

if ($idField -notmatch '(\d+\.\d+\.\d+)\.schema\.json$') {
    throw "Could not parse version from `$id field: '$idField'"
}
$schemaVersion = $Matches[1]
$versionedDir  = Join-Path $manifestsDir "v$schemaVersion"

Write-Host "Current schema version: $schemaVersion"

# ---------------------------------------------------------------------------
# If -BumpVersion: read binary version and decide what to do
# ---------------------------------------------------------------------------

$newVersion = $null

if ($BumpVersion) {
    foreach ($path in $binverH, $manifestCommonH, $schemaValidationCpp, $manifestSchemaH, $manifestVersionCs) {
        if (-not (Test-Path $path)) {
            throw "Required path not found for -BumpVersion: '$path'. Verify -RepoRoot is correct."
        }
    }

    $binverContent = Get-Content $binverH -Raw
    if ($binverContent -notmatch '#define VERSION_MAJOR\s+(\d+)') { throw "Could not parse VERSION_MAJOR from version.h" }
    $binMajor = $Matches[1]
    if ($binverContent -notmatch '#define VERSION_MINOR\s+(\d+)') { throw "Could not parse VERSION_MINOR from version.h" }
    $binMinor = $Matches[1]
    $newVersion = "$binMajor.$binMinor.0"

    Write-Host "Binary version:         $newVersion"

    if ($schemaVersion -eq $newVersion) {
        Write-Host ''
        Write-Host "Schema version already matches the binary version ($newVersion). No bump needed."
        exit 0
    }

    Write-Host "Schema will be bumped:  $schemaVersion -> $newVersion"
}

Write-Host ''

# ---------------------------------------------------------------------------
# Branch function: freeze current latest/ into v{schemaVersion}/
# ---------------------------------------------------------------------------

$schemaTypes = @('singleton', 'version', 'installer', 'defaultLocale', 'locale')
$rcRelBase   = '..\..\schemas\JSON\manifests'
$vcxRelBase  = '$(MSBuildThisFileDirectory)..\..\schemas\JSON\manifests'
$ns          = 'http://schemas.microsoft.com/developer/msbuild/2003'

function Invoke-BranchLatest {
    param([string]$Version, [string]$VersionedDir)

    $alreadyBranched = (Test-Path $VersionedDir) -and
        ((Get-ChildItem $VersionedDir -Filter "*.json").Count -ge $schemaTypes.Count)

    if ($alreadyBranched) {
        Write-Host "=== Branch: v$Version already exists -- skipping schema file copy ==="
    } else {
        # Step 1: Schema files
        Write-Host '=== Branch Step 1: Schema files ==='
        if ($PSCmdlet.ShouldProcess($VersionedDir, 'Create directory')) {
            New-Item -ItemType Directory -Path $VersionedDir -Force | Out-Null
        }
        foreach ($type in $schemaTypes) {
            $src  = Join-Path $latestDir "manifest.$type.latest.json"
            $dest = Join-Path $VersionedDir "manifest.$type.$Version.json"
            if ($PSCmdlet.ShouldProcess("manifest.$type.latest.json -> manifest.$type.$Version.json", 'Copy')) {
                Copy-Item -Path $src -Destination $dest -Force
                Write-Host "  Copied manifest.$type.$Version.json"
            }
        }
    }

    # Step 2: ManifestSchema.rc -- redirect latest/ entries to versioned paths
    Write-Host ''
    Write-Host '=== Branch Step 2: ManifestSchema.rc ==='
    $rcContent  = Get-Content $rcFile -Raw
    $rcModified = $false

    foreach ($type in $schemaTypes) {
        $latestPath    = ("$rcRelBase\latest\manifest.$type.latest.json").Replace('\', '\\')
        $versionedPath = ("$rcRelBase\v$Version\manifest.$type.$Version.json").Replace('\', '\\')

        if ($rcContent.Contains($latestPath)) {
            if ($PSCmdlet.ShouldProcess($rcFile, "Redirect manifest.$type from latest/ to v$Version/")) {
                $rcContent  = $rcContent.Replace($latestPath, $versionedPath)
                $rcModified = $true
                Write-Host "  Updated path for manifest.$type"
            }
        } elseif ($rcContent.Contains($versionedPath)) {
            Write-Host "  manifest.$type already points to v$Version/ - no change needed."
        } else {
            Write-Warning "  manifest.${type}: neither latest/ nor v${Version}/ path found in RC file. Manual edit required."
        }
    }

    if ($rcModified -and $PSCmdlet.ShouldProcess($rcFile, 'Save')) {
        Save-TextFile $rcContent $rcFile
        Write-Host "  Saved $rcFile"
    }

    # Step 3: ManifestSchema.vcxitems -- add versioned <None> entries
    Write-Host ''
    Write-Host '=== Branch Step 3: ManifestSchema.vcxitems ==='

    [xml]$vcxXml = Get-Content $vcxitemsFile -Raw
    $nsm = New-Object System.Xml.XmlNamespaceManager($vcxXml.NameTable)
    $nsm.AddNamespace('msb', $ns)

    $noneGroup = $vcxXml.SelectSingleNode('//msb:ItemGroup[msb:None]', $nsm)
    if (-not $noneGroup) { throw 'Could not locate <ItemGroup> containing <None> in ManifestSchema.vcxitems.' }

    $vcxModified = $false
    foreach ($type in $schemaTypes) {
        $versionedInclude = "$vcxRelBase\v$Version\manifest.$type.$Version.json"
        if ($vcxXml.SelectSingleNode("//msb:None[@Include='$versionedInclude']", $nsm)) {
            Write-Host "  manifest.$type v$Version already present - no change needed."
            continue
        }
        if ($PSCmdlet.ShouldProcess($vcxitemsFile, "Add <None> for manifest.$type v$Version")) {
            $el = $vcxXml.CreateElement('None', $ns)
            $el.SetAttribute('Include', $versionedInclude)
            $noneGroup.AppendChild($el) | Out-Null
            $vcxModified = $true
            Write-Host "  Added <None> for manifest.$type.$Version.json"
        }
    }

    if ($vcxModified -and $PSCmdlet.ShouldProcess($vcxitemsFile, 'Save')) {
        Save-XmlFile $vcxXml $vcxitemsFile
        Write-Host "  Saved $vcxitemsFile"
    }

    # Step 4: ManifestSchema.vcxitems.filters -- add filter + <None> entries
    Write-Host ''
    Write-Host '=== Branch Step 4: ManifestSchema.vcxitems.filters ==='

    [xml]$filtersXml = Get-Content $filtersFile -Raw
    $fnsm = New-Object System.Xml.XmlNamespaceManager($filtersXml.NameTable)
    $fnsm.AddNamespace('msb', $ns)

    $filterName     = "schema\v$Version"
    $filterGroup    = $filtersXml.SelectSingleNode('//msb:ItemGroup[msb:Filter]', $fnsm)
    $existingFilter = $filtersXml.SelectSingleNode("//msb:Filter[@Include='$filterName']", $fnsm)
    $filtersModified = $false

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

    $noneFilterGroup = $filtersXml.SelectSingleNode('//msb:ItemGroup[msb:None]', $fnsm)
    if (-not $noneFilterGroup) {
        $noneFilterGroup = $filtersXml.CreateElement('ItemGroup', $ns)
        $filtersXml.DocumentElement.AppendChild($noneFilterGroup) | Out-Null
    }

    foreach ($type in $schemaTypes) {
        $versionedInclude = "$vcxRelBase\v$Version\manifest.$type.$Version.json"
        if ($filtersXml.SelectSingleNode("//msb:None[@Include='$versionedInclude']", $fnsm)) {
            Write-Host "  manifest.$type v$Version filter entry already present."
            continue
        }
        if ($PSCmdlet.ShouldProcess($filtersFile, "Add <None> filter entry for manifest.$type v$Version")) {
            $noneEl      = $filtersXml.CreateElement('None', $ns)
            $noneEl.SetAttribute('Include', $versionedInclude)
            $filterChild = $filtersXml.CreateElement('Filter', $ns)
            $filterChild.InnerText = $filterName
            $noneEl.AppendChild($filterChild) | Out-Null
            $noneFilterGroup.AppendChild($noneEl) | Out-Null
            $filtersModified = $true
            Write-Host "  Added <None> filter entry for manifest.$type.$Version.json"
        }
    }

    if ($filtersModified -and $PSCmdlet.ShouldProcess($filtersFile, 'Save')) {
        Save-XmlFile $filtersXml $filtersFile
        Write-Host "  Saved $filtersFile"
    }
}

# ---------------------------------------------------------------------------
# Run branch step
# ---------------------------------------------------------------------------

Invoke-BranchLatest -Version $schemaVersion -VersionedDir $versionedDir

# ---------------------------------------------------------------------------
# If not bumping, print manual-step reminder and exit
# ---------------------------------------------------------------------------

if (-not $BumpVersion) {
    $oldSuffix = Get-CppSuffix $schemaVersion
    Write-Host ''
    Write-Host '=== Done ==='
    Write-Host ''
    Write-Host 'Manual steps remaining (if not already done in a prior PR):'
    Write-Host ''
    Write-Host "  1. src\AppInstallerCommonCore\Public\winget\ManifestCommon.h"
    Write-Host "       Add: constexpr std::string_view s_ManifestVersion${oldSuffix} = `"$schemaVersion`"sv;"
    Write-Host ''
    Write-Host "  2. src\AppInstallerCommonCore\Manifest\ManifestSchemaValidation.cpp"
    Write-Host "       Prepend a new 'if (manifestVersion >= ManifestVer{ s_ManifestVersion${oldSuffix} })'"
    Write-Host "       block at the top of the version-check chain."
    Write-Host ''
    Write-Host "  3. src\ManifestSchema\ManifestSchema.h"
    Write-Host "       Add five IDX_MANIFEST_SCHEMA_${oldSuffix}_* constants (next available IDs, must stay < 300)."
    Write-Host ''
    Write-Host "  4. src\WinGetUtilInterop\Manifest\ManifestVersion.cs"
    Write-Host "       Add: public const string ManifestVersion${oldSuffix} = `"$schemaVersion`";"
    Write-Host ''
    $parts = $schemaVersion -split '\.'
    Write-Host "  5. src\AppInstallerCLITests\TestData\"
    Write-Host "       Add ManifestV$($parts[0])_$($parts[1])-Singleton.yaml and ManifestV$($parts[0])_$($parts[1])-MultiFile-*.yaml."
    Write-Host ''
    Write-Host "  6. src\AppInstallerCLITests\AppInstallerCLITests.vcxproj and .vcxproj.filters"
    Write-Host "       Reference the new test manifest files."
    Write-Host ''
    Write-Host "  7. src\AppInstallerCLITests\YamlManifest.cpp"
    Write-Host "       Add test cases for manifest version $schemaVersion."
    exit 0
}

# ---------------------------------------------------------------------------
# BumpVersion: update latest/ JSON files and all source files
# ---------------------------------------------------------------------------

$oldVersion = $schemaVersion
$oldSuffix  = Get-CppSuffix $oldVersion
$newSuffix  = Get-CppSuffix $newVersion
$parts      = $newVersion -split '\.'
$newMajor   = $parts[0]
$newMinor   = $parts[1]

# Bump Step 1: Update $id and description in all latest/ JSON files
Write-Host ''
Write-Host '=== Bump Step 1: Update latest/ schema files ==='
foreach ($type in $schemaTypes) {
    $jsonFile = Join-Path $latestDir "manifest.$type.latest.json"
    $content  = Get-Content $jsonFile -Raw
    if ($content.Contains($oldVersion)) {
        if ($PSCmdlet.ShouldProcess($jsonFile, "Replace $oldVersion with $newVersion")) {
            $updated = $content.Replace($oldVersion, $newVersion)
            Save-TextFile $updated $jsonFile
            Write-Host "  Updated manifest.$type.latest.json ($oldVersion -> $newVersion)"
        }
    } else {
        Write-Warning "  manifest.$type.latest.json: '$oldVersion' not found. Inspect the file manually."
    }
}

# Bump Step 2: ManifestCommon.h -- add new version constant
Write-Host ''
Write-Host '=== Bump Step 2: ManifestCommon.h ==='
$mcContent = Get-Content $manifestCommonH -Raw

$newConstant  = "    // V$newMajor.$newMinor manifest version`n    constexpr std::string_view s_ManifestVersion${newSuffix} = `"$newVersion`"sv;`n"
$insertBefore = '    // Any new manifest version must also be added to'

if ($mcContent.Contains("s_ManifestVersion${newSuffix}")) {
    Write-Host "  s_ManifestVersion${newSuffix} already present - no change needed."
} elseif (-not $mcContent.Contains($insertBefore)) {
    Write-Warning "  Could not locate insertion point in ManifestCommon.h. Manual edit required."
} else {
    if ($PSCmdlet.ShouldProcess($manifestCommonH, "Add s_ManifestVersion${newSuffix}")) {
        $mcContent = $mcContent.Replace($insertBefore, "$newConstant`n$insertBefore")
        Save-TextFile $mcContent $manifestCommonH
        Write-Host "  Added s_ManifestVersion${newSuffix} = `"$newVersion`"sv"
    }
}

# Bump Step 3: ManifestSchema.h -- add five new IDX constants
Write-Host ''
Write-Host '=== Bump Step 3: ManifestSchema.h ==='
$mshContent = Get-Content $manifestSchemaH -Raw

if ($mshContent.Contains("IDX_MANIFEST_SCHEMA_${newSuffix}_LOCALE")) {
    Write-Host "  IDX_MANIFEST_SCHEMA_${newSuffix}_* already present - no change needed."
} else {
    # Find the last defined IDX constant ID number to determine the next available IDs.
    $allIds = [System.Text.RegularExpressions.Regex]::Matches($mshContent, '#define IDX_MANIFEST_SCHEMA_\w+\s+(\d+)') |
        ForEach-Object { [int]$_.Groups[1].Value }
    if (-not $allIds) { throw 'Could not parse existing IDX constants from ManifestSchema.h.' }
    $nextId = ($allIds | Measure-Object -Maximum).Maximum + 1

    if ($nextId + 4 -ge 300) {
        Write-Warning "  Next available ID would be $nextId; approaching the 300 limit. Manual review required."
    }

    $newBlock = @"

#define IDX_MANIFEST_SCHEMA_${newSuffix}_SINGLETON           $nextId
#define IDX_MANIFEST_SCHEMA_${newSuffix}_VERSION             $($nextId + 1)
#define IDX_MANIFEST_SCHEMA_${newSuffix}_INSTALLER           $($nextId + 2)
#define IDX_MANIFEST_SCHEMA_${newSuffix}_DEFAULTLOCALE       $($nextId + 3)
#define IDX_MANIFEST_SCHEMA_${newSuffix}_LOCALE              $($nextId + 4)

"@
    # Insert before the "Packages schema starts at 300" comment
    $insertBefore = '// Packages schema starts at 300'
    if (-not $mshContent.Contains($insertBefore)) {
        Write-Warning "  Could not locate insertion point in ManifestSchema.h. Manual edit required."
    } elseif ($PSCmdlet.ShouldProcess($manifestSchemaH, "Add IDX_MANIFEST_SCHEMA_${newSuffix}_* constants")) {
        $mshContent = $mshContent.Replace($insertBefore, "$newBlock$insertBefore")
        Save-TextFile $mshContent $manifestSchemaH
        Write-Host "  Added IDX_MANIFEST_SCHEMA_${newSuffix}_* (IDs $nextId-$($nextId + 4))"
    }
}

# Bump Step 4: ManifestSchema.rc -- append new resource entries pointing to latest/
Write-Host ''
Write-Host '=== Bump Step 4: ManifestSchema.rc ==='
$rcContent = Get-Content $rcFile -Raw

if ($rcContent.Contains("IDX_MANIFEST_SCHEMA_${newSuffix}_SINGLETON")) {
    Write-Host "  IDX_MANIFEST_SCHEMA_${newSuffix}_* already present in RC file - no change needed."
} else {
    $rcAppend = @"

IDX_MANIFEST_SCHEMA_${newSuffix}_SINGLETON         MANIFESTSCHEMA_RESOURCE_TYPE    "..\\..\\schemas\\JSON\\manifests\\latest\\manifest.singleton.latest.json"
IDX_MANIFEST_SCHEMA_${newSuffix}_VERSION           MANIFESTSCHEMA_RESOURCE_TYPE    "..\\..\\schemas\\JSON\\manifests\\latest\\manifest.version.latest.json"
IDX_MANIFEST_SCHEMA_${newSuffix}_INSTALLER         MANIFESTSCHEMA_RESOURCE_TYPE    "..\\..\\schemas\\JSON\\manifests\\latest\\manifest.installer.latest.json"
IDX_MANIFEST_SCHEMA_${newSuffix}_DEFAULTLOCALE     MANIFESTSCHEMA_RESOURCE_TYPE    "..\\..\\schemas\\JSON\\manifests\\latest\\manifest.defaultLocale.latest.json"
IDX_MANIFEST_SCHEMA_${newSuffix}_LOCALE            MANIFESTSCHEMA_RESOURCE_TYPE    "..\\..\\schemas\\JSON\\manifests\\latest\\manifest.locale.latest.json"
"@
    if ($PSCmdlet.ShouldProcess($rcFile, "Append IDX_MANIFEST_SCHEMA_${newSuffix}_* entries")) {
        $rcContent = $rcContent.TrimEnd() + $rcAppend + "`n"
        Save-TextFile $rcContent $rcFile
        Write-Host "  Appended IDX_MANIFEST_SCHEMA_${newSuffix}_* entries (pointing to latest/)"
    }
}

# Bump Step 5: ManifestSchemaValidation.cpp -- prepend new version block at top of if-chain
Write-Host ''
Write-Host '=== Bump Step 5: ManifestSchemaValidation.cpp ==='
$cppContent = Get-Content $schemaValidationCpp -Raw

if ($cppContent.Contains("s_ManifestVersion${newSuffix}")) {
    Write-Host "  s_ManifestVersion${newSuffix} already present - no change needed."
} else {
    # The current top of the if-chain begins with "if (manifestVersion >= ManifestVer{ s_ManifestVersionOLD })"
    $oldIfLine = "        if (manifestVersion >= ManifestVer{ s_ManifestVersion${oldSuffix} })"
    if (-not $cppContent.Contains($oldIfLine)) {
        Write-Warning "  Could not locate '$oldIfLine' in ManifestSchemaValidation.cpp. Manual edit required."
    } else {
        $newBlock = @"
        if (manifestVersion >= ManifestVer{ s_ManifestVersion${newSuffix} })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_${newSuffix}_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_${newSuffix}_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_${newSuffix}_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_${newSuffix}_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_${newSuffix}_LOCALE },
            };
        }
        else if (manifestVersion >= ManifestVer{ s_ManifestVersion${oldSuffix} })
"@
        if ($PSCmdlet.ShouldProcess($schemaValidationCpp, "Prepend s_ManifestVersion${newSuffix} block")) {
            $cppContent = $cppContent.Replace($oldIfLine, $newBlock)
            Save-TextFile $cppContent $schemaValidationCpp
            Write-Host "  Prepended s_ManifestVersion${newSuffix} block; old block demoted to 'else if'"
        }
    }
}

# Bump Step 6: ManifestVersion.cs -- add new version constant
Write-Host ''
Write-Host '=== Bump Step 6: ManifestVersion.cs ==='
$csContent = Get-Content $manifestVersionCs -Raw

if ($csContent.Contains("ManifestVersion${newSuffix}")) {
    Write-Host "  ManifestVersion${newSuffix} already present - no change needed."
} else {
    # Insert before the closing #pragma restore line
    $insertBefore = '#pragma warning restore SA1310'
    $newConstant  = @"

        /// <summary>
        /// V$newMajor.$newMinor manifest version.
        /// </summary>
        public const string ManifestVersion${newSuffix} = "$newVersion";

"@
    if (-not $csContent.Contains($insertBefore)) {
        Write-Warning "  Could not locate insertion point in ManifestVersion.cs. Manual edit required."
    } elseif ($PSCmdlet.ShouldProcess($manifestVersionCs, "Add ManifestVersion${newSuffix}")) {
        $csContent = $csContent.Replace($insertBefore, "$newConstant$insertBefore")
        Save-TextFile $csContent $manifestVersionCs
        Write-Host "  Added ManifestVersion${newSuffix} = `"$newVersion`""
    }
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

Write-Host ''
Write-Host '=== Done ==='
Write-Host ''
Write-Host 'Manual steps remaining:'
Write-Host ''
Write-Host "  1. src\AppInstallerCLITests\TestData\"
Write-Host "       Add ManifestV${newMajor}_${newMinor}-Singleton.yaml and ManifestV${newMajor}_${newMinor}-MultiFile-*.yaml."
Write-Host ''
Write-Host "  2. src\AppInstallerCLITests\AppInstallerCLITests.vcxproj and .vcxproj.filters"
Write-Host "       Reference the new test manifest files."
Write-Host ''
Write-Host "  3. src\AppInstallerCLITests\YamlManifest.cpp"
Write-Host "       Add test cases for manifest version $newVersion."
