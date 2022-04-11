Param(
  [Parameter(Position = 0, HelpMessage = "The root location of the results.")]
  [String] $ResultsPath
)

$resultFile = Join-Path $ResultsPath "results.csv"
$failedFile = Join-Path $ResultsPath "failed.csv"

if (Test-Path $resultFile)
{
    Remove-Item $resultFile -Force
}

if (Test-Path $failedFile)
{
    Remove-Item $failedFile -Force
}

foreach ($result in (Get-ChildItem $ResultsPath -Directory))
{
    $resultJSON = Join-Path $result "install_and_correlate.json"
    if (-not (Test-Path $resultJSON))
    {
        continue
    }
    
    $resultObj = (Get-Content -Path $resultJSON -Encoding utf8 | ConvertFrom-Json)

    if ($resultObj.HRESULT -eq 0)
    {
        Export-Csv -InputObject ($resultObj | Select-Object -Property * -ExcludeProperty @("Error", "Phase", "Action", "HRESULT") ) -Path $resultFile -Append
    }
    else
    {
        Export-Csv -InputObject $resultObj -Path $failedFile -Append
    }
}
