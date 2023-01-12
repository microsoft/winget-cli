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
    CorrelateMetadata = 0
    CorrelationDisagreement = 0
    CorrelateArchiveRatio = 0
    CorrelatePackageKnownRatio = 0
    CorrelateMetadataRatio = 0
    CorrelationDisagreementRatio = 0
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

    $metadataJSON = Join-Path $result.FullName "metadata_output.json"
    if (Test-Path $metadataJSON)
    {
        $metadataObj = (Get-Content -Path $metadataJSON -Encoding utf8 | ConvertFrom-Json)
    }

    if ($resultObj.HRESULT -eq 0)
    {
        $stats.Completed++
        $stats.CorrelateArchive += $resultObj.CorrelateArchive
        $stats.CorrelatePackageKnown += $resultObj.CorrelatePackageKnown
        if ($metadataObj -and $metadataObj.status -eq "Success")
        {
            $stats.CorrelateMetadata += 1
            Add-Member -InputObject $resultObj -MemberType NoteProperty -Name "CorrelateMetadata" -Value 1 -Force
            Add-Member -InputObject $resultObj -MemberType NoteProperty -Name "MetadataName" -Value $metadataObj.metadata[0].metadata[0].AppsAndFeaturesEntries[0].DisplayName -Force
            Add-Member -InputObject $resultObj -MemberType NoteProperty -Name "MetadataPublisher" -Value $metadataObj.metadata[0].metadata[0].AppsAndFeaturesEntries[0].Publisher -Force

            if ($resultObj.PackageKnownName -ne "" -and $resultObj.MetadataName -ne "" -and $resultObj.PackageKnownName -ne $resultObj.MetadataName)
            {
                $stats.CorrelationDisagreement += 1
            }
        }
        Export-Csv -InputObject ($resultObj | Select-Object -Property * -ExcludeProperty @("Error", "Phase", "Action", "HRESULT") ) -Path $resultFile -Append -Encoding utf8
    }
    else
    {
        $stats.Failed++
        Export-Csv -InputObject $resultObj -Path $failedFile -Append -Encoding utf8
    }
}

# Write some stats to a file for quick evaluation
$stats.CompletedRatio = $stats.Completed / $stats.Total
if ($stats.Completed -ne 0)
{
    $stats.CorrelateArchiveRatio = $stats.CorrelateArchive / $stats.Completed
    $stats.CorrelatePackageKnownRatio = $stats.CorrelatePackageKnown / $stats.Completed
    $stats.CorrelateMetadataRatio = $stats.CorrelateMetadata / $stats.Completed
    $stats.CorrelationDisagreementRatio = $stats.CorrelationDisagreement / $stats.Completed
}
$stats | ConvertTo-Json | Out-File $statsFile -Force
