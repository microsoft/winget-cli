# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Pester tests related to the Microsoft.WinGet.DSC PowerShell module.
   The tests require the localhost web server to be running and serving the test data.
   'Invoke-Pester' should be called in an admin PowerShell window.
#>

BeforeAll {
    Install-Module -Name PSDesiredStateConfiguration -Force -SkipPublisherCheck
    Import-Module Microsoft.WinGet.Client
    Import-Module Microsoft.WinGet.DSC

    # Helper function for calling Invoke-DscResource on the Microsoft.WinGet.DSC module.
    function InvokeWinGetDSC() {
        param (
            [Parameter()]
            [string]$Name,

            [Parameter()]
            [string]$Method,

            [Parameter()]
            [hashtable]$Property
        )

        return Invoke-DscResource -Name $Name -ModuleName Microsoft.WinGet.DSC -Method $Method -Property $Property
    }

    function AddTestSource {
        try {
            Get-WinGetSource -Name 'TestSource'
        }
        catch {
            # TODO: Add-WinGetSource does not support setting trust level yet.
            # Add-WinGetSource -Name 'TestSource' -Arg 'https://localhost:5001/TestKit/'
            $sourceAddCommand = "wingetdev.exe source add TestSource https://localhost:5001/TestKit/ --trust-level trusted"
            Invoke-Expression -Command $sourceAddCommand
        }
    }
}

Describe 'List available DSC resources'{
    It 'Shows DSC Resources'{
        $expectedDSCResources = "WinGetAdminSettings", "WinGetPackage", "WinGetPackageManager", "WinGetSources", "WinGetUserSettings"
        $availableDSCResources = (Get-DscResource -Module Microsoft.WinGet.DSC).Name
        $availableDSCResources.length | Should -Be 5
        $availableDSCResources | Where-Object {$expectedDSCResources -notcontains $_} | Should -BeNullOrEmpty -ErrorAction Stop
    }
}

Describe 'WinGetAdminSettings' {

    BeforeAll {
        $initialAdminSettings = (Get-WinGetSettings).adminSettings
        $adminSettingsHash = @{
            BypassCertificatePinningForMicrosoftStore = !$initialAdminSettings.BypassCertificatePinningForMicrosoftStore;
            InstallerHashOverride = !$initialAdminSettings.InstallerHashOverride;
            LocalManifestFiles = !$initialAdminSettings.LocalManifestFiles;
            LocalArchiveMalwareScanOverride = !$initialAdminSettings.LocalArchiveMalwareScanOverride;
        }
    }

    It 'Get admin settings' {
        $result = InvokeWinGetDSC -Name WinGetAdminSettings -Method Get -Property @{ Settings = $adminSettingsHash }
        $adminSettings = $result.Settings
        $adminSettings.BypassCertificatePinningForMicrosoftStore | Should -Be $initialAdminSettings.BypassCertificatePinningForMicrosoftStore
        $adminSettings.InstallerHashOverride | Should -Be $initialAdminSettings.InstallerHashOverride
        $adminSettings.LocalManifestFiles | Should -Be $initialAdminSettings.LocalManifestFiles
        $adminSettings.LocalArchiveMalwareScanOverride | Should -Be $initialAdminSettings.LocalArchiveMalwareScanOverride
    }

    It 'Test admin settings' {
        $result = InvokeWinGetDSC -Name WinGetAdminSettings -Method Test -Property @{ Settings = $adminSettingsHash }
        $result.InDesiredState | Should -Be $false
    }

    It 'Set admin settings' {
        InvokeWinGetDSC -Name WinGetAdminSettings -Method Set -Property @{ Settings = $adminSettingsHash }

        # Verify settings were applied.
        $result = InvokeWinGetDSC -Name WinGetAdminSettings -Method Get -Property @{ Settings = $adminSettingsHash }
        $adminSettings = $result.Settings
        $adminSettings.BypassCertificatePinningForMicrosoftStore | Should -Not -Be $initialAdminSettings.BypassCertificatePinningForMicrosoftStore
        $adminSettings.InstallerHashOverride | Should -Not -Be $initialAdminSettings.InstallerHashOverride
        $adminSettings.LocalManifestFiles | Should -Not -Be $initialAdminSettings.LocalManifestFiles
        $adminSettings.LocalArchiveMalwareScanOverride | Should -Not -Be $initialAdminSettings.LocalArchiveMalwareScanOverride

        $testResult = InvokeWinGetDSC -Name WinGetAdminSettings -Method Test -Property @{ Settings = $adminSettingsHash }
        $testResult | Should -Be $true
    }

    AfterAll {
        InvokeWinGetDSC -Name WinGetAdminSettings -Method Set -Property @{ Settings = $initialAdminSettings }
    }
}

