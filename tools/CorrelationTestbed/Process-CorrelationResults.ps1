Param(
  [Parameter(Position = 0, HelpMessage = "The root location of the results.")]
  [String] $ResultsPath
)

$resultFile = Join-Path $ResultsPath "results.csv"
$failedFile = Join-Path $ResultsPath "failed.csv"
$statsFile = Join-Path $ResultsPath "stats.json"

if (Test-Path $resultFile)
{
    Remove-Item $resultFile -Force
}

if (Test-Path $failedFile)
{
    Remove-Item $failedFile -Force
}

$stats = @{
    Total = 0
    Missing = 0
    Completed = 0
    Failed = 0
    CorrelatePackageKnown = 0
    CorrelateArchive = 0
}

# Aggregate results in a single CSV file
foreach ($result in (Get-ChildItem $ResultsPath -Directory))
{
    $stats.Total++

    $resultJSON = Join-Path $result.FullName "install_and_correlate.json"
    if (Test-Path $resultJSON)
    {
        $resultObj = (Get-Content -Path $resultJSON -Encoding utf8 | ConvertFrom-Json)
    }

    if (-not $resultObj)
    {
        # Result JSON file does not exist or is empty
        $stats.Missing++
        continue
    }

    if ($resultObj.HRESULT -eq 0)
    {
        $stats.Completed++
        $stats.CorrelateArchive += $resultObj.CorrelateArchive
        $stats.CorrelatePackageKnown += $resultObj.CorrelatePackageKnown
        Export-Csv -InputObject ($resultObj | Select-Object -Property * -ExcludeProperty @("Error", "Phase", "Action", "HRESULT") ) -Path $resultFile -Append
    }
    else
    {
        $stats.Failed++
        Export-Csv -InputObject $resultObj -Path $failedFile -Append
    }
}

# Write some stats to a file for quick evaluation
$stats.CompletedRatio = $stats.Completed / $stats.Total
$stats.CorrelateArchiveRatio = $stats.CorrelateArchive / $stats.Completed
$stats.CorrelatePackageKnownRatio = $stats.CorrelatePackageKnown / $stats.Completed
$stats | ConvertTo-Json | Out-File $statsFile -Force
