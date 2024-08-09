Param(
  [String[]] $DesktopAppInstallerDependencyPath
)

$ProgressPreference = 'SilentlyContinue'

$desktopPath = "C:\Users\WDAGUtilityAccount\Desktop"

Write-Host @"
--> Installing WinGet

"@

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
