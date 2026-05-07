#Requires -Version 5.1

<#
.SYNOPSIS
    Runs manifest validation on every manifest under a given path and produces an HTML report.

.DESCRIPTION
    Discovers all manifest directories under the specified path (the leaf directories
    containing YAML files, as found in a winget-pkgs clone), runs 'wingetdev validate'
    on each, shows progress, and writes a self-contained HTML report with no external
    script or style references.

    wingetdev is resolved from PATH. An explicit path may be provided via -WingetDevPath
    if wingetdev is not on PATH.

.PARAMETER ManifestsPath
    Path to search for manifests. May be the root of a winget-pkgs clone (the script will
    descend into its 'manifests' subdirectory if present) or a manifests directory directly.

.PARAMETER WingetDevPath
    Optional explicit path to wingetdev.exe. If not provided, wingetdev is resolved from PATH.

.PARAMETER OutputPath
    Optional path for the HTML report file. Defaults to 'manifest-validation-report.html'
    in the current working directory.

.EXAMPLE
    .\Invoke-ManifestValidation.ps1 -ManifestsPath C:\repos\winget-pkgs

.EXAMPLE
    .\Invoke-ManifestValidation.ps1 -ManifestsPath C:\repos\winget-pkgs\manifests `
        -WingetDevPath C:\tools\wingetdev.exe -OutputPath C:\reports\results.html
#>

[CmdletBinding()]
Param(
    [Parameter(Mandatory = $true, Position = 0, HelpMessage = "Path to search for manifests (winget-pkgs root or manifests directory).")]
    [string] $ManifestsPath,

    [Parameter(HelpMessage = "Path to wingetdev.exe. Resolved from PATH if not provided.")]
    [string] $WingetDevPath,

    [Parameter(HelpMessage = "Output path for the HTML report.")]
    [string] $OutputPath = (Join-Path (Get-Location) "manifest-validation-report.html"),

    [Parameter(HelpMessage = "Launch the HTML report in the default browser when complete.")]
    [switch] $Launch,

    [Parameter(HelpMessage = "Exclude warnings from the report results table.")]
    [switch] $SuppressWarnings,

    [Parameter(HelpMessage = "Resume from an existing report file, skipping already-completed top-level directories.")]
    [switch] $Resume
)

$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# HTML encoding helper (avoids requiring System.Web)
# ---------------------------------------------------------------------------
function ConvertTo-HtmlEncoded([string] $text)
{
    $text.Replace('&', '&amp;').Replace('<', '&lt;').Replace('>', '&gt;').Replace('"', '&quot;')
}

# ---------------------------------------------------------------------------
# Resolve wingetdev
# ---------------------------------------------------------------------------
if ($WingetDevPath)
{
    if (-not (Test-Path $WingetDevPath -PathType Leaf))
    {
        Write-Error -Category InvalidArgument -Message "wingetdev.exe not found at: $WingetDevPath"
    }
    $wingetDev = $WingetDevPath
}
else
{
    $wingetDevCmd = Get-Command "wingetdev" -ErrorAction SilentlyContinue
    if (-not $wingetDevCmd)
    {
        Write-Error -Category ObjectNotFound -Message @"
wingetdev was not found on PATH.
Either add wingetdev to your PATH, or provide its location with -WingetDevPath.
"@
    }
    $wingetDev = $wingetDevCmd.Source
}

Write-Host "Using wingetdev: $wingetDev"

$wingetDevVersion = (& $wingetDev --version 2>&1 | Out-String).Trim()

# ---------------------------------------------------------------------------
# Resolve manifests search root
# ---------------------------------------------------------------------------
$ManifestsPath = [System.IO.Path]::GetFullPath($ManifestsPath)
if (-not (Test-Path $ManifestsPath -PathType Container))
{
    Write-Error -Category InvalidArgument -Message "ManifestsPath does not exist or is not a directory: $ManifestsPath"
}

# Support passing either the repo root (which contains a 'manifests' subdirectory)
# or the manifests directory itself.
$manifestsSubdir = Join-Path $ManifestsPath "manifests"
$searchRoot = if (Test-Path $manifestsSubdir -PathType Container) { $manifestsSubdir } else { $ManifestsPath }

Write-Host "Discovering top-level directories under: $searchRoot"

$tier1Dirs = Get-ChildItem $searchRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name
if (-not $tier1Dirs)
{
    Write-Error -Category ObjectNotFound -Message "No subdirectories found under: $searchRoot"
}
$tier1Total   = $tier1Dirs.Count
$tier1Current = 0

Write-Host "Found $tier1Total top-level directories."

# ---------------------------------------------------------------------------
# Initialize script-level state (shared with Write-ValidationReport)
# ---------------------------------------------------------------------------
$script:existingRowsHtml   = ''
$script:completedTier1Dirs = [System.Collections.Generic.List[string]]::new()

$passed   = 0
$warnings = 0
$failed   = 0
$errors   = 0
$total    = 0

if ($Resume)
{
    if (-not (Test-Path $OutputPath -PathType Leaf))
    {
        Write-Error -Category ObjectNotFound -Message "Resume requested but no report file found at: $OutputPath"
    }

    Write-Host "Reading resume state from: $OutputPath"
    $content = Get-Content $OutputPath -Raw -Encoding utf8

    if ($content -notmatch '(?s)<script type="application/json" id="validation-state">(.*?)</script>')
    {
        Write-Error -Category InvalidData -Message "Could not find embedded resume state in: $OutputPath"
    }
    $state = $Matches[1].Trim() | ConvertFrom-Json

    foreach ($d in $state.completedTier1Dirs) { $script:completedTier1Dirs.Add($d) }
    $passed   = [int]$state.passed
    $warnings = [int]$state.warnings
    $failed   = [int]$state.failed
    $errors   = [int]$state.errors
    $total    = [int]$state.total

    if ($content -match '(?s)<tbody id="tb">(.*?)</tbody>')
    {
        $script:existingRowsHtml = $Matches[1].Trim()
    }

    Write-Host ("Resuming: {0} top-level directories already complete, {1} manifests already processed." -f $script:completedTier1Dirs.Count, $total)
}

# ---------------------------------------------------------------------------
# Report writing helper
# ---------------------------------------------------------------------------
function Write-ValidationReport
{
    param(
        [System.Collections.Generic.List[PSCustomObject]] $Results,
        [int] $Total,
        [int] $Passed,
        [int] $Warnings,
        [int] $Failed,
        [int] $Errors
    )

    # Always back up the existing report before overwriting - guards against data loss
    # during a long-running write. State is already in memory at this point.
    if (Test-Path $OutputPath -PathType Leaf)
    {
        $dir    = [System.IO.Path]::GetDirectoryName($OutputPath)
        $base   = [System.IO.Path]::GetFileNameWithoutExtension($OutputPath)
        $ext    = [System.IO.Path]::GetExtension($OutputPath)
        $backup = Join-Path $dir "$base.backup$ext"
        Move-Item $OutputPath $backup -Force
    }

    $timestamp        = (Get-Date).ToString('yyyy-MM-dd HH:mm:ss')
    $escapedRoot      = ConvertTo-HtmlEncoded $searchRoot
    $escapedWingetVer = ConvertTo-HtmlEncoded $wingetDevVersion

    # Rows from the previous (resumed) run come first; new rows from this run follow.
    $newRowsHtml = ($Results | ForEach-Object {
        $statusClass   = $_.Status.ToLower()
        $escapedPath   = ConvertTo-HtmlEncoded $_.RelativePath
        $escapedOutput = (ConvertTo-HtmlEncoded $_.Output) -replace "`r?`n", '<br>'
        "      <tr class='$statusClass'><td>$escapedPath</td><td class='sc'>$($_.Status)</td><td class='oc'>$escapedOutput</td></tr>"
    }) -join "`n"

    $rowsHtml = if ($script:existingRowsHtml -and $newRowsHtml) { "$($script:existingRowsHtml)`n$newRowsHtml" }
                elseif ($script:existingRowsHtml)                { $script:existingRowsHtml }
                else                                              { $newRowsHtml }

    # Embed progress state so the run can be resumed later.
    $stateJson = [PSCustomObject]@{
        completedTier1Dirs = @($script:completedTier1Dirs)
        tier1Total = $tier1Total
        total    = $Total
        passed   = $Passed
        warnings = $Warnings
        failed   = $Failed
        errors   = $Errors
    } | ConvertTo-Json -Compress

    $completed = $script:completedTier1Dirs.Count
    $bannerHtml = if ($completed -lt $tier1Total) {
        "  <div class=`"banner`"><svg width=`"22`" height=`"22`" viewBox=`"0 0 24 24`" fill=`"none`" stroke=`"#e65100`" stroke-width=`"2`" stroke-linecap=`"round`" stroke-linejoin=`"round`"><path d=`"M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z`"/><line x1=`"12`" y1=`"9`" x2=`"12`" y2=`"13`"/><line x1=`"12`" y1=`"17`" x2=`"12.01`" y2=`"17`"/></svg><div class=`"banner-text`"><strong>Validation in progress &mdash; results are partial</strong><span>$completed of $tier1Total top-level directories complete.</span></div></div>"
    } else { '' }

    $html = @"
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Manifest Validation Report</title>
  <style>
    *,*::before,*::after{box-sizing:border-box}
    body{font-family:"Segoe UI",Arial,sans-serif;margin:0;padding:20px;background:#f4f4f4;color:#222}
    h1{margin-bottom:4px}
    .meta{color:#666;font-size:.875em;margin-bottom:18px}
    .summary{display:flex;flex-wrap:wrap;gap:10px;margin-bottom:18px}
    .card{background:#fff;border-radius:6px;padding:12px 22px;text-align:center;box-shadow:0 1px 3px rgba(0,0,0,.12);min-width:100px}
    .card .n{font-size:2em;font-weight:700;line-height:1.1}
    .card .l{font-size:.78em;color:#666;margin-top:3px;text-transform:uppercase;letter-spacing:.04em}
    .ct .n{color:#333}
    .cp .n{color:#2e7d32}
    .cw .n{color:#e65100}
    .cf .n{color:#c62828}
    .ce .n{color:#6a1b9a}
    .controls{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin-bottom:10px}
    .fi{padding:6px 11px;font-size:.9em;border:1px solid #ccc;border-radius:4px;width:340px}
    .fi:focus{outline:none;border-color:#0078d4;box-shadow:0 0 0 2px rgba(0,120,212,.18)}
    select{padding:6px 10px;font-size:.9em;border:1px solid #ccc;border-radius:4px}
    .cl{color:#666;font-size:.85em}
    table{width:100%;border-collapse:collapse;background:#fff;border-radius:6px;overflow:hidden;box-shadow:0 1px 3px rgba(0,0,0,.12)}
    thead th{background:#0078d4;color:#fff;padding:9px 13px;text-align:left;cursor:pointer;user-select:none;white-space:nowrap}
    thead th:hover{background:#006cbd}
    thead th.sa::after{content:" \u25b2"}
    thead th.sd::after{content:" \u25bc"}
    tbody tr{border-bottom:1px solid #f0f0f0}
    tbody tr:last-child{border-bottom:none}
    tbody tr:hover td{background:#fafafa}
    tbody tr.pass{border-left:4px solid #2e7d32}
    tbody tr.warning{border-left:4px solid #e65100}
    tbody tr.fail{border-left:4px solid #c62828}
    tbody tr.error{border-left:4px solid #6a1b9a}
    td{padding:8px 13px;vertical-align:top;font-size:.875em}
    .sc{font-weight:600;white-space:nowrap}
    tr.pass    .sc{color:#2e7d32}
    tr.warning .sc{color:#e65100}
    tr.fail    .sc{color:#c62828}
    tr.error   .sc{color:#6a1b9a}
    .oc{font-family:Consolas,"Courier New",monospace;font-size:.82em;color:#555;max-width:560px;word-break:break-word}
    .hidden{display:none!important}
    .banner{display:flex;align-items:center;gap:12px;background:#fff8e1;border:1px solid #ffc107;border-left:5px solid #e65100;border-radius:6px;padding:12px 16px;margin-bottom:18px}
    .banner svg{flex-shrink:0;animation:pulse 1.6s ease-in-out infinite}
    .banner-text strong{display:block;color:#bf360c}
    .banner-text span{font-size:.85em;color:#555}
    @keyframes pulse{0%,100%{opacity:1}50%{opacity:.35}}
  </style>
</head>
<body>
  <h1>Manifest Validation Report</h1>
$bannerHtml
  <div class="meta">
    Generated: $timestamp &nbsp;&bull;&nbsp;
    wingetdev: $escapedWingetVer &nbsp;&bull;&nbsp;
    Path: $escapedRoot
  </div>

  <div class="summary">
    <div class="card ct"><div class="n">$Total</div><div class="l">Total</div></div>
    <div class="card cp"><div class="n">$Passed</div><div class="l">Pass</div></div>
    <div class="card cw"><div class="n">$Warnings</div><div class="l">Warning</div></div>
    <div class="card cf"><div class="n">$Failed</div><div class="l">Fail</div></div>
    <div class="card ce"><div class="n">$Errors</div><div class="l">Error</div></div>
  </div>

  <div class="controls">
    <input type="text" class="fi" id="fi" placeholder="Filter by path or output&hellip;" oninput="applyFilters()">
    <select id="sf" onchange="applyFilters()">
      <option value="">All statuses</option>
      <option value="pass">Pass</option>
      <option value="warning">Warning</option>
      <option value="fail">Fail</option>
      <option value="error">Error</option>
    </select>
    <span class="cl" id="cl"></span>
  </div>

  <table>
    <thead>
      <tr>
        <th onclick="sortTable(0)">Path</th>
        <th onclick="sortTable(1)">Status</th>
        <th onclick="sortTable(2)">Output</th>
      </tr>
    </thead>
    <tbody id="tb">
$rowsHtml
    </tbody>
  </table>

  <script>
    var sc=-1,sa=true;
    function applyFilters(){
      var txt=document.getElementById('fi').value.toLowerCase();
      var st=document.getElementById('sf').value;
      var rows=document.querySelectorAll('#tb tr');
      var vis=0;
      rows.forEach(function(r){
        var ok=(!txt||r.textContent.toLowerCase().indexOf(txt)!==-1)&&(!st||r.classList.contains(st));
        r.classList.toggle('hidden',!ok);
        if(ok)vis++;
      });
      document.getElementById('cl').textContent='Showing '+vis+' of '+rows.length;
    }
    function sortTable(col){
      var tb=document.getElementById('tb');
      var rows=Array.from(tb.querySelectorAll('tr'));
      if(sc===col){sa=!sa;}else{sc=col;sa=true;}
      rows.sort(function(a,b){
        var at=a.cells[col]?a.cells[col].textContent.trim():'';
        var bt=b.cells[col]?b.cells[col].textContent.trim():'';
        return sa?at.localeCompare(bt):bt.localeCompare(at);
      });
      rows.forEach(function(r){tb.appendChild(r);});
      document.querySelectorAll('thead th').forEach(function(th){th.classList.remove('sa','sd');});
      document.querySelectorAll('thead th')[col].classList.add(sa?'sa':'sd');
    }
    applyFilters();
  </script>
  <script type="application/json" id="validation-state">$stateJson</script>
</body>
</html>
"@

    $html | Out-File -FilePath $OutputPath -Encoding utf8 -Force
}

# ---------------------------------------------------------------------------
# Validate manifests with two-tier progress
# ---------------------------------------------------------------------------
$results = [System.Collections.Generic.List[PSCustomObject]]::new()

foreach ($tier1 in $tier1Dirs)
{
    $tier1Current++
    Write-Progress -Id 1 -Activity "Processing top-level directories" `
                   -Status "($tier1Current / $tier1Total) $($tier1.Name)" `
                   -PercentComplete (($tier1Current / $tier1Total) * 100)

    if ($script:completedTier1Dirs.Contains($tier1.Name))
    {
        Write-Host "Skipping (already complete): $($tier1.Name)"
        continue
    }

    # Discover manifest directories (leaf dirs with .yaml files) under this tier-1 dir.
    $yamlFiles    = Get-ChildItem $tier1.FullName -Recurse -File -Filter "*.yaml" -ErrorAction SilentlyContinue
    $manifestDirs = if ($yamlFiles) { $yamlFiles | Select-Object -ExpandProperty DirectoryName | Sort-Object -Unique } else { @() }
    $tier2Total   = $manifestDirs.Count
    $tier2Current = 0
    $total       += $tier2Total

    foreach ($dir in $manifestDirs)
    {
        $tier2Current++
        $relativePath = $dir.Substring($searchRoot.Length).TrimStart([char]'\', [char]'/')

        Write-Progress -Id 2 -ParentId 1 -Activity "Validating manifests" `
                       -Status "($tier2Current / $tier2Total) $relativePath" `
                       -PercentComplete (($tier2Current / $tier2Total) * 100)

        $output   = & $wingetDev validate $dir 2>&1 | Out-String
        $exitCode = $LASTEXITCODE

        $status = if     ($output -match 'Manifest validation succeeded with warnings') { 'Warning' }
                  elseif ($output -match 'Manifest validation succeeded')                { 'Pass'    }
                  elseif ($output -match 'Manifest validation failed')                   { 'Fail'    }
                  else                                                                   { 'Error'   }

        switch ($status)
        {
            'Pass'    { $passed++;   break }
            'Warning' { $warnings++; break }
            'Fail'    { $failed++;   break }
            'Error'   { $errors++;   break }
        }

        $keepInReport = $status -ne 'Pass' -and (-not $SuppressWarnings -or $status -ne 'Warning')
        if ($keepInReport)
        {
            $results.Add([PSCustomObject]@{
                RelativePath = $relativePath
                AbsolutePath = $dir
                Status       = $status
                ExitCode     = $exitCode
                Output       = $output.Trim()
            })
        }
    }

    Write-Progress -Id 2 -Completed

    $script:completedTier1Dirs.Add($tier1.Name)
    Write-ValidationReport -Results $results -Total $total -Passed $passed `
                           -Warnings $warnings -Failed $failed -Errors $errors
}

Write-Progress -Id 1 -Completed

Write-Host ""
Write-Host ("Results: Total={0}  Pass={1}  Warning={2}  Fail={3}  Error={4}" -f $total, $passed, $warnings, $failed, $errors)

if ($Launch)
{
    Start-Process $OutputPath
}
else
{
    Write-Host "Report created at $OutputPath"
}
