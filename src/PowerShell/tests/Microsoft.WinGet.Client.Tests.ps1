# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Pester tests related to the Microsoft.WinGet.Client PowerShell module.
   The tests require the localhost web server to be running and serving the test data.
   'Invoke-Pester' should be called in an admin PowerShell window.
#>

BeforeAll {
    $settingsFilePath = (ConvertFrom-Json (wingetdev.exe settings export)).userSettingsFile

    $deviceGroupPolicyRoot = "HKLM:\Software\Policies\Microsoft\Windows"
    $wingetPolicyKeyName = "AppInstaller"
    $wingetGroupPolicyRegistryRoot = $deviceGroupPolicyRoot + "\" + $wingetPolicyKeyName

    Import-Module Microsoft.WinGet.Client

    function SetWinGetSettingsHelper($settings) {
        $content = ConvertTo-Json $settings -Depth 4
        Set-Content -Path $settingsFilePath -Value $content
    }

    # Source Add requires admin privileges, this will only execute successfully in an elevated PowerShell.
    function AddTestSource {
        try {
            Get-WinGetSource -Name 'TestSource'
        }
        catch {
            Add-WinGetSource -Name 'TestSource' -Arg 'https://localhost:5001/TestKit/'
        }
    }

    function RemoveTestSource {
        try {
            Get-WinGetSource -Name 'TestSource'
        }
        catch {
            # Source Remove requires admin privileges, this will only execute successfully in an elevated PowerShell.
            # This is a workaround to an issue where the server takes longer than expected to terminate when
            # running from PowerShell. This can cause other E2E tests to fail when attempting to reset the test source.
            Start-Process -FilePath "wingetdev" -ArgumentList "source remove TestSource"
        }
    }

    function CreatePolicyKeyIfNotExists()
    {
        $registryExists = test-path  -Path $wingetGroupPolicyRegistryRoot

       if(-Not($registryExists))
       {
           New-Item -Path $deviceGroupPolicyRoot -Name $wingetPolicyKeyName
       }
    }

    function CleanupGroupPolicyKeyIfExists()
    {
        $registryExists = test-path  -Path $wingetGroupPolicyRegistryRoot

        if($registryExists)
        {
           Remove-Item -Path  $wingetGroupPolicyRegistryRoot -Recurse
        }
    }

    function CleanupGroupPolicies()
    {
        $registryExists = test-path  -Path $wingetGroupPolicyRegistryRoot

        if($registryExists)
        {
            Remove-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name *
        }
    }
}

Describe 'Get-WinGetVersion' {

    It 'Get-WinGetVersion' {
        $version = Get-WinGetVersion
        $version | Should -Not -BeNullOrEmpty -ErrorAction Stop
    }
}

Describe 'Get|Add|Reset-WinGetSource' {

    BeforeAll {
        AddTestSource
    }

    It 'Get Test source' {
        $source = Get-WinGetSource -Name 'TestSource'

        $source | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $source.Name | Should -Be 'TestSource'
        $source.Argument | Should -Be 'https://localhost:5001/TestKit/'
        $source.Type | Should -Be 'Microsoft.PreIndexed.Package'
    }

    It 'Get fake source' {
        { Get-WinGetSource -Name 'Fake' } | Should -Throw
    }

    # This tests require admin
    It 'Reset Test source' {
        Reset-WinGetSource -Name TestSource
    }
}

