# Started by copying from:
# https://github.com/microsoft/winget-pkgs/blob/c393e50b66448cc25a5cd27aa754d37677f42ce2/Tools/SandboxTest.ps1

Param(
  [Parameter(Position = 0, HelpMessage = "The package identifiers to test.")]
  [String[]] $PackageIdentifiers,
  [Parameter(Position = 1, HelpMessage = "The source name that the package identifiers are from.")]
  [String] $Source,
  [Parameter(HelpMessage = "The directory where the correlation program is located.")]
  [String] $ExePath,
  [Parameter(HelpMessage = "Indicates that the local dev build should be used rather than the published package.")]
  [Switch] $UseDev,
  [Parameter(HelpMessage = "The directory where local dev build is located; only the release build works.")]
  [String] $DevPackagePath,
  [Parameter(HelpMessage = "The results output path.")]
  [String] $ResultsPath,
  [Parameter(HelpMessage = "The path to registry files that should be injected before the test.")]
  [String] $RegFileDirectory
)

$ErrorActionPreference = "Stop"

# Validate that the ExePath points to a reasonable location

if (-not $ExePath)
{
  $ExePath = Join-Path $PSScriptRoot "InstallAndCheckCorrelation\x64\Release"
}

$ExePath = [System.IO.Path]::GetFullPath($ExePath)

if (-not (Test-Path (Join-Path $ExePath "InstallAndCheckCorrelation.exe")))
{
  Write-Error -Category InvalidArgument -Message @"
InstallAndCheckCorrelation.exe does not exist in the path $ExePath
Either build it, or provide the location using -ExePath
"@
}

# Validate that the local dev manifest exists

if ($UseDev)
{
  if (-not $DevPackagePath)
  {
    $DevPackagePath = Join-Path $PSScriptRoot "..\..\src\AppInstallerCLIPackage\bin\x64\Release\AppX"
  }

  $DevPackagePath = [System.IO.Path]::GetFullPath($DevPackagePath)

  if ($DevPackagePath.ToLower().Contains("debug"))
  {
    Write-Error -Category InvalidArgument -Message @"
The Debug dev package does not work for unknown reasons.
Use the Release build or figure out how to make debug work and fix the scripts.
"@
  }

  if (-not (Test-Path (Join-Path $DevPackagePath "AppxManifest.xml")))
  {
    Write-Error -Category InvalidArgument -Message @"
AppxManifest.xml does not exist in the path $DevPackagePath
Either build the local dev package, or provide the location using -DevPackagePath
"@
  }
}

# Check if Windows Sandbox is enabled

if (-Not (Get-Command 'WindowsSandbox' -ErrorAction SilentlyContinue))
{
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
    if ($sandbox)
    {
      Write-Host '--> Closing Windows Sandbox'

      $sandboxServer = Get-Process 'WindowsSandbox' -ErrorAction SilentlyContinue

      $sandbox | Stop-Process
      $sandbox | Wait-Process -Timeout 30

      # Also wait for the server to close
      if ($sandboxServer)
      {
        $sandboxServer | Wait-Process -Timeout 30
      }

      Write-Host
    }
    Remove-Variable sandbox
}

Close-WindowsSandbox

# Create output location for results

if (-not $ResultsPath)
{
  $ResultsPath = Join-Path ([System.IO.Path]::GetTempPath()) (New-Guid)
}

$ResultsPath = [System.IO.Path]::GetFullPath($ResultsPath)

if (Test-Path $ResultsPath)
{
  Remove-Item -Recurse $ResultsPath -Force
}

New-Item -ItemType Directory $ResultsPath  | Out-Null

# Initialize Temp Folder

$tempFolderName = 'CorrelationTestStaging'
$tempFolder = Join-Path -Path ([System.IO.Path]::GetTempPath()) -ChildPath $tempFolderName

New-Item $tempFolder -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

# Set dependencies

