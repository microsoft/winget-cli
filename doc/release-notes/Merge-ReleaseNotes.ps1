<#
.SYNOPSIS
    Combines a release's Markdown fragments into one document on standard output.
.PARAMETER Version
    The major.minor release to generate. Defaults to the binary version.
#>
[CmdletBinding()]
param(
    [ValidatePattern('^\d+\.\d+$')]
    [string] $Version
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $Version) {
    $versionFile = Join-Path $PSScriptRoot '..\..\src\binver\binver\version.h'
    $versionHeader = Get-Content -LiteralPath $versionFile -Raw
    if ($versionHeader -notmatch '(?m)^#define VERSION_MAJOR\s+(\d+)') {
        throw "Cannot read VERSION_MAJOR from '$versionFile'."
    }
    $major = $Matches[1]
    if ($versionHeader -notmatch '(?m)^#define VERSION_MINOR\s+(\d+)') {
        throw "Cannot read VERSION_MINOR from '$versionFile'."
    }
    $Version = "$major.$($Matches[1])"
}

$releaseDirectory = Join-Path $PSScriptRoot $Version
if (-not (Test-Path -LiteralPath $releaseDirectory -PathType Container)) {
    throw "Release notes directory does not exist: '$releaseDirectory'."
}

$categories = [ordered]@{
    'New Features' = [System.Collections.Generic.List[string]]::new()
    'Bug Fixes' = [System.Collections.Generic.List[string]]::new()
}
$minorFixes = [System.Collections.Generic.List[string]]::new()

[string[]] $files = @(Get-ChildItem -LiteralPath $releaseDirectory -Filter '*.md' -File |
    ForEach-Object { $_.FullName })
[Array]::Sort($files, [StringComparer]::Ordinal)

foreach ($file in $files) {
    $content = [System.IO.File]::ReadAllText($file).Replace("`r`n", "`n").Trim()
    $headings = [regex]::Matches($content, '(?m)^## ([^\n]+)')
    if (-not $headings.Count -or $headings[0].Index -ne 0) {
        throw "Invalid release note '$file': expected a New Features or Bug Fixes heading."
    }
    for ($index = 0; $index -lt $headings.Count; ++$index) {
        $heading = $headings[$index]
        $category = $heading.Groups[1].Value
        if (-not $categories.Contains($category)) {
            throw "Invalid release note '$file': unknown category '$category'."
        }
        $start = $heading.Index + $heading.Length
        $end = if ($index + 1 -lt $headings.Count) { $headings[$index + 1].Index } else { $content.Length }
        $body = $content.Substring($start, $end - $start).Trim()
        if (-not $body) {
            continue
        }
        if ($body -match '(?m)^#{1,2} ') {
            throw "Invalid release note '$file': use only level-three or deeper body headings."
        }
        foreach ($subsection in [regex]::Split($body, '(?m)(?=^### )')) {
            $subsection = $subsection.Trim()
            if (-not $subsection) {
                continue
            }
            if ($subsection -match '\A### Minor Bug Fixes(?:\n|\z)') {
                if ($category -ne 'Bug Fixes') {
                    throw "Invalid release note '$file': Minor Bug Fixes must be under Bug Fixes."
                }
                $items = $subsection.Substring($Matches[0].Length).Trim()
                foreach ($line in $items -split "`n") {
                    $line = $line.Trim()
                    if (-not $line -or $line -match '^[*-] <Describe a minor bug fix in one line\.>$') {
                        continue
                    }
                    if ($line -notmatch '^[*-] \S.*$') {
                        throw "Invalid release note '$file': each minor bug fix must be a single-line bullet."
                    }
                    $minorFixes.Add("* $($line.Substring(2))")
                }
            }
            elseif (-not $subsection.Contains('<Short title>') -and
                -not $subsection.Contains('<Describe the user-visible change, including usage or limitations when relevant.>')) {
                $categories[$category].Add($subsection)
            }
        }
    }
}

if ($minorFixes.Count) {
    $categories['Bug Fixes'].Add("### Minor Bug Fixes`n`n" + ($minorFixes -join "`n"))
}

$sections = [System.Collections.Generic.List[string]]::new()
$sections.Add("## New in v$Version")
foreach ($category in $categories.Keys) {
    if ($categories[$category].Count) {
        $sections.Add("## $category")
        $sections.Add(($categories[$category] -join "`n`n"))
    }
}
if ($sections.Count -eq 1) {
    $sections.Add('Nothing yet.')
}

($sections -join "`n`n") + "`n"
