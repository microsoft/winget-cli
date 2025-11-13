#Requires -Version 5.1

<#
.SYNOPSIS
    Recursively searches for .vcxproj files and extracts NuGet package versions from associated packages.config files.

.DESCRIPTION
    This script searches directories below the script location for Visual Studio C++ project files (.vcxproj)
    and their corresponding packages.config files. It extracts all NuGet package references with their versions
    and generates a comprehensive report.

.PARAMETER OutputFormat
    Specifies the output format: Table, CSV, JSON, or Object. Default is Table. Object format returns PackageInfo objects without printing anything.

.PARAMETER ExportPath
    Optional path to export the results to a file. Not applicable for Object format.

.PARAMETER PackageFilter
    Optional filter to search for specific package names. Supports wildcards. If specified, only packages matching this filter will be included in the results.

.EXAMPLE
    .\Get-VcxprojNugetPackageVersions.ps1
    
.EXAMPLE
    .\Get-VcxprojNugetPackageVersions.ps1 -OutputFormat CSV -ExportPath "packages-report.csv"

.EXAMPLE
    .\Get-VcxprojNugetPackageVersions.ps1 -PackageFilter "Microsoft*"

.EXAMPLE
    .\Get-VcxprojNugetPackageVersions.ps1 -PackageFilter "Newtonsoft.Json" -OutputFormat JSON

.EXAMPLE
    $packages = .\Get-VcxprojNugetPackageVersions.ps1 -OutputFormat Object
#>

[CmdletBinding()]
param(
    [ValidateSet("Table", "CSV", "JSON", "Object")]
    [string]$OutputFormat = "Table",
    
    [string]$ExportPath,
    
    [string]$PackageFilter
)

# Custom class to hold package information
class PackageInfo {
    [string]$ProjectFile
    [string]$ProjectName
    [string]$PackageId
    [string]$Version
    [string]$TargetFramework
    [string]$PackagesConfigPath
    [bool]$PackagesConfigExists
}

function Get-VcxprojFiles {
    param(
        [string]$SearchPath,
        [bool]$SuppressOutput = $false
    )
    
    Write-Verbose "Searching for .vcxproj files in: $SearchPath"
    
    try {
        $vcxprojFiles = Get-ChildItem -Path $SearchPath -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue
        if (-not $SuppressOutput) {
            Write-Host "Found $($vcxprojFiles.Count) .vcxproj files" -ForegroundColor Green
        }
        return $vcxprojFiles
    }
    catch {
        Write-Warning "Error searching for .vcxproj files: $($_.Exception.Message)"
        return @()
    }
}

function Get-PackagesFromConfig {
    param(
        [string]$PackagesConfigPath,
        [string]$ProjectFile,
        [string]$ProjectName,
        [string]$PackageFilter
    )
    
    $packages = @()
    
    if (-not (Test-Path $PackagesConfigPath)) {
        Write-Verbose "No packages.config found for project: $ProjectName"
        
        # If a package filter is specified and there's no packages.config, don't include this project
        if ($PackageFilter) {
            Write-Verbose "Package filter specified but no packages.config exists for project: $ProjectName. Excluding from results."
            return @()
        }
        
        # Return empty package info to indicate no packages.config
        $packageInfo = [PackageInfo]::new()
        $packageInfo.ProjectFile = $ProjectFile
        $packageInfo.ProjectName = $ProjectName
        $packageInfo.PackageId = "N/A"
        $packageInfo.Version = "N/A"
        $packageInfo.TargetFramework = "N/A"
        $packageInfo.PackagesConfigPath = $PackagesConfigPath
        $packageInfo.PackagesConfigExists = $false
        return @($packageInfo)
    }
    
    try {
        Write-Verbose "Processing packages.config: $PackagesConfigPath"
        [xml]$packagesXml = Get-Content $PackagesConfigPath -ErrorAction Stop
        
        $packageNodes = $packagesXml.packages.package
        
        if ($null -eq $packageNodes) {
            Write-Verbose "No package nodes found in packages.config for project: $ProjectName"
            return @()
        }
        
        # Handle both single package and multiple packages scenarios
        if ($packageNodes -is [System.Xml.XmlElement]) {
            # Single package
            $packageNodes = @($packageNodes)
        }
        
        foreach ($package in $packageNodes) {
            # Apply package filter if specified
            if ($PackageFilter -and $package.id -notlike $PackageFilter) {
                Write-Verbose "Package '$($package.id)' does not match filter '$PackageFilter', skipping"
                continue
            }
            
            $packageInfo = [PackageInfo]::new()
            $packageInfo.ProjectFile = $ProjectFile
            $packageInfo.ProjectName = $ProjectName
            $packageInfo.PackageId = $package.id
            $packageInfo.Version = $package.version
            $packageInfo.TargetFramework = $package.targetFramework
            $packageInfo.PackagesConfigPath = $PackagesConfigPath
            $packageInfo.PackagesConfigExists = $true
            
            $packages += $packageInfo
        }
        
        Write-Verbose "Found $($packages.Count) packages in $ProjectName"
    }
    catch {
        Write-Warning "Error parsing packages.config for project '$ProjectName': $($_.Exception.Message)"
    }
    
    return $packages
}