$desktopInSandbox = 'C:\Users\WDAGUtilityAccount\Desktop'

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
  folderInLocal = Join-Path ${env:ProgramFiles(x86)} "Microsoft SDKs\Windows Kits\10\ExtensionSDKs\Microsoft.VCLibs.Desktop\14.0\Appx\Retail\x64"
}
$uiLibsUwp = @{
  fileName = 'Microsoft.UI.Xaml.2.7.zip'
  url = 'https://www.nuget.org/api/v2/package/Microsoft.UI.Xaml/2.7.0'
  hash = "422FD24B231E87A842C4DAEABC6A335112E0D35B86FAC91F5CE7CF327E36A591"
}

if ($UseDev)
{
  $dependencies = @($vcLibsUwp)
}
else
{
  $dependencies = @($desktopAppInstaller, $vcLibsUwp, $uiLibsUwp)
}

# Clean temp directory

Get-ChildItem $tempFolder -Recurse -Exclude $dependencies.fileName | Remove-Item -Force -Recurse

# Download dependencies

Write-Host '--> Checking dependencies'

foreach ($dependency in $dependencies)
{
  $dependency.pathInSandbox = Join-Path -Path $desktopInSandbox -ChildPath (Join-Path -Path $tempFolderName -ChildPath $dependency.fileName)

  # First see if the file exists locally to copy instead of downloading
  if ($dependency.folderInLocal -ne $null)
  {
    $dependencyFilePath = Join-Path -Path $dependency.folderInLocal -ChildPath $dependency.fileName
    if (Test-Path -Path $dependencyFilePath -PathType Leaf)
    {
      $dependencyFilePath
      $tempFolder
      Copy-Item -Path $dependencyFilePath -Destination $tempFolder -Force
      continue
    }
  }

  # File does not exist locally, we need to download

  $dependency.file = Join-Path -Path $tempFolder -ChildPath $dependency.fileName

  # Only download if the file does not exist, or its hash does not match.
  if (-Not ((Test-Path -Path $dependency.file -PathType Leaf) -And $dependency.hash -eq $(Get-FileHash $dependency.file).Hash))
  {
    Write-Host @"
  - Downloading:
    $($dependency.url)
"@

    try
    {
      $WebClient.DownloadFile($dependency.url, $dependency.file)
    }
    catch
    {
      #Pass the exception as an inner exception
      throw [System.Net.WebException]::new("Error downloading $($dependency.url).",$_.Exception)
    }
    if (-not ($dependency.hash -eq $(Get-FileHash $dependency.file).Hash))
    {
      throw [System.Activities.VersionMismatchException]::new('Dependency hash does not match the downloaded file')
    }
  }
}

if (-not $UseDev)
{
  # Extract Microsoft.UI.Xaml from zip (if freshly downloaded).
  # This is a workaround until https://github.com/microsoft/winget-cli/issues/1861 is resolved.

  if (-Not (Test-Path (Join-Path -Path $tampFolder -ChildPath \Microsoft.UI.Xaml.2.7\tools\AppX\x64\Release\Microsoft.UI.Xaml.2.7.appx)))
  {
    Expand-Archive -Path $uiLibsUwp.file -DestinationPath ($tempFolder + "\Microsoft.UI.Xaml.2.7") -Force
  }
  $uiLibsUwp.file = (Join-Path -Path $tempFolder -ChildPath \Microsoft.UI.Xaml.2.7\tools\AppX\x64\Release\Microsoft.UI.Xaml.2.7.appx)
  $uiLibsUwp.pathInSandbox = Join-Path -Path $desktopInSandbox -ChildPath (Join-Path -Path $tempFolderName -ChildPath \Microsoft.UI.Xaml.2.7\tools\AppX\x64\Release\Microsoft.UI.Xaml.2.7.appx)
  Write-Host
}

# Copy main script

$mainPs1FileName = 'InSandboxScript.ps1'
Copy-Item (Join-Path $PSScriptRoot $mainPs1FileName) (Join-Path $tempFolder $mainPs1FileName)

