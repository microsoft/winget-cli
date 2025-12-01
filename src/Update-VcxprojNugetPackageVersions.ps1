#Requires -Version 5.1

<#
.SYNOPSIS
    Updates the version of a specific NuGet package across all .vcxproj files and their associated packages.config files.

.DESCRIPTION
    This script searches for all projects that use a specified NuGet package and updates both the packages.config
    file and the .vcxproj file to use a new version. It uses Get-VcxprojNugetPackageVersions.ps1 to find target
    projects and then performs the necessary updates.

.PARAMETER PackageName
    The name of the NuGet package to update (e.g., "Newtonsoft.Json").

.PARAMETER NewVersion
    The new version to update to (e.g., "13.0.3").

.PARAMETER WhatIf
    Shows what changes would be made without actually making them.

.PARAMETER Backup
    Creates backup files (.bak) before making changes.

.EXAMPLE
    .\Update-VcxprojNugetPackageVersions.ps1 -PackageName "Newtonsoft.Json" -NewVersion "13.0.3"
    
.EXAMPLE
    .\Update-VcxprojNugetPackageVersions.ps1 -PackageName "Microsoft.VisualStudio.TestTools.UnitTesting" -NewVersion "2.2.8" -WhatIf

.EXAMPLE
    .\Update-VcxprojNugetPackageVersions.ps1 -PackageName "boost" -NewVersion "1.82.0" -Backup:$false
#>

[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory = $true)]
    [string]$PackageName,
    
    [Parameter(Mandatory = $true)]
    [string]$NewVersion,
    
    [switch]$Backup
)

# Import the Get-VcxprojNugetPackageVersions script
$getPackagesScript = Join-Path $PSScriptRoot "Get-VcxprojNugetPackageVersions.ps1"

if (-not (Test-Path $getPackagesScript)) {
    Write-Error "Could not find Get-VcxprojNugetPackageVersions.ps1 in the same directory as this script."
    exit 1
}

function Update-PackagesConfig {
    param(
        [string]$PackagesConfigPath,
        [string]$PackageName,
        [string]$OldVersion,
        [string]$NewVersion,
        [string]$TargetFramework,
        [bool]$CreateBackup = $true,
        [bool]$WhatIfMode = $false
    )
    
    if (-not (Test-Path $PackagesConfigPath)) {
        Write-Warning "packages.config not found: $PackagesConfigPath"
        return $false
    }
    
    try {
        # Create backup if requested
        if ($CreateBackup -and -not $WhatIfMode) {
            $backupPath = "$PackagesConfigPath.bak"
            Copy-Item $PackagesConfigPath $backupPath -Force
            Write-Verbose "Created backup: $backupPath"
        }
        
        # Load and parse the XML
        [xml]$packagesXml = Get-Content $PackagesConfigPath -ErrorAction Stop
        
        $packageNode = $packagesXml.packages.package | Where-Object { $_.id -eq $PackageName }
        
        if (-not $packageNode) {
            Write-Warning "Package '$PackageName' not found in $PackagesConfigPath"
            return $false
        }
        
        $currentVersion = $packageNode.version
        
        if ($WhatIfMode) {
            Write-Host "WHATIF: Would update $PackagesConfigPath" -ForegroundColor Yellow
            Write-Host "  Package: $PackageName" -ForegroundColor Cyan
            Write-Host "  Current Version: $currentVersion" -ForegroundColor Red
            Write-Host "  New Version: $NewVersion" -ForegroundColor Green
            return $true
        }
        
        # Update the version
        $packageNode.version = $NewVersion
        
        # Save the updated XML
        $packagesXml.Save($PackagesConfigPath)
        
        Write-Host "Updated packages.config: $PackagesConfigPath" -ForegroundColor Green
        Write-Host "  ${PackageName}: $currentVersion → $NewVersion" -ForegroundColor Cyan
        
        return $true
    }
    catch {
        Write-Error "Failed to update packages.config '$PackagesConfigPath': $($_.Exception.Message)"
        return $false
    }
}

function Update-VcxprojFile {
    param(
        [string]$VcxprojPath,
        [string]$PackageName,
        [string]$OldVersion,
        [string]$NewVersion,
        [bool]$CreateBackup = $true,
        [bool]$WhatIfMode = $false
    )
    
    if (-not (Test-Path $VcxprojPath)) {
        Write-Warning ".vcxproj file not found: $VcxprojPath"
        return $false
    }
    
    try {
        # Read the file content
        $content = Get-Content $VcxprojPath -Raw -ErrorAction Stop
        
        # Pattern to match package references in the format "PACKAGE-NAME.VERSION"
        $oldPattern = [regex]::Escape("$PackageName.$OldVersion")
        $newPattern = "$PackageName.$NewVersion"
        
        # Check if there are any matches
        if ($content -notmatch $oldPattern) {
            Write-Verbose "No references to $PackageName.$OldVersion found in $VcxprojPath"
            return $false
        }
        
        if ($WhatIfMode) {
            $regexMatches = [regex]::Matches($content, $oldPattern)
            Write-Host "WHATIF: Would update $VcxprojPath" -ForegroundColor Yellow
            Write-Host "  Found $($regexMatches.Count) reference(s) to update:" -ForegroundColor Cyan
            Write-Host "  $oldPattern → $newPattern" -ForegroundColor Cyan
            return $true
        }
        
        # Create backup if requested
        if ($CreateBackup) {
            $backupPath = "$VcxprojPath.bak"
            Copy-Item $VcxprojPath $backupPath -Force
            Write-Verbose "Created backup: $backupPath"
        }
        
        # Replace all occurrences
        $updatedContent = $content -replace $oldPattern, $newPattern
        
        # Count the number of replacements made
        $originalMatches = [regex]::Matches($content, $oldPattern).Count
        $remainingMatches = [regex]::Matches($updatedContent, $oldPattern).Count
        $replacementCount = $originalMatches - $remainingMatches
        
        # Save the updated content
        Set-Content $VcxprojPath -Value $updatedContent -Encoding UTF8 -ErrorAction Stop
        
        Write-Host "Updated .vcxproj file: $VcxprojPath" -ForegroundColor Green
        Write-Host "  Replaced $replacementCount reference(s): $oldPattern → $newPattern" -ForegroundColor Cyan
        
        return $true
    }
    catch {
        Write-Error "Failed to update .vcxproj file '$VcxprojPath': $($_.Exception.Message)"
        return $false
    }
}

