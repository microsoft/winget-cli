Write-Information "[winget] Downloading Appx packages"

try {

    Push-Location $env:TEMP

    # https://github.com/microsoft/winget-cli/issues/2230
    if (-not (Test-Path 'Microsoft.DesktopAppInstaller.msixbundle')) {
        Write-Information "`tMicrosoft.DesktopAppInstaller"
        Invoke-WebRequest 'https://aka.ms/getwinget' -OutFile 'Microsoft.DesktopAppInstaller.msixbundle' -UseBasicParsing -ErrorAction Stop
    }
    if (-not (Test-Path 'Microsoft.UI.Xaml.zip')) {
        Write-Information "`tMicrosoft.UI.Xaml"
        Invoke-WebRequest 'https://www.nuget.org/api/v2/package/Microsoft.UI.Xaml/2.7.1' -OutFile 'Microsoft.UI.Xaml.zip' -UseBasicParsing -ErrorAction Stop
    }
    if (-not (Test-Path 'Microsoft.VCLibs.x64.14.Desktop.appx')) {
        Write-Information "`tMicrosoft.VCLibs.x64.14"
        Invoke-WebRequest 'https://aka.ms/Microsoft.VCLibs.x64.14.00.Desktop.appx' -OutFile 'Microsoft.VCLibs.x64.14.Desktop.appx' -UseBasicParsing -ErrorAction Stop
    }
    if (-not (Test-Path '.\Microsoft.UI.Xaml\tools\AppX\x64\Release\Microsoft.UI.Xaml.2.7.appx')) { Expand-Archive 'Microsoft.UI.Xaml.zip' -ErrorAction Stop }

    Write-Information "[winget] Installing"
    Add-AppxPackage 'Microsoft.DesktopAppInstaller.msixbundle' -DependencyPath '.\Microsoft.UI.Xaml\tools\AppX\x64\Release\Microsoft.UI.Xaml.2.7.appx', '.\Microsoft.VCLibs.x64.14.Desktop.appx'  -ErrorAction Stop

}
catch {

    Write-Error $_

}
finally {

    Write-Information "[winget] Removing installer artifacts"
    Get-ChildItem 'Microsoft.DesktopAppInstaller.msixbundle', 'Microsoft.UI.Xaml.zip', 'Microsoft.VCLibs.x64.14.Desktop.appx', 'Microsoft.UI.Xaml' | Remove-Item -Force -Recurse

    Pop-Location

}