function Get-ProjectName {
    param([string]$VcxprojPath)
    
    try {
        [xml]$projectXml = Get-Content $VcxprojPath -ErrorAction Stop
        
        # Try to get project name from PropertyGroup
        $projectNameNode = $projectXml.Project.PropertyGroup.ProjectName | Select-Object -First 1
        if ($projectNameNode) {
            return $projectNameNode
        }
        
        # Fallback to filename without extension
        return [System.IO.Path]::GetFileNameWithoutExtension($VcxprojPath)
    }
    catch {
        Write-Verbose "Could not parse project file for name, using filename: $($_.Exception.Message)"
        return [System.IO.Path]::GetFileNameWithoutExtension($VcxprojPath)
    }
}

function Export-Results {
    param(
        [array]$Results,
        [string]$Format,
        [string]$Path
    )
    
    if (-not $Path) {
        return
    }
    
    try {
        switch ($Format) {
            "CSV" {
                $Results | Export-Csv -Path $Path -NoTypeInformation
                Write-Host "Results exported to CSV: $Path" -ForegroundColor Green
            }
            "JSON" {
                $Results | ConvertTo-Json -Depth 3 | Out-File -FilePath $Path -Encoding utf8
                Write-Host "Results exported to JSON: $Path" -ForegroundColor Green
            }
            default {
                $Results | Format-Table -AutoSize | Out-File -FilePath $Path -Encoding utf8
                Write-Host "Results exported to text file: $Path" -ForegroundColor Green
            }
        }
    }
    catch {
        Write-Warning "Failed to export results to '$Path': $($_.Exception.Message)"
    }
}

function Show-Summary {
    param([array]$AllPackages)
    
    $projectsWithPackages = $AllPackages | Where-Object { $_.PackagesConfigExists } | Group-Object ProjectFile
    $projectsWithoutPackages = $AllPackages | Where-Object { -not $_.PackagesConfigExists } | Group-Object ProjectFile
    
    Write-Host "`n=== SUMMARY ===" -ForegroundColor Cyan
    Write-Host "Total .vcxproj files found: $($projectsWithPackages.Count + $projectsWithoutPackages.Count)"
    Write-Host "Projects with packages.config: $($projectsWithPackages.Count)" -ForegroundColor Green
    Write-Host "Projects without packages.config: $($projectsWithoutPackages.Count)" -ForegroundColor Yellow
    Write-Host "Total package references: $(($AllPackages | Where-Object { $_.PackagesConfigExists }).Count)" -ForegroundColor Green
}

