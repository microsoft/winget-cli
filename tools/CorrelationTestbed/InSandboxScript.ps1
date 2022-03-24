Param(
  [String] $DesktopAppInstallerPath,
  [String[]] $DesktopAppInstallerDependencyPath,
  [String] $PackageIdentifier,
  [String] $SourceName,
  [String] $OutputPath
)

function Get-ARPTable {
  $registry_paths = @('HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*','HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*', 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*', 'HKCU:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*')
  return Get-ItemProperty $registry_paths -ErrorAction SilentlyContinue | 
       Select-Object DisplayName, DisplayVersion, Publisher, @{N='ProductCode'; E={$_.PSChildName}} |
       Where-Object {$null -ne $_.DisplayName }
}

Write-Host @"
--> Installing WinGet

"@

$ProgressPreference = 'SilentlyContinue'
Add-AppxPackage -Path $DesktopAppInstallerPath -DependencyPath $DesktopAppInstallerDependencyPath

$originalARP = Get-ARPTable

Write-Host @"

--> Installing $PackageIdentifier

"@

winget install --id "$PackageIdentifier" -s "$SourceName" --verbose-logs --accept-source-agreements --accept-package-agreements

Write-Host @"

--> Comparing ARP Entries
"@

(Compare-Object (Get-ARPTable) $originalARP -Property DisplayName,DisplayVersion,Publisher,ProductCode)| Select-Object -Property * -ExcludeProperty SideIndicator | Format-Table

"Test output" | Out-File (Join-Path $OutputPath "test-out.txt")