Describe 'WinGetUserSettings' {
    BeforeAll {
        # Delete existing user settings file.
        $settingsFilePath = (Get-WinGetSettings).userSettingsFile
        $backupSettingsFilePath = $settingsFilePath + ".backup"

        if (Test-Path -Path $settingsFilePath)
        {
            Remove-Item $settingsFilePath
        }

        if (Test-Path -Path $backupSettingsFilePath)
        {
            Remove-Item $backupSettingsFilePath
        }

        $userSettingsHash = @{
            experimentalFeatures = @{ directMSI = $true };
            installBehavior = @{ Preferences = @{ Scope = 'User' }}
        }
    }

    It 'Get user settings' {
        $result = InvokeWinGetDSC -Name WinGetUserSettings -Method Get -Property @{ Settings = $userSettingsHash }
        $result.Settings.Count | Should -Be 0
    }

    It 'Test user settings' {
        $result = InvokeWinGetDSC -Name WinGetUserSettings -Method Test -Property @{ Settings = $userSettingsHash }
        $result.InDesiredState | Should -Be $false
    }

    It 'Set user settings' {
        InvokeWinGetDSC -Name WinGetUserSettings -Method Set -Property @{ Settings = $userSettingsHash }

        # Verify user settings were applied.
        $result = InvokeWinGetDSC -Name WinGetUserSettings -Method Get -Property @{ Settings = $userSettingsHash }
        $userSettings = $result.Settings
        $userSettings.experimentalFeatures.directMSI | Should -Be $true
        $userSettings.installBehavior.Preferences.Scope | Should -Be 'User'
    }
}

Describe 'WinGetSources' {
    BeforeAll {
        $testSourceName = 'TestSource'

        $testSourceValue = @{
            Type = 'Microsoft.PreIndexed.Package'
            Arg = 'https://localhost:5001/TestKit/'
        }
        
        InvokeWinGetDSC -Name WinGetSources -Method Set -Property @{ Action = 'Partial'; Ensure = 'Absent'; Sources = @{ $testSourceName = $testSourceValue }}
    }

    It 'Get WinGet source' {  
        $result = InvokeWinGetDSC -Name WinGetSources -Method Get -Property @{ Sources = @{ $testSourceName = $testSourceValue }}
        $result.Sources.Keys  | Should -Not -Contain $testSourceName
    }

    It 'Test WinGet source' {
        $result = InvokeWinGetDSC -Name WinGetSources -Method Test -Property @{ Ensure='Present'; Sources = @{ $testSourceName = $testSourceValue }}
        $result.InDesiredState | Should -Be $false
    }

    It 'Set WinGet source' {
        # InvokeWinGetDSC -Name WinGetSources -Method Set -Property @{ Ensure = 'Present'; Sources = @{ $testSourceName = $testSourceValue }}
        # TODO: Replace with DSC once '--trust-level' is supported
        AddTestSource

        $result = InvokeWinGetDSC -Name WinGetSources -Method Get -Property @{ Sources = @{ $testSourceName = $testSourceValue }}
        $result.Sources.Keys  | Should -Contain $testSourceName

        $testSource = $result.Sources.$($testSourceName)
        $testSource.Type | Should -Be 'Microsoft.PreIndexed.Package'
        $testSource.Arg | Should -Be 'https://localhost:5001/TestKit/'
        $testSource.Identifier | Should -Be $null
    }
}

