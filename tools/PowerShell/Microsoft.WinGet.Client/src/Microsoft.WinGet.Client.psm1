. $PSScriptRoot\Library\Get-WinGetVersion.ps1

$RequiredVersion = "1.1.12653"

$version = Get-WinGetVersion
if ($version -lt $RequiredVersion) {
    # Need to do localization
    Write-Host "Windows Package Manager is missing. For more information on installing the Windows Package Manager. `n  Visit: https://github.com/microsoft/winget-cli#installing-the-client"
    throw [WinGetVersionMismatch]::new("Requires Windows Package Manager $RequiredVersion or later to be installed.")
}

Get-ChildItem -Path $PSScriptRoot\Library -Filter *.ps1 | foreach-object { . $_.FullName }

class WinGetVersionMismatch : Exception {
	WinGetVersionMismatch([string] $message) : base($message) {}
}