Describe 'Find-WinGetPackage' {

    BeforeAll {
        AddTestSource
    }

    It 'Given no parameters, lists all available packages' {
        $allPackages = Find-WinGetPackage -Source TestSource
        $allPackages.Count | Should -BeGreaterThan 0
    }

    It 'Find by Id' {
        $package = Find-WinGetPackage -Source 'TestSource' -Id 'AppInstallerTest.TestExampleInstaller'

        $package | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $package.Name | Should -Be 'TestExampleInstaller'
        $package.Id | Should -Be 'AppInstallerTest.TestExampleInstaller'
        $package.Version | Should -Be '1.2.3.4'
        $package.Source | Should -Be 'TestSource'
    }

    It 'Find by Name' {
        $package = Find-WinGetPackage -Source 'TestSource' -Name 'TestPortableExe'

        $package | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $package[0].Name | Should -Be 'TestPortableExe'
        $package[1].Name | Should -Be 'TestPortableExeWithCommand'
    }

    It 'Find by Name sort by Version' {
        $package = Find-WinGetPackage -Source 'TestSource' -Name 'TestPortableExe' | Sort-Object 'Version'

        $package | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $package[0].Name | Should -Be 'TestPortableExeWithCommand'
        $package[1].Name | Should -Be 'TestPortableExe'
    }

    It 'Find package and verify PackageVersionInfo' {
        $package = Find-WinGetPackage -Source 'TestSource' -Id 'AppInstallerTest.TestPortableExe' -MatchOption Equals

        $package | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $package.AvailableVersions[0] | Should -Be '3.0.0.0'
        $package.AvailableVersions[1] | Should -Be '2.0.0.0'
        $package.AvailableVersions.Count | Should -Be 4

        $packageVersionInfo = $package.GetPackageVersionInfo("3.0.0.0")
        $packageVersionInfo.DisplayName | Should -Be 'TestPortableExe'
        $packageVersionInfo.Id | Should -Be 'AppInstallerTest.TestPortableExe'
        $packageVersionInfo.CompareToVersion("2.0.0.0") | Should -Be 'Greater'
        $packageVersionInfo.CompareToVersion("4.0.0.0") | Should -Be 'Lesser'
    }
}

Describe 'Install|Update|Uninstall-WinGetPackage' {

    BeforeAll {
        AddTestSource
    }

    It 'Install by Id' {
        $result = Install-WinGetPackage -Id AppInstallerTest.TestExeInstaller -Version '1.0.0.0'

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Install by exact Name and Version' {
        $result = Install-WinGetPackage -Name TestPortableExe -Version '2.0.0.0' -MatchOption Equals

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Update by Id' {
        $result = Update-WinGetPackage -Id AppInstallerTest.TestExeInstaller

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Update by Name' {
        $result = Update-WinGetPackage -Name TestPortableExe

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Uninstall by Id' {
        $result = Uninstall-WinGetPackage -Id AppInstallerTest.TestExeInstaller

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.UninstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Uninstall by Name' {
        $result = Uninstall-WinGetPackage -Name TestPortableExe

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.UninstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    AfterAll {
        # Uninstall all test packages after each  for proper cleanup.
        $testExe = Get-WinGetPackage -Id AppInstallerTest.TestExeInstaller -MatchOption Equals
        if ($testExe.Count -gt 0)
        {
            Uninstall-WinGetPackage -Id AppInstallerTest.TestExeInstaller 
        }

        $testPortable = Get-WinGetPackage -Id AppInstallerTest.TestPortableExe -MatchOption Equals
        if ($testPortable.Count -gt 0)
        {
            Uninstall-WinGetPackage -Id AppInstallerTest.TestPortableExe
        }
   }
}

Describe 'Get-WinGetPackage' {

    BeforeAll {
        AddTestSource
    }

    It 'Install by Id' {
        $result = Install-WinGetPackage -Id AppInstallerTest.TestExeInstaller -Version '1.0.0.0'

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Get package by Id' {
        $result = Get-WinGetPackage -Id AppInstallerTest.TestExeInstaller

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.Name | Should -Be 'TestExeInstaller'
        $result.Id | Should -Be 'AppInstallerTest.TestExeInstaller'
        $result.InstalledVersion | Should -Be '1.0.0.0'
        $result.Source | Should -Be 'TestSource'
        $result.AvailableVersions[0] | Should -Be '2.0.0.0'
    }

    It 'Get package by Name' {
        $result = Get-WinGetPackage -Name TestExeInstaller

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.Name | Should -Be 'TestExeInstaller'
        $result.Id | Should -Be 'AppInstallerTest.TestExeInstaller'
        $result.InstalledVersion | Should -Be '1.0.0.0'
        $result.Source | Should -Be 'TestSource'
        $result.AvailableVersions[0] | Should -Be '2.0.0.0'
    }

    AfterAll {
        # Uninstall all test packages after each  for proper cleanup.
        $testExe = Get-WinGetPackage -Id AppInstallerTest.TestExeInstaller
        if ($testExe.Count -gt 0)
        {
            Uninstall-WinGetPackage -Id AppInstallerTest.TestExeInstaller 
        } 
   }
}

Describe 'Get-WinGetUserSettings' {

    It 'Get settings' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $userSettings = Get-WinGetUserSettings
        $userSettings | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $userSettings.Count | Should -Be 2
        $userSettings.visual.progressBar | Should -Be 'rainbow'
        $userSettings.experimentalFeatures.experimentalArg | Should -Be $false
        $userSettings.experimentalFeatures.experimentalCmd | Should -Be $true
    }

    It 'Get settings. Bad json file' {
        Set-Content -Path $settingsFilePath -Value "Hi, im not a json. Thank you, Test."
        { Get-WinGetUserSettings } | Should -Throw
    }
}