Describe 'WinGetPackage' {
    BeforeAll {
        $testPackageId = 'AppInstallerTest.TestExeInstaller'
        $testPackageVersion = '1.0.0.0'

        # Add test source.
        # InvokeWinGetDSC -Name WinGetSources -Method Set -Property @{ Action = 'Partial'; Ensure = 'Present'; Sources = @{ TestSource = @{ Arg = 'https://localhost:5001/TestKit/'; Type = 'Microsoft.PreIndexed.Package' }}}

        # TODO: Replace with DSC once '--trust-level' is supported.
        AddTestSource
    }

    It 'Get WinGetPackage' {
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Get -Property @{ Id = $testPackageId; Version = $testPackageVersion }
        $result.IsInstalled | Should -Be $false
    }

    It 'Test WinGetPackage' {
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Test -Property @{ Id = $testPackageId; Version = $testPackageVersion }
        $result.InDesiredState | Should -Be $false
    }

    It 'Install WinGetPackage' {
        InvokeWinGetDSC -Name WinGetPackage -Method Set -Property @{ Id = $testPackageId; Version = $testPackageVersion }

        # Verify package installed.
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Get -Property @{ Id = $testPackageId; Version = $testPackageVersion }
        $result.IsInstalled | Should -Be $true
        $result.IsUpdateAvailable | Should -Be $true
        $result.InstalledVersion | Should -Be 1.0.0.0
    }

    It 'Update WinGetPackage' {
        $testResult = InvokeWinGetDSC -Name WinGetPackage -Method Test -Property @{ Id = $testPackageId; UseLatest = $true }
        $testResult.InDesiredState | Should -Be $false

        InvokeWinGetDSC -Name WinGetPackage -Method Set -Property @{ Id = $testPackageId; UseLatest = $true }

        # Verify package updated.
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Get -Property @{ Id = $testPackageId; UseLatest = $true }
        $result.IsInstalled | Should -Be $true
        $result.IsUpdateAvailable | Should -Be $false
        $result.InstalledVersion | Should -Not -Be 1.0.0.0
    } 

    It 'Uninstall WinGetPackage' {
        InvokeWinGetDSC -Name WinGetPackage -Method Set -Property @{ Id = $testPackageId; UseLatest = $true }

        $testResult = InvokeWinGetDSC -Name WinGetPackage -Method Test -Property @{ Ensure = 'Absent'; Id = $testPackageId }
        $testResult.InDesiredState | Should -Be $false

        InvokeWinGetDSC -Name WinGetPackage -Method Set -Property @{ Ensure = 'Absent'; Id = $testPackageId }

        # Verify package uninstalled.
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Get -Property @{ Ensure = 'Absent'; Id = $testPackageId }
        $result.IsInstalled | Should -Be $false
    }

    AfterAll {
        InvokeWinGetDSC -Name WinGetPackage -Method Set -Property @{ Ensure = 'Absent'; Id = $testPackageId}
    }
}

Describe 'WinGetPackageManager' {
    It 'Get WinGet version' {
        $result = InvokeWinGetDSC -Name WinGetPackageManager -Method Get -Property @{}
        $result.Version | Should -Not -Be $null
    }

    It 'Test WinGet version' {
        $result = InvokeWinGetDSC -Name WinGetPackageManager -Method Test -Property @{ Version = "1.2.3.4" }
        $result.InDesiredState | Should -Be $false

        $currentVersion = Get-WinGetVersion
        $result = InvokeWinGetDSC -Name WinGetPackageManager -Method Test -Property @{ Version = $currentVersion }
        $result.InDesiredState | Should -Be $true
    }

    # TODO: Add test to verify Set method for WinGetPackageManager
}
