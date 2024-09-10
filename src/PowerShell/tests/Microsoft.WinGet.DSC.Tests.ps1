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
}

Describe 'List available DSC resources'{
    It 'Shows DSC Resources'{
        $expectedDSCResources = "WinGetAdminSettings", "WinGetPackage", "WinGetPackageManager", "WinGetSource", "WinGetUserSettings"
        $availableDSCResources = (Get-DscResource -Module Microsoft.WinGet.DSC).Name
        $availableDSCResources.length | Should -Be 5
        $availableDSCResources | Where-Object {$expectedDSCResources -notcontains $_} | Should -BeNullOrEmpty -ErrorAction Stop
    }
}

Describe 'WinGetAdminSettings' {

    BeforeAll {
        $initialAdminSettings = (Get-WinGetSetting).adminSettings
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
        $settingsFilePath = (Get-WinGetSetting).userSettingsFile
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

Describe 'WinGetSource' {
    BeforeAll {
        $testSourceName = 'TestSource'
        $testSourceArg = 'https://localhost:5001/TestKit/'
        $testSourceType = 'Microsoft.PreIndexed.Package'

        InvokeWinGetDSC -Name WinGetSource -Method Set -Property @{ Ensure = 'Absent'; Name = $testSourceName }
    }

    It 'Get WinGet source' {  
        $result = InvokeWinGetDSC -Name WinGetSource -Method Get -Property @{ Name = $testSourceName }
        $result.Ensure  | Should -Be 'Absent'
    }

    It 'Test WinGet source' {
        $result = InvokeWinGetDSC -Name WinGetSource -Method Test -Property @{ Ensure='Present'; Name = $testSourceName; Argument = $testSourceArg; Type = $testSourceType }
        $result.InDesiredState | Should -Be $false
    }

    It 'Set WinGet source' {
        InvokeWinGetDSC -Name WinGetSource -Method Set -Property @{ Name = $testSourceName; Argument = $testSourceArg; Type = $testSourceType; TrustLevel = 'Trusted'; Explicit = $true }

        $result = InvokeWinGetDSC -Name WinGetSource -Method Test -Property @{ Name = $testSourceName; Argument = $testSourceArg; Type = $testSourceType }
        $result.InDesiredState | Should -Be $true

        $result = InvokeWinGetDSC -Name WinGetSource -Method Get -Property @{ Name = $testSourceName }
        $result.Name  | Should -Be $testSourceName
        $result.Type | Should -Be $testSourceType
        $result.Argument | Should -Be $testSourceArg
        $result.TrustLevel | Should -Be 'Trusted'
        $result.Explicit | Should -Be $true
    }
}

Describe 'WinGetPackage' {
    BeforeAll {
        $testSourceName = 'TestSource'
        $testSourceArg = 'https://localhost:5001/TestKit/'
        $testSourceType = 'Microsoft.PreIndexed.Package'

        $testPackageId = 'AppInstallerTest.TestExeInstaller'
        $testPackageVersion = '1.0.0.0'

        InvokeWinGetDSC -Name WinGetSource -Method Set -Property @{ Name = $testSourceName; Argument = $testSourceArg; Type = $testSourceType; TrustLevel = 'Trusted' }
    }

    It 'Get WinGetPackage' {
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Get -Property @{ Id = $testPackageId; Version = $testPackageVersion }
        $result.Ensure | Should -Be 'Absent'
    }

    It 'Test WinGetPackage' {
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Test -Property @{ Id = $testPackageId; Version = $testPackageVersion }
        $result.InDesiredState | Should -Be $false
    }

    It 'Install WinGetPackage' {
        InvokeWinGetDSC -Name WinGetPackage -Method Set -Property @{ Id = $testPackageId; Version = $testPackageVersion }

        # Verify package installed.
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Get -Property @{ Id = $testPackageId; Version = $testPackageVersion }
        $result.Ensure | Should -Be 'Present'
        $result.UseLatest | Should -Be $false
        $result.Version | Should -Be $testPackageVersion
    }

    It 'Update WinGetPackage' {
        $testResult = InvokeWinGetDSC -Name WinGetPackage -Method Test -Property @{ Id = $testPackageId; UseLatest = $true }
        $testResult.InDesiredState | Should -Be $false

        InvokeWinGetDSC -Name WinGetPackage -Method Set -Property @{ Id = $testPackageId; UseLatest = $true }

        # Verify package updated.
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Get -Property @{ Id = $testPackageId; UseLatest = $true }
        $result.Ensure | Should -Be 'Present'
        $result.UseLatest | Should -Be $true
        $result.Version | Should -Not -Be $testPackageVersion
    } 

    It 'Uninstall WinGetPackage' {
        InvokeWinGetDSC -Name WinGetPackage -Method Set -Property @{ Id = $testPackageId; UseLatest = $true }

        $testResult = InvokeWinGetDSC -Name WinGetPackage -Method Test -Property @{ Ensure = 'Absent'; Id = $testPackageId }
        $testResult.InDesiredState | Should -Be $false

        InvokeWinGetDSC -Name WinGetPackage -Method Set -Property @{ Ensure = 'Absent'; Id = $testPackageId }

        # Verify package uninstalled.
        $result = InvokeWinGetDSC -Name WinGetPackage -Method Get -Property @{ Ensure = 'Absent'; Id = $testPackageId }
        $result.Ensure | Should -Be 'Absent'
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
