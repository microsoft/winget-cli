Param(
  [String] $DesktopAppInstallerPath,
  [String[]] $DesktopAppInstallerDependencyPath,
  [String] $PackageIdentifier,
  [String] $SourceName,
  [String] $OutputPath,
  [Switch] $UseDev
)

function Get-ARPTable {
  $registry_paths = @('HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*','HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*', 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*', 'HKCU:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*')
  return Get-ItemProperty $registry_paths -ErrorAction SilentlyContinue | 
       Select-Object DisplayName, DisplayVersion, Publisher, @{N='ProductCode'; E={$_.PSChildName}} |
       Where-Object {$null -ne $_.DisplayName }
}

$ProgressPreference = 'SilentlyContinue'

$desktopPath = "C:\Users\WDAGUtilityAccount\Desktop"

$regFilesDirPath = Join-Path $desktopPath "RegFiles"

if (Test-Path $regFilesDirPath)
{
  foreach ($regFile in (Get-ChildItem $regFilesDirPath))
  {

    Write-Host @"
--> Importing reg file $($regFile.FullName)
"@
    reg import $($regFile.FullName)
  }
}

Write-Host @"
--> Installing WinGet

"@

if ($UseDev)
{
  foreach($dependency in $DesktopAppInstallerDependencyPath)
  {
    Write-Host @"
  ----> Installing $dependency
"@
    Add-AppxPackage -Path $dependency
  }

  Write-Host @"
  ----> Enabling dev mode
"@
  reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock" /t REG_DWORD /f /v "AllowDevelopmentWithoutDevLicense" /d "1"

  $devPackageManifestPath = Join-Path $desktopPath "DevPackage\AppxManifest.xml"
  Write-Host @"
  ----> Installing $devPackageManifestPath
"@
  Add-AppxPackage -Path $devPackageManifestPath -Register
}
else
{
  Add-AppxPackage -Path $DesktopAppInstallerPath -DependencyPath $DesktopAppInstallerDependencyPath
}

$originalARP = Get-ARPTable

Write-Host @"

--> Installing $PackageIdentifier

"@

$installAndCorrelateOutPath = Join-Path $OutputPath "install_and_correlate.json"

$installAndCorrelationExpression = Join-Path $desktopPath "InstallAndCheckCorrelation\InstallAndCheckCorrelation.exe"
$installAndCorrelationExpression = -join($installAndCorrelationExpression, ' -id "', $PackageIdentifier, '" -src "', $SourceName, '" -out "', $installAndCorrelateOutPath, '"')

if ($UseDev)
{
  $installAndCorrelationExpression = -join($installAndCorrelationExpression, ' -dev')
}

Invoke-Expression $installAndCorrelationExpression

Write-Host @"

--> Copying logs
"@

if ($UseDev)
{
  Copy-Item -Recurse (Join-Path $env:LOCALAPPDATA "Packages\WinGetDevCLI_8wekyb3d8bbwe\LocalState\DiagOutputDir") $OutputPath
}
else
{
  Copy-Item -Recurse (Join-Path $env:LOCALAPPDATA "Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\DiagOutputDir") $OutputPath
}


Write-Host @"

--> Comparing ARP Entries
"@

$arpCompared = (Compare-Object (Get-ARPTable) $originalARP -Property DisplayName,DisplayVersion,Publisher,ProductCode)
$arpCompared | Select-Object -Property * -ExcludeProperty SideIndicator | Format-Table

$arpCompared | Select-Object -Property * -ExcludeProperty SideIndicator | Format-Table | Out-File (Join-Path $OutputPath "ARPCompare.txt")

"Done" | Out-File (Join-Path $OutputPath "done.txt")
