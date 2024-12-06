# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

<#
.SYNOPSIS
Sets up dependencies required to build and work with the SFS Client.

.DESCRIPTION
This script will install all of the dependencies required to build and work with the SFS Client.
Use this on Windows platforms in a PowerShell session.

.EXAMPLE
PS> .\scripts\Setup.ps1
#>

$ErrorActionPreference = "Stop"

$GitRoot = (Resolve-Path (&git -C $PSScriptRoot rev-parse --show-toplevel)).Path

function Update-Env {
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
}

function Install-Python {
    Write-Host -ForegroundColor Cyan "Installing latest Python if it's not installed"

    # We use the "python --version" command to check if it exists because by default Windows now comes
    # with a python.exe from the Store that simply opens the Store when someone tries to use python.exe
    python --version 2>&1 | Out-Null
    if (!$?) {
        winget install python
        if (!$?) {
            Write-Host -ForegroundColor Red "Failed to install Python"
            exit 1
        }
    }
}

function Install-PipDependencies {
    Write-Host -ForegroundColor Cyan "`nInstalling dependencies using pip"

    # Upgrade pip and install requirements. Filter out output for dependencies that are already installed
    $PipInstalledPackageString = "Requirement already satisfied"
    python -m pip install --upgrade pip | Select-String -Pattern $PipInstalledPackageString -NotMatch

    # Installing to user site packages
    $PipReqs = Join-Path $PSScriptRoot "pip.requirements.txt" -Resolve
    pip install --user -r $PipReqs | Select-String -Pattern $PipInstalledPackageString -NotMatch
}

function Install-CMake {
    Write-Host -ForegroundColor Cyan "`nInstalling cmake if it's not installed"

    # Installing cmake from winget because it adds to PATH
    try {
        cmake --version
    }
    catch {
        winget install cmake
        if (!$?) {
            Write-Host -ForegroundColor Red "Failed to install cmake"
            exit 1
        }

        # Update env with newly installed cmake
        Update-Env
    }
}

function Install-CppBuildTools {
    Write-Host -ForegroundColor Cyan "`nInstalling C++ Builds tools if they are not installed"

    # Instaling vswhere, which will be used to query for the required build tools
    try {
        vswhere -? 2>&1 | Out-Null
    }
    catch {
        winget install vswhere
        if (!$?) {
            Write-Host -ForegroundColor Red "Failed to install vswhere"
            exit 1
        }
    }

    # - Microsoft.VisualStudio.Workload.VCTools is the C++ workload in the Visual Studio Build Tools
    # - Microsoft.VisualStudio.Workload.NativeDesktop is the C++ workload that comes pre-installed in the github runner image
    $ExistingBuildTools = vswhere -products * -requires Microsoft.VisualStudio.Workload.VCTools Microsoft.VisualStudio.Workload.NativeDesktop -requiresAny -format json | ConvertFrom-Json
    if ($null -eq $ExistingBuildTools)
    {
        Write-Host "`nTools not found, installing..."

        # --wait makes the install synchronous
        winget install Microsoft.VisualStudio.2022.BuildTools --silent --override "--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --remove Microsoft.VisualStudio.Component.VC.CMake.Project"
        if (!$?) {
            Write-Host -ForegroundColor Red "Failed to install build tools"
            exit 1
        }
    }
}

function Install-Vcpkg {
    Write-Host -ForegroundColor Cyan "`nSetting up vcpkg"

    if (!(Test-Path vcpkg)) {
        Write-Host "Cloning vcpkg repo"
        git clone https://github.com/microsoft/vcpkg $GitRoot\vcpkg
    }
    else {
        Write-Host "Checking if vcpkg repo has new commits"
        $NoUpdatesString = "Already up to date."
        git -C "$GitRoot\vcpkg" pull --show-forced-updates | Select-String -Pattern $NoUpdatesString -NotMatch
    }

    # Bootstrapping on every setup updates the vcpkg.exe and solves potential issues with VS Build Tools not being found
    & "$GitRoot\vcpkg\bootstrap-vcpkg.bat"
}

function Set-GitHooks {
    Write-Host -ForegroundColor Cyan "`nSetting Git hooks"

    $HookDestDir = Join-Path $GitRoot "\.git\hooks" -Resolve
    $GitHooks = @{"pre-commit-wrapper.sh" = "pre-commit" }
    foreach ($i in $GitHooks.GetEnumerator()) {
        $HookSrc = Join-Path $GitRoot $i.Name -Resolve
        $HookDest = Join-Path $HookDestDir $i.Value

        # If the destination doesn't exist or is different than the one in the source, we'll copy it over.
        if (-not (Test-Path $HookDest) -or (Get-FileHash $HookDest).Hash -ne (Get-FileHash $HookSrc).Hash) {
            Copy-Item -Path $HookSrc -Destination $HookDest -Force | Out-Null
            Write-Host -ForegroundColor Cyan "Setup git $($i.Value) hook with $HookSrc"
        }
    }
}

function Set-Aliases {
    Write-Host -ForegroundColor Cyan "`nSetting aliases"

    $PythonScriptsDir = python -c "import os,sysconfig;print(sysconfig.get_path('scripts',f'{os.name}_user'))"

    $Aliases = [ordered]@{
        "build"        = "$GitRoot\scripts\Build.ps1"
        "clang-format" = "$PythonScriptsDir\clang-format.exe"
        "cmake-format" = "$PythonScriptsDir\cmake-format.exe"
        "test"         = "$GitRoot\scripts\Test.ps1"
        "vcpkg"        = "$GitRoot\vcpkg\vcpkg.exe"
    }
    foreach ($i in $Aliases.GetEnumerator()) {
        $Alias = $i.Name
        $Target = $i.Value

        # If the alias doesn't exist or is set to something different than the one we want to target, we'll add it.
        $AliasDoesNotExist = !(Get-Alias $Alias -ErrorAction SilentlyContinue)
        if ($AliasDoesNotExist -or (Get-Alias $Alias).Definition -ne $Target) {
            if (!$AliasDoesNotExist) {
                Remove-Alias $Alias -Scope Global
            }
            New-Alias -Name $Alias -Value $Target -Scope Global
            Write-Host "Setup alias $Alias"
        }
    }
}

Install-Python
Install-PipDependencies
Install-CMake
Install-CppBuildTools
Install-Vcpkg
Set-GitHooks
Set-Aliases