foreach ($packageIdentifier in $PackageIdentifiers)
{

    # Create temporary location for output
    $outPath = Join-Path $ResultsPath $packageIdentifier
    New-Item -ItemType Directory $outPath  | Out-Null

    $outPathInSandbox = Join-Path -Path $desktopInSandbox -ChildPath (Split-Path -Path $outPath -Leaf)

    if ($UseDev)
    {
      $dependenciesPathsInSandbox = "@('$($vcLibsUwp.pathInSandbox)')"
    }
    else
    {
      $dependenciesPathsInSandbox = "@('$($vcLibsUwp.pathInSandbox)', '$($uiLibsUwp.pathInSandbox)')"
    }

    $bootstrapPs1Content = ".\$mainPs1FileName -DesktopAppInstallerDependencyPath @($dependenciesPathsInSandbox) -PackageIdentifier '$packageIdentifier' -SourceName '$Source' -OutputPath '$outPathInSandbox'"

    if ($UseDev)
    {
      $bootstrapPs1Content += " -UseDev"
    }
    else
    {
      $bootstrapPs1Content += " -DesktopAppInstallerPath '$($desktopAppInstaller.pathInSandbox)'"
    }

    $bootstrapPs1FileName = 'Bootstrap.ps1'
    $bootstrapPs1Content | Out-File (Join-Path $tempFolder $bootstrapPs1FileName) -Force

    # Create Windows Sandbox configuration file

    $bootstrapPs1InSandbox = Join-Path -Path $desktopInSandbox -ChildPath (Join-Path -Path $tempFolderName -ChildPath $bootstrapPs1FileName)
    $tempFolderInSandbox = Join-Path -Path $desktopInSandbox -ChildPath $tempFolderName
    $exePathInSandbox = Join-Path -Path $desktopInSandbox -ChildPath "InstallAndCheckCorrelation"

    $devPackageInSandbox = Join-Path -Path $desktopInSandbox -ChildPath "DevPackage"
    $devPackageXMLFragment = ""

    if ($UseDev)
    {
      $devPackageXMLFragment = @"
      <MappedFolder>
          <HostFolder>$DevPackagePath</HostFolder>
          <SandboxFolder>$devPackageInSandbox</SandboxFolder>
      </MappedFolder>
"@
    }

    $regFileDirInSandbox = Join-Path -Path $desktopInSandbox -ChildPath "RegFiles"
    $regFileDirXMLFragment = ""

    if ($RegFileDirectory)
    {
      $regFileDirXMLFragment = @"
      <MappedFolder>
          <HostFolder>$RegFileDirectory</HostFolder>
          <SandboxFolder>$regFileDirInSandbox</SandboxFolder>
          <ReadOnly>true</ReadOnly>
      </MappedFolder>
"@
    }

    $sandboxTestWsbContent = @"
<Configuration>
    <MappedFolders>
    <MappedFolder>
        <HostFolder>$tempFolder</HostFolder>
        <ReadOnly>true</ReadOnly>
    </MappedFolder>
    <MappedFolder>
        <HostFolder>$ExePath</HostFolder>
        <SandboxFolder>$exePathInSandbox</SandboxFolder>
        <ReadOnly>true</ReadOnly>
    </MappedFolder>
    $devPackageXMLFragment
    $regFileDirXMLFragment
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
--> Starting Windows Sandbox:
    - Package: $packageIdentifier
    - Output directory: $outPath
"@

    Write-Host

    WindowsSandbox $SandboxTestWsbFile

    $outputFileBlockerPath = Join-Path $outPath "done.txt"

    # The correlation program should time out on its own after 10m.
    $waitTimeout = [System.TimeSpan]::new(0, 15, 0)
    $startWaitTime = Get-Date

    while (-not (Test-Path $outputFileBlockerPath))
    {
        $elapsedTime = (Get-Date) - $startWaitTime
        if ($elapsedTime -gt $waitTimeout)
        {
          break
        }
        Start-Sleep 1
    }

    Close-WindowsSandbox
}

Write-Host @"
--> Results are located at $ResultsPath
"@