Describe 'Test-WinGetUserSettings' {

    It 'Bad json file' {
        Set-Content -Path $settingsFilePath -Value "Hi, im not a json. Thank you, Test."

        $inputSettings = @{ visual= @{ progressBar="retro"} }
        Test-WinGetUserSettings -UserSettings $inputSettings | Should -Be $false
    }

    It 'Equal' {
        $ogSettings = @{ visual= @{ progressBar="retro"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        Test-WinGetUserSettings -UserSettings $ogSettings | Should -Be $true
    }

    It 'Equal. Ignore schema' {
        Set-Content -Path $settingsFilePath -Value '{ "$schema": "https://aka.ms/winget-settings.schema.json", "visual": { "progressBar": "retro" } }'

        $inputSettings = @{ visual= @{ progressBar="retro"} }
        Test-WinGetUserSettings -UserSettings $inputSettings | Should -Be $true
    }

    It 'Not Equal string' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="retro"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSettings -UserSettings $inputSettings | Should -Be $false
    }

    It 'Not Equal bool' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$true ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSettings -UserSettings $inputSettings | Should -Be $false
    }

    It 'Not Equal. More settings' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false }}
        Test-WinGetUserSettings -UserSettings $inputSettings | Should -Be $false
    }

    It 'Not Equal. More settings input' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSettings -UserSettings $inputSettings | Should -Be $false
    }

    It 'Equal IgnoreNotSet' {
        $ogSettings = @{ visual= @{ progressBar="retro"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        Test-WinGetUserSettings -UserSettings $ogSettings -IgnoreNotSet | Should -Be $true
    }

    It 'Equal IgnoreNotSet. More settings' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false }}
        Test-WinGetUserSettings -UserSettings $inputSettings -IgnoreNotSet | Should -Be $true
    }

    It 'Not Equal IgnoreNotSet. More settings input' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSettings -UserSettings $inputSettings -IgnoreNotSet | Should -Be $false
    }

    It 'Not Equal bool IgnoreNotSet' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$true ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSettings -UserSettings $inputSettings -IgnoreNotSet | Should -Be $false
    }

    It 'Not Equal array IgnoreNotSet' {
        $ogSettings = @{ installBehavior= @{ preferences= @{ architectures = @("x86", "x64")} }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ installBehavior= @{ preferences= @{ architectures = @("x86", "arm64")} }}
        Test-WinGetUserSettings -UserSettings $inputSettings -IgnoreNotSet | Should -Be $false
    }

    It 'Not Equal wrong type IgnoreNotSet' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$true ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=4 ; experimentalCmd=$true}}
        Test-WinGetUserSettings -UserSettings $inputSettings -IgnoreNotSet | Should -Be $false
    }

    AfterAll {
        SetWinGetSettingsHelper @{ debugging= @{ enableSelfInitiatedMinidump=$true ; keepAllLogFiles=$true } }
    }
}

