# Started by copying from:
# https://github.com/microsoft/winget-pkgs/blob/c393e50b66448cc25a5cd27aa754d37677f42ce2/Tools/SandboxTest.ps1

Param(
  [Parameter(Position = 0, HelpMessage = "The package identifiers to test.")]
  [String[]] $PackageIdentifiers,
  [Parameter(Position = 1, HelpMessage = "The source name that the package identifiers are from.")]
  [String] $Source
)

$ErrorActionPreference = "Stop"

# Check if Windows Sandbox is enabled

if (-Not (Get-Command 'WindowsSandbox' -ErrorAction SilentlyContinue)) {
  Write-Error -Category NotInstalled -Message @'
Windows Sandbox does not seem to be available. Check the following URL for prerequisites and further details:
https://docs.microsoft.com/windows/security/threat-protection/windows-sandbox/windows-sandbox-overview

You can run the following command in an elevated PowerShell for enabling Windows Sandbox:
$ Enable-WindowsOptionalFeature -Online -FeatureName 'Containers-DisposableClientVM'
'@
}

# Close Windows Sandbox

function Close-WindowsSandbox {
    $sandbox = Get-Process 'WindowsSandboxClient' -ErrorAction SilentlyContinue
    if ($sandbox) {
      Write-Host '--> Closing Windows Sandbox'

      $sandbox | Stop-Process
      $sandbox | Wait-Process -Timeout 30

      Write-Host
    }
    Remove-Variable sandbox
}

Close-WindowsSandbox

# Initialize Temp Folder

$tempFolderName = 'CorrelationTestStaging'
$tempFolder = Join-Path -Path ([System.IO.Path]::GetTempPath()) -ChildPath $tempFolderName

New-Item $tempFolder -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

# Set dependencies

$apiLatestUrl = 'https://api.github.com/repos/microsoft/winget-cli/releases/latest'

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$WebClient = New-Object System.Net.WebClient

function Get-LatestUrl {
  ((Invoke-WebRequest $apiLatestUrl -UseBasicParsing | ConvertFrom-Json).assets | Where-Object { $_.name -match '^Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle$' }).browser_download_url
}

function Get-LatestHash {
  $shaUrl = ((Invoke-WebRequest $apiLatestUrl -UseBasicParsing | ConvertFrom-Json).assets | Where-Object { $_.name -match '^Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.txt$' }).browser_download_url

  $shaFile = Join-Path -Path $tempFolder -ChildPath 'Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.txt'
  $WebClient.DownloadFile($shaUrl, $shaFile)

  Get-Content $shaFile
}

# Hide the progress bar of Invoke-WebRequest
$oldProgressPreference = $ProgressPreference
$ProgressPreference = 'SilentlyContinue'

$desktopAppInstaller = @{
  fileName = 'Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle'
  url      = $(Get-LatestUrl)
  hash     = $(Get-LatestHash)
}

$ProgressPreference = $oldProgressPreference

$vcLibsUwp = @{
  fileName = 'Microsoft.VCLibs.x64.14.00.Desktop.appx'
  url      = 'https://aka.ms/Microsoft.VCLibs.x64.14.00.Desktop.appx'
  hash     = 'A39CEC0E70BE9E3E48801B871C034872F1D7E5E8EEBE986198C019CF2C271040'
}
$uiLibsUwp = @{
    fileName = 'Microsoft.UI.Xaml.2.7.zip'
    url = 'https://www.nuget.org/api/v2/package/Microsoft.UI.Xaml/2.7.0'
    hash = "422FD24B231E87A842C4DAEABC6A335112E0D35B86FAC91F5CE7CF327E36A591"
}

$dependencies = @($desktopAppInstaller, $vcLibsUwp, $uiLibsUwp)

# Clean temp directory

Get-ChildItem $tempFolder -Recurse -Exclude $dependencies.fileName | Remove-Item -Force -Recurse

if (-Not [String]::IsNullOrWhiteSpace($Manifest)) {
  Copy-Item -Path $Manifest -Recurse -Destination $tempFolder
}

# Download dependencies

Write-Host '--> Checking dependencies'

$desktopInSandbox = 'C:\Users\WDAGUtilityAccount\Desktop'