function Show-UpdateSummary {
    param(
        [array]$TargetPackages,
        [int]$SuccessfulPackagesConfig,
        [int]$SuccessfulVcxproj,
        [string]$PackageName,
        [string]$NewVersion
    )
    
    Write-Host "`n=== UPDATE SUMMARY ===" -ForegroundColor Cyan
    Write-Host "Package: $PackageName" -ForegroundColor White
    Write-Host "New Version: $NewVersion" -ForegroundColor White
    Write-Host "Projects found with package: $($TargetPackages.Count)" -ForegroundColor White
    Write-Host "packages.config files updated: $SuccessfulPackagesConfig" -ForegroundColor Green
    Write-Host ".vcxproj files updated: $SuccessfulVcxproj" -ForegroundColor Green
    
    # Show unique versions being replaced
    $uniqueVersions = $TargetPackages | Group-Object Version | Sort-Object Name
    if ($uniqueVersions.Count -gt 0) {
        Write-Host "`nVersions being replaced:" -ForegroundColor Cyan
        foreach ($version in $uniqueVersions) {
            Write-Host "  $($version.Name) (in $($version.Count) project(s))" -ForegroundColor Yellow
        }
    }
}

# Main execution
try {
    Write-Host "Starting package version update process..." -ForegroundColor Cyan
    Write-Host "Package: $PackageName" -ForegroundColor White
    Write-Host "New Version: $NewVersion" -ForegroundColor White
    
    if ($WhatIfPreference) {
        Write-Host "Mode: WHATIF (no changes will be made)" -ForegroundColor Yellow
    } else {
        Write-Host "Backup files: $(if ($Backup) { 'Enabled' } else { 'Disabled' })" -ForegroundColor White
    }
    
    Write-Host "`nSearching for projects using package '$PackageName'..." -ForegroundColor Cyan
    
    # Use the Get-VcxprojNugetPackageVersions script to find target packages
    $targetPackages = & $getPackagesScript -PackageFilter $PackageName -OutputFormat Object
    
    if ($targetPackages.Count -eq 0) {
        Write-Warning "No projects found using package '$PackageName'."
        return
    }
    
    Write-Host "Found $($targetPackages.Count) reference(s) to package '$PackageName'" -ForegroundColor Green
    
    # Group by project to avoid duplicate processing
    $projectGroups = $targetPackages | Group-Object ProjectFile
    
    Write-Host "`nProjects to update:" -ForegroundColor Cyan
    foreach ($group in $projectGroups) {
        $project = $group.Group[0]  # Get the first package info for project details
        Write-Host "  $($project.ProjectName) - Version(s): $($group.Group.Version -join ', ')" -ForegroundColor White
    }
    
    if ($WhatIfPreference) {
        Write-Host "`n=== WHATIF MODE - SHOWING PLANNED CHANGES ===" -ForegroundColor Yellow
    } else {
        Write-Host "`nStarting updates..." -ForegroundColor Cyan
    }
    
    $successfulPackagesConfig = 0
    $successfulVcxproj = 0
    $processedCount = 0
    
    foreach ($package in $targetPackages) {
        $processedCount++
        
        if (-not $WhatIfPreference) {
            Write-Progress -Activity "Updating package versions" -Status "Processing $($package.ProjectName)" -PercentComplete (($processedCount / $targetPackages.Count) * 100)
        }
        
        # Update packages.config
        $packagesConfigSuccess = Update-PackagesConfig -PackagesConfigPath $package.PackagesConfigPath -PackageName $PackageName -OldVersion $package.Version -NewVersion $NewVersion -TargetFramework $package.TargetFramework -CreateBackup $Backup.ToBool() -WhatIfMode $WhatIfPreference
        
        if ($packagesConfigSuccess) {
            $successfulPackagesConfig++
        }
        
        # Update .vcxproj file
        $vcxprojSuccess = Update-VcxprojFile -VcxprojPath $package.ProjectFile -PackageName $PackageName -OldVersion $package.Version -NewVersion $NewVersion -CreateBackup $Backup.ToBool() -WhatIfMode $WhatIfPreference
        
        if ($vcxprojSuccess) {
            $successfulVcxproj++
        }
    }
    
    if (-not $WhatIfPreference) {
        Write-Progress -Activity "Updating package versions" -Completed
    }
    
    # Show summary
    Show-UpdateSummary -TargetPackages $targetPackages -SuccessfulPackagesConfig $successfulPackagesConfig -SuccessfulVcxproj $successfulVcxproj -PackageName $PackageName -NewVersion $NewVersion
    
    if ($WhatIfPreference) {
        Write-Host "`nTo perform these updates, run the script without -WhatIf" -ForegroundColor Yellow
    } else {
        Write-Host "`nUpdate process completed!" -ForegroundColor Green
        
        if ($Backup) {
            Write-Host "Backup files (.bak) have been created for all modified files." -ForegroundColor Cyan
        }
    }
}
catch {
    Write-Error "An error occurred during execution: $($_.Exception.Message)"
    Write-Error $_.ScriptStackTrace
    exit 1
}