Describe 'Set-WinGetUserSettings' {

    It 'Overwrites' {
        $ogSettings = @{ source= @{ autoUpdateIntervalInMinutes=3}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$true ; experimentalCmd=$false}}
        $result = Set-WinGetUserSettings -UserSettings $inputSettings

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.'$schema' | Should -Not -BeNullOrEmpty 
        $result.visual | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.visual.progressBar | Should -Be "rainbow"
        $result.experimentalFeatures | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.experimentalFeatures.experimentalArg | Should -Be $true
        $result.experimentalFeatures.experimentalCmd | Should -Be $false
        $result.source | Should -BeNullOrEmpty
    }

    It 'Merge' {
        $ogSettings = @{ source= @{ autoUpdateIntervalInMinutes=3}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$true ; experimentalCmd=$false}}
        $result = Set-WinGetUserSettings -UserSettings $inputSettings -Merge

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.'$schema' | Should -Not -BeNullOrEmpty 
        $result.visual | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.visual.progressBar | Should -Be "rainbow"
        $result.experimentalFeatures | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.experimentalFeatures.experimentalArg | Should -Be $true
        $result.experimentalFeatures.experimentalCmd | Should -Be $false
        $result.source | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.source.autoUpdateIntervalInMinutes | Should -Be 3
    }

    It 'Schema.' {
        Set-Content -Path $settingsFilePath -Value '{ "$schema": "https://aka.ms/winget-settings.schema.json", "visual": { "progressBar": "retro" } }'

        $inputSettings = @{ visual= @{ progressBar="retro"} }
        $result = Set-WinGetUserSettings -UserSettings $inputSettings

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.'$schema' | Should -Not -BeNullOrEmpty 
        $result.visual.progressBar | Should -Be "retro"
    }

    It 'Overwrites Bad json file' {
        Set-Content -Path $settingsFilePath -Value "Hi, im not a json. Thank you, Test."

        $inputSettings = @{ visual= @{ progressBar="retro"} }
        $result = Set-WinGetUserSettings -UserSettings $inputSettings

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.'$schema' | Should -Not -BeNullOrEmpty 
        $result.visual.progressBar | Should -Be "retro"
    }

    It 'Overwrites Bad json file' {
        Set-Content -Path $settingsFilePath -Value "Hi, im not a json. Thank you, Test."

        $inputSettings = @{ visual= @{ progressBar="retro"} }
        { Set-WinGetUserSettings -UserSettings $inputSettings -Merge } | Should -Throw
    }

    AfterAll {
        SetWinGetSettingsHelper @{ debugging= @{ enableSelfInitiatedMinidump=$true ; keepAllLogFiles=$true } }
    }
}

Describe 'Get|Enable|Disable-WinGetSetting' {

    It 'Get-WinGetSetting' {
        $settings = Get-WinGetSettings
        $settings | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $settings.'$schema' | Should -Not -BeNullOrEmpty 
        $settings.adminSettings | Should -Not -BeNullOrEmpty
        $settings.userSettingsFile | Should -Be $settingsFilePath
    }

    # This tests require admin
    It 'Enable|Disable' {
        $settings = Get-WinGetSettings
        $settings | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $settings.adminSettings | Should -Not -BeNullOrEmpty
        $settings.adminSettings.LocalManifestFiles | Should -Be $false

        Enable-WinGetSetting -Name LocalManifestFiles

        $afterEnable = Get-WinGetSettings
        $afterEnable | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $afterEnable.adminSettings | Should -Not -BeNullOrEmpty
        $afterEnable.adminSettings.LocalManifestFiles | Should -Be $true

        Disable-WingetSetting -Name LocalManifestFiles

        $afterDisable = Get-WinGetSettings
        $afterDisable | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $afterDisable.adminSettings | Should -Not -BeNullOrEmpty
        $afterDisable.adminSettings.LocalManifestFiles | Should -Be $false
    }
}

Describe 'Test-GroupPolicies' {
    BeforeAll {
        CleanupGroupPolicies
        CreatePolicyKeyIfNotExists
    }

    It "Disable WinGetPolicy and run Get-WinGetSources" {
        $policyKeyValueName =  "EnableAppInstaller"

        Set-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName -Value 0
        $registryKey  =  Get-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName
        $registryKey | Should -Not -BeNullOrEmpty
        $registryKey.EnableAppInstaller | Should -Be 0

        { Get-WinGetSource } | Should -Throw "This operation is disabled by Group Policy : Enable Windows Package Manager"

        CleanupGroupPolicies
    }

    It "Disable EnableWindowsPackageManagerCommandLineInterfaces Policy and run Get-WinGetSources" {
       $policyKeyValueName =  "EnableWindowsPackageManagerCommandLineInterfaces"

        Set-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName -Value 0
        $registryKey  =  Get-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName
        $registryKey | Should -Not -BeNullOrEmpty
        $registryKey.EnableWindowsPackageManagerCommandLineInterfaces | Should -Be 0

        { Get-WinGetSource } | Should -Throw "This operation is disabled by Group Policy : Enable Windows Package Manager command line interfaces"

        CleanupGroupPolicies
    }

    AfterAll {
        CleanupGroupPolicies
        CleanupGroupPolicyKeyIfExists
    }
}

AfterAll {
    RemoveTestSource
}