foreach ($dependency in $dependencies) {
  $dependency.file = Join-Path -Path $tempFolder -ChildPath $dependency.fileName
  $dependency.pathInSandbox = Join-Path -Path $desktopInSandbox -ChildPath (Join-Path -Path $tempFolderName -ChildPath $dependency.fileName)

  # Only download if the file does not exist, or its hash does not match.
  if (-Not ((Test-Path -Path $dependency.file -PathType Leaf) -And $dependency.hash -eq $(Get-FileHash $dependency.file).Hash)) {
    Write-Host @"
    - Downloading:
      $($dependency.url)
"@

    try {
      $WebClient.DownloadFile($dependency.url, $dependency.file)
    }
    catch {
      #Pass the exception as an inner exception
      throw [System.Net.WebException]::new("Error downloading $($dependency.url).",$_.Exception)
    }
    if (-not ($dependency.hash -eq $(Get-FileHash $dependency.file).Hash)) {
      throw [System.Activities.VersionMismatchException]::new('Dependency hash does not match the downloaded file')
    }
  }
}

# Extract Microsoft.UI.Xaml from zip (if freshly downloaded).
# This is a workaround until https://github.com/microsoft/winget-cli/issues/1861 is resolved.

if (-Not (Test-Path (Join-Path -Path $tempFolder -ChildPath \Microsoft.UI.Xaml.2.7\tools\AppX\x64\Release\Microsoft.UI.Xaml.2.7.appx))){
  Expand-Archive -Path $uiLibsUwp.file -DestinationPath ($tempFolder + "\Microsoft.UI.Xaml.2.7") -Force
}  
$uiLibsUwp.file = (Join-Path -Path $tempFolder -ChildPath \Microsoft.UI.Xaml.2.7\tools\AppX\x64\Release\Microsoft.UI.Xaml.2.7.appx)
$uiLibsUwp.pathInSandbox = Join-Path -Path $desktopInSandbox -ChildPath (Join-Path -Path $tempFolderName -ChildPath \Microsoft.UI.Xaml.2.7\tools\AppX\x64\Release\Microsoft.UI.Xaml.2.7.appx)
Write-Host

# Copy main script

$mainPs1FileName = 'InSandboxScript.ps1'
Copy-Item (Join-Path $PSScriptRoot $mainPs1FileName) (Join-Path $tempFolder $mainPs1FileName)

foreach ($packageIdentifier in $PackageIdentifiers)
{

    # Create temporary location for output
    $outPath = Join-Path ([System.IO.Path]::GetTempPath()) (New-Guid)
    New-Item -ItemType Directory $outPath > $nul

    $outPathInSandbox = Join-Path -Path $desktopInSandbox -ChildPath (Split-Path -Path $outPath -Leaf)

    $bootstrapPs1Content = @"
.\$mainPs1FileName -DesktopAppInstallerPath '$($desktopAppInstaller.pathInSandbox)' -DesktopAppInstallerDependencyPath @('$($vcLibsUwp.pathInSandbox)', '$($uiLibsUwp.pathInSandbox)') -PackageIdentifier '$packageIdentifier' -SourceName '$Source'-OutputPath '$outPathInSandbox'
"@

    $bootstrapPs1FileName = 'Bootstrap.ps1'
    $bootstrapPs1Content | Out-File (Join-Path $tempFolder $bootstrapPs1FileName) -Force

    # Create Wsb file

    $bootstrapPs1InSandbox = Join-Path -Path $desktopInSandbox -ChildPath (Join-Path -Path $tempFolderName -ChildPath $bootstrapPs1FileName)
    $tempFolderInSandbox = Join-Path -Path $desktopInSandbox -ChildPath $tempFolderName

    $sandboxTestWsbContent = @"
<Configuration>
    <MappedFolders>
    <MappedFolder>
        <HostFolder>$tempFolder</HostFolder>
        <ReadOnly>true</ReadOnly>
    </MappedFolder>
    <MappedFolder>
        <HostFolder>$outPath</HostFolder>
    </MappedFolder>
    </MappedFolders>
    <LogonCommand>
    <Command>PowerShell Start-Process PowerShell -WindowStyle Maximized -WorkingDirectory '$tempFolderInSandbox' -ArgumentList '-ExecutionPolicy Bypass -NoExit -NoLogo -File $bootstrapPs1InSandbox'</Command>
    </LogonCommand>
</Configuration>
"@

    $sandboxTestWsbFileName = 'SandboxTest.wsb'
    $sandboxTestWsbFile = Join-Path -Path $tempFolder -ChildPath $sandboxTestWsbFileName
    $sandboxTestWsbContent | Out-File $sandboxTestWsbFile -Force

    Write-Host @"
--> Starting Windows Sandbox, and:
    - Mounting the following directories:
        - $tempFolder as read-only
        - $outPath as read-write
    - Installing WinGet
    - Installing the package: $packageIdentifier
    - Comparing ARP Entries
"@

    Write-Host

    WindowsSandbox $SandboxTestWsbFile

    $outputFileBlockerPath = Join-Path $outPath "test-out.txt"

    while (-not (Test-Path $outputFileBlockerPath))
    {
        Start-Sleep 1
    }

    Close-WindowsSandbox
}
