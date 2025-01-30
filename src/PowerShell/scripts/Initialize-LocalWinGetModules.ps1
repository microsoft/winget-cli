# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
    .SYNOPSIS
        Helper script to setup the modules locally.
        - Copies the PowerShell modules output into this location.
        - Copies the modules files from the project because there's no guarantee they are updated in the module output
          location.
        - Adds the module location to PSModulePath if not there.
        - Import Microsoft.WinGet.* modules.
    
    .PARAMETER Platform
        The platform we are building for.
    
    .PARAMETER Configuration
        The configuration we are building in.
#>

[CmdletBinding()]
param (
    [Parameter(Mandatory)]
    [ValidateSet("Debug", "Release", "ReleaseStatic")]
    [string]
    $Configuration,

    [ValidateSet("Client", "DSC", "Configuration", "All")]
    [string]
    $ModuleType = "All",

    [string]
    $BuildRoot = "",

    [switch]
    $SkipImportModule,

    [switch]
    $Clean
)

class WinGetModule
{
    [string]$Name
    [string]$ModuleRoot
    [string]$Output
    [string[]]$ArchSpecificFiles = $null

    WinGetModule([string]$n, [string]$m, [string]$o)
    {
        $this.Name = $n
        $this.ModuleRoot = $m
        $this.Output = Join-Path $o $this.Name
        New-Item $this.Output -ItemType Directory -ErrorAction SilentlyContinue

        if (Get-Module -Name $this.Name)
        {
            Remove-Module $this.Name -Force
        }
    }

    [void]PrepareScriptFiles()
    {
        Write-Verbose "Copying script files: $($this.ModuleRoot) -> $($this.Output)"
        xcopy $this.ModuleRoot $this.Output /d /s /f /y
    }

    [void]PrepareBinaryFiles([string] $buildRoot, [string] $config)
    {
        Write-Verbose "Copying binary files: $buildRoot\AnyCpu\$config\PowerShell\$($this.Name)\* -> $($this.Output)"
        $copyErrors = $null
        Copy-Item "$buildRoot\AnyCpu\$config\PowerShell\$($this.Name)\*" $this.Output -Force -Recurse -ErrorVariable copyErrors -ErrorAction SilentlyContinue
        $copyErrors | ForEach-Object { Write-Warning $_ }
    }

    # Location is the path relative to the out module directory
    [void]AddArchSpecificFiles([string[]] $files, [string]$location, [string] $buildRoot, [string] $config)
    {
        $x64Path = "$($this.Output)\$location\x64\"
        $x86Path = "$($this.Output)\$location\x86\"
        $arm64Path = "$($this.Output)\$location\arm64\"
        if (-not (Test-Path $x64Path))
        {
            New-Item $x64Path -ItemType directory
        }

        if (-not (Test-Path $x86Path))
        {
            New-Item $x86Path -ItemType directory
        }

        if (-not (Test-Path $arm64Path))
        {
            New-Item $arm64Path -ItemType directory
        }        

        foreach ($f in $files)
        {
            $copyErrors = $null
            Copy-Item "$buildRoot\x64\$config\$f" "$($this.Output)\$location\x64\" -Force -ErrorVariable copyErrors -ErrorAction SilentlyContinue
            $copyErrors | ForEach-Object { Write-Warning $_ }
            Copy-Item "$buildRoot\x86\$config\$f" "$($this.Output)\$location\x86\" -Force -ErrorVariable copyErrors -ErrorAction SilentlyContinue
            $copyErrors | ForEach-Object { Write-Warning $_ }
            Copy-Item "$buildRoot\arm64\$config\$f" "$($this.Output)\$location\arm64\" -Force -ErrorVariable copyErrors -ErrorAction SilentlyContinue
            $copyErrors | ForEach-Object { Write-Warning $_ }            
        }
    }

    [void]AddAnyCpuSpecificFilesToArch([string[]] $files, [string]$location, [string] $buildRoot, [string] $config)
    {
        $x64Path = "$($this.Output)\$location\x64\"
        $x86Path = "$($this.Output)\$location\x86\"
        $arm64Path = "$($this.Output)\$location\arm64\"        
        if (-not (Test-Path $x64Path))
        {
            New-Item $x64Path -ItemType directory
        }

        if (-not (Test-Path $x86Path))
        {
            New-Item $x86Path -ItemType directory
        }

        if (-not (Test-Path $arm64Path))
        {
            New-Item $arm64Path -ItemType directory
        }

        foreach ($f in $files)
        {
            $copyErrors = $null
            Copy-Item "$buildRoot\AnyCpu\$config\$f" "$($this.Output)\$location\x64\" -Force -ErrorVariable copyErrors -ErrorAction SilentlyContinue
            $copyErrors | ForEach-Object { Write-Warning $_ }
            Copy-Item "$buildRoot\AnyCpu\$config\$f" "$($this.Output)\$location\x86\" -Force -ErrorVariable copyErrors -ErrorAction SilentlyContinue
            $copyErrors | ForEach-Object { Write-Warning $_ }
            Copy-Item "$buildRoot\AnyCpu\$config\$f" "$($this.Output)\$location\arm64\" -Force -ErrorVariable copyErrors -ErrorAction SilentlyContinue
            $copyErrors | ForEach-Object { Write-Warning $_ }
        }
    }
}

