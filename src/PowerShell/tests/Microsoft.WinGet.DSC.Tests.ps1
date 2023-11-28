# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Pester tests related to the Microsoft.WinGet.DSC PowerShell module.
   The tests require the localhost web server to be running and serving the test data.
   'Invoke-Pester' should be called in an admin PowerShell window.
#>

BeforeAll {
    Import-Module -Name Microsoft.WinGet.DSC
}


Describe 'Get-WinGetDscResources'{
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
        $settingsHash = @{
            BypassCertificatePinningForMicrosoftStore = !$initialAdminSettings.BypassCertificatePinningForMicrosoftStore;
            InstallerHashOverride = !$initialAdminSettings.InstallerHashOverride;
            LocalManifestFiles = !$initialAdminSettings.LocalManifestFiles;
            LocalArchiveMalwareScanOverride = !$initialAdminSettings.LocalArchiveMalwareScanOverride;
        }
    }

    It 'Get admin settings' {
        $initialAdminSettings.BypassCertificatePinningForMicrosoftStore | Should -Be $False -ErrorAction Stop

        $getResult = Invoke-DscResource -Name WinGetAdminSettings -ModuleName Microsoft.WinGet.DSC -Method Get -Property @{ Settings = $settingsHash }
        $getResult.Settings.BypassCertificatePinningForMicrosoftStore | Should -Be $initialAdminSettings.BypassCertificatePinningForMicrosoftStore
        $getResult.Settings.InstallerHashOverride | Should -Be $initialAdminSettings.InstallerHashOverride
        $getResult.Settings.LocalManifestFiles | Should -Be $initialAdminSettings.LocalManifestFiles
        $getResult.Settings.LocalArchiveMalwareScanOverride | Should -Be $initialAdminSettings.LocalArchiveMalwareScanOverride
    }

    It 'Test admin settings' {
        $testResult = Invoke-DscResource -Name WinGetAdminSettings -ModuleName Microsoft.WinGet.DSC -Method Test -Property @{ Settings = $settingsHash }
        $testResult.InDesiredState | Should -Be $false
    }

    It 'Set admin settings' {
        Invoke-DscResource -Name WinGetAdminSettings -ModuleName Microsoft.WinGet.DSC -Method Set -Property @{ Settings = $settingsHash }
        
        # Verify admin settings have been set.
        $getResult = Invoke-DscResource -Name WinGetAdminSettings -ModuleName Microsoft.WinGet.DSC -Method Get -Property @{ Settings = $settingsHash }
        $getResult.Settings.BypassCertificatePinningForMicrosoftStore | Should -Not -Be $initialAdminSettings.BypassCertificatePinningForMicrosoftStore
        $getResult.Settings.InstallerHashOverride | Should -Not -Be $initialAdminSettings.InstallerHashOverride
        $getResult.Settings.LocalManifestFiles | Should -Not -Be $initialAdminSettings.LocalManifestFiles
        $getResult.Settings.LocalArchiveMalwareScanOverride | Should -Not -Be $initialAdminSettings.LocalArchiveMalwareScanOverride

        $testResult = Invoke-DscResource -Name WinGetAdminSettings -ModuleName Microsoft.WinGet.DSC -Method Test -Property @{ Settings = $settingsHash }
        $testResult | Should -Be $true
    }

    AfterAll {
        # Reset back to original state
        Invoke-DscResource -Name WinGetAdminSettings -ModuleName Microsoft.WinGet.DSC -Method Set -Property @{ Settings = $initialAdminSettings }
    }
}