# Main execution
try {
    $scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
    
    # Suppress output messages for Object format
    if ($OutputFormat -ne "Object") {
        Write-Host "Starting search from: $scriptPath" -ForegroundColor Cyan
        Write-Host "Output format: $OutputFormat" -ForegroundColor Cyan
        
        if ($PackageFilter) {
            Write-Host "Package filter: $PackageFilter" -ForegroundColor Cyan
        }
        
        if ($ExportPath) {
            Write-Host "Export path: $ExportPath" -ForegroundColor Cyan
        }
    }
    
    # Find all .vcxproj files
    $vcxprojFiles = Get-VcxprojFiles -SearchPath $scriptPath -SuppressOutput ($OutputFormat -eq "Object")
    
    if ($vcxprojFiles.Count -eq 0) {
        if ($OutputFormat -ne "Object") {
            Write-Warning "No .vcxproj files found in the search directory."
        }
        return @()  # Return empty array for Object format
    }
    
    # Process each .vcxproj file
    $allPackages = @()
    $processedCount = 0
    
    foreach ($vcxproj in $vcxprojFiles) {
        $processedCount++
        Write-Progress -Activity "Processing .vcxproj files" -Status "$processedCount of $($vcxprojFiles.Count)" -PercentComplete (($processedCount / $vcxprojFiles.Count) * 100)
        
        $projectName = Get-ProjectName -VcxprojPath $vcxproj.FullName
        $packagesConfigPath = Join-Path $vcxproj.DirectoryName "packages.config"
        
        $packages = Get-PackagesFromConfig -PackagesConfigPath $packagesConfigPath -ProjectFile $vcxproj.FullName -ProjectName $projectName -PackageFilter $PackageFilter
        $allPackages += $packages
    }
    
    Write-Progress -Activity "Processing .vcxproj files" -Completed
    
    # Display results
    if ($allPackages.Count -gt 0) {
        switch ($OutputFormat) {
            "CSV" {
                if ($ExportPath) {
                    Export-Results -Results $allPackages -Format "CSV" -Path $ExportPath
                } else {
                    $allPackages | ConvertTo-Csv -NoTypeInformation | Write-Output
                }
            }
            "JSON" {
                if ($ExportPath) {
                    Export-Results -Results $allPackages -Format "JSON" -Path $ExportPath
                } else {
                    $allPackages | ConvertTo-Json -Depth 3 | Write-Output
                }
            }
            "Object" {
                # Return only packages where PackagesConfigExists is true
                $packagesWithConfig = $allPackages | Where-Object { $_.PackagesConfigExists }
                return $packagesWithConfig
            }
            default {
                # Table format
                Write-Host "`n=== PACKAGE DETAILS ===" -ForegroundColor Cyan

                # Show the set of packages and versions found across all projects
                $uniquePackages = $allPackages | Where-Object { $_.PackagesConfigExists } | Group-Object PackageId, Version
                if ($uniquePackages.Count -gt 0) {
                    $uniquePackages | Format-Table -Property Name, Count -AutoSize
                }

                Write-Host "`n=== PROJECT DETAILS ===" -ForegroundColor Cyan
                
                # Show projects with packages
                $packagesWithConfig = $allPackages | Where-Object { $_.PackagesConfigExists }
                if ($packagesWithConfig.Count -gt 0) {
                    $packagesWithConfig | Format-Table -Property ProjectName, PackageId, Version, TargetFramework -AutoSize
                }
                
                if ($ExportPath) {
                    Export-Results -Results $allPackages -Format $OutputFormat -Path $ExportPath
                }
            }
        }
        
        # Show summary (but not for Object format)
        if ($OutputFormat -ne "Object") {
            Show-Summary -AllPackages $allPackages
        }
    } else {
        if ($OutputFormat -ne "Object") {
            Write-Host "No packages found in any .vcxproj files." -ForegroundColor Yellow
        }
    }
}
catch {
    Write-Error "An error occurred during execution: $($_.Exception.Message)"
    Write-Error $_.ScriptStackTrace
}

if ($OutputFormat -ne "Object") {
    Write-Host "`nScript completed." -ForegroundColor Green
}