[Flags()] enum ModuleType
{
    None = 0
    Client = 1
    DSC = 2
    Configuration = 4
}

$local:moduleToConfigure = [ModuleType]::None
switch ($ModuleType)
{
    "Client"
    {
        $moduleToConfigure = [ModuleType]::Client;
        break
    }

    "DSC"
    {
        $moduleToConfigure = [ModuleType]::DSC;
        break
    }

    "Configuration"
    {
        $moduleToConfigure = [ModuleType]::Configuration;
        break
    }

    "All"
    {
        $moduleToConfigure = [ModuleType]::Client + [ModuleType]::DSC + [ModuleType]::Configuration;
        break
    } 
}

# I know it makes sense, but please don't do a clean up of $moduleRootOutput. When the modules are loaded
# there's no way to tell PowerShell to release the binary dlls that are loaded.
$local:moduleRootOutput = "$PSScriptRoot\Module\"

if ($BuildRoot -eq "")
{
    $BuildRoot = "$PSScriptRoot\..\..";
}

if ($Clean -and (Test-Path $moduleRootOutput))
{
    Remove-Item $moduleRootOutput -Recurse
}

# Modules, they should be in dependency order so that when importing we don't pick up the release modules.
$local:modules = @()
if ($moduleToConfigure.HasFlag([ModuleType]::Client))
{
    Write-Host "Setting up Microsoft.WinGet.Client"
    $module = [WinGetModule]::new("Microsoft.WinGet.Client", "$PSScriptRoot\..\Microsoft.WinGet.Client\ModuleFiles\", $moduleRootOutput)
    $module.PrepareBinaryFiles($BuildRoot, $Configuration)
    $module.PrepareScriptFiles()
    $additionalFiles = @(
        "Microsoft.Management.Deployment.InProc\Microsoft.Management.Deployment.dll"
        "Microsoft.Management.Deployment\Microsoft.Management.Deployment.winmd"
        "WindowsPackageManager\WindowsPackageManager.dll"
        "UndockedRegFreeWinRT\winrtact.dll"
    )
    $module.AddArchSpecificFiles($additionalFiles, "net8.0-windows10.0.22000.0\SharedDependencies", $BuildRoot, $Configuration)
    $module.AddArchSpecificFiles($additionalFiles, "net48\SharedDependencies", $BuildRoot, $Configuration)
    $modules += $module
}

if ($moduleToConfigure.HasFlag([ModuleType]::DSC))
{
    Write-Host "Setting up Microsoft.WinGet.DSC"
    $module = [WinGetModule]::new("Microsoft.WinGet.DSC", "$PSScriptRoot\..\Microsoft.WinGet.DSC\", $moduleRootOutput)
    $module.PrepareScriptFiles()
    $modules += $module
}

if ($moduleToConfigure.HasFlag([ModuleType]::Configuration))
{
    Write-Host "Setting up Microsoft.WinGet.Configuration"
    $module = [WinGetModule]::new("Microsoft.WinGet.Configuration", "$PSScriptRoot\..\Microsoft.WinGet.Configuration\ModuleFiles\", $moduleRootOutput)
    $module.PrepareBinaryFiles($BuildRoot, $Configuration)
    $module.PrepareScriptFiles()
    $additionalFiles = @(
        "Microsoft.Management.Configuration\Microsoft.Management.Configuration.dll"
    )
    $module.AddArchSpecificFiles($additionalFiles, "SharedDependencies", $BuildRoot, $Configuration)
    $additionalFiles = @(
        "Microsoft.Management.Configuration.Projection\net8.0-windows10.0.22000.0\Microsoft.Management.Configuration.Projection.dll"
    )
    $module.AddAnyCpuSpecificFilesToArch($additionalFiles, "SharedDependencies", $BuildRoot, $Configuration)
    $modules += $module
}

# Add it to module path if not there.
if (-not $env:PSModulePath.Contains($moduleRootOutput))
{
    Write-Host "Added $moduleRootOutput to PSModulePath" -ForegroundColor Green
    $env:PSModulePath += ";$moduleRootOutput"
}

# Now import modules.
if (-not $SkipImportModule)
{
    foreach($module in $modules)
    {
        Write-Host "Importing module $($module.Name)" -ForegroundColor Green
        Import-Module "$moduleRootOutput\$($module.Name)\" -Force
    }
}
