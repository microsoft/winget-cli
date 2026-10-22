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

[string[]] $files = @(Get-ChildItem -LiteralPath $releaseDirectory -Filter '*.md' -File |
    ForEach-Object { $_.FullName })
[Array]::Sort($files, [StringComparer]::Ordinal)

foreach ($file in $files) {
    $content = [System.IO.File]::ReadAllText($file).Replace("`r`n", "`n").Trim()
    $headings = [regex]::Matches($content, '(?m)^## ([^\n]+)')
    if (-not $headings.Count -or $headings[0].Index -ne 0) {
        throw "Invalid release note '$file': expected a New Features or Bug Fixes heading."
    }
    $hasNotes = $false
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
        if ($body -match '(?m)^#{1,2} ' -or
            $body.Contains('<Short title>') -or
            $body.Contains('<Describe the user-visible change, including usage or limitations when relevant.>')) {
            throw "Invalid release note '$file': fill out the template and use only level-three or deeper body headings."
        }
        $categories[$category].Add($body)
        $hasNotes = $true
    }
    if (-not $hasNotes) {
        throw "Invalid release note '$file': add at least one feature or bug fix."
    }
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
