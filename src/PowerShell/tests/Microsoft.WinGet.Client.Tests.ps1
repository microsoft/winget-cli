# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Pester tests related to the Microsoft.WinGet.Client PowerShell module.
   The tests require the localhost web server to be running and serving the test data.
   'Invoke-Pester' should be called in an admin PowerShell window.
#>
[CmdletBinding()]
param(
    # Whether to use production or developement targets.
    [switch]$TargetProduction
)

BeforeAll {
    if ($TargetProduction)
    {
        $wingetExeName = "winget.exe"
    }
    else
    {
        $wingetExeName = "wingetdev.exe"
    }

    $settingsFilePath = (ConvertFrom-Json (& $wingetExeName settings export)).userSettingsFile

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
            Add-WinGetSource -Name 'TestSource' -Arg 'https://localhost:5001/TestKit/' -TrustLevel 'Trusted'
        }
    }

    # This is a workaround to an issue where the server takes longer than expected to terminate when
    # running from PowerShell. This can cause other E2E tests to fail when attempting to reset the test source.
    function RemoveTestSource {
        try {
            # Source Remove requires admin privileges, this will only execute successfully in an elevated PowerShell.
            $testSource = Get-WinGetSource | Where-Object -Property 'Name' -eq 'TestSource'
            if ($null -ne $testSource)
            {
                # Source Remove requires admin privileges
                Remove-WinGetSource -Name 'TestSource'
            }
        }
        catch {
            # Non-admin
            Start-Process -FilePath $wingetExeName -ArgumentList "source remove TestSource"
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

    function WaitForWindowsPackageManagerServer([bool]$force = $false)
    {
        $processes = Get-Process | Where-Object { $_.Name -eq "WindowsPackageManagerServer" }
        foreach ($p in $processes)
        {
            if ($force)
            {
                Stop-Process $p
            }

            $timeout = 300
            $secondsToWait = 5
            $time = 0
            while ($p.HasExited -eq $false)
            {
                $time += $secondsToWait
                if ($time -ge $timeout )
                {
                    throw "Timeout waiting for $($p.Id) to exit"
                }
                Start-Sleep -Seconds 5
            }
        }
    }

    function GetRandomTestDirectory()
    {
        return Join-Path -Path $env:Temp -ChildPath "WingetPwshTest-$(New-Guid)"
    }

    function Validate-WinGetResultCommonFields([psobject]$result, [psobject]$expected) {
        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.Id | Should -Be $expected.Id
        $result.Name | Should -Be $expected.Name
        $result.Source | Should -Be $expected.Source
        $result.Status | Should -Be $expected.Status
    }

    function Validate-WinGetPackageOperationResult([psobject]$result, [psobject]$expected, [string]$operationType)
    {
        Validate-WinGetResultCommonFields $result $expected
        $result.RebootRequired | Should -Be $expected.RebootRequired

        switch ($operationType) {
            'install' {
                $result.InstallerErrorCode | Should -Be $expected.InstallerErrorCode
            }
            'update' {
                $result.InstallerErrorCode | Should -Be $expected.InstallerErrorCode
            }
            'repair' {
                $result.RepairErrorCode | Should -Be $expected.RepairErrorCode
            }
            'uninstall' {
                $result.UninstallerErrorCode | Should -Be $expected.UninstallerErrorCode
            }
            default {
                throw "Unknown operation type: $operationType"
            }
        }
    }
}

Describe 'Get-WinGetVersion' {

    It 'Get-WinGetVersion' {
        $version = Get-WinGetVersion
        $version | Should -Not -BeNullOrEmpty -ErrorAction Stop
    }
}

Describe 'Reset-WinGetSource' {
    BeforeAll {
        AddTestSource
    }

    # Requires admin
    It 'Resets all sources' {
        Reset-WinGetSource -All
    }

    It 'Test source should be removed' {
        { Get-WinGetSource -Name 'TestSource' } | Should -Throw
    }
}

Describe 'Get|Add|Reset-WinGetSource' {

    BeforeAll {
        Add-WinGetSource -Name 'TestSource' -Arg 'https://localhost:5001/TestKit/' -TrustLevel 'Trusted' -Explicit
    }

    It 'Get Test source' {
        $source = Get-WinGetSource -Name 'TestSource'

        $source | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $source.Name | Should -Be 'TestSource'
        $source.Argument | Should -Be 'https://localhost:5001/TestKit/'
        $source.Type | Should -Be 'Microsoft.PreIndexed.Package'
        $source.TrustLevel | Should -Be 'Trusted'
        $source.Explicit | Should -Be $true
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

    BeforeEach {
        $expectedExeInstallerResult = [PSCustomObject]@{
            Id = "AppInstallerTest.TestExeInstaller"
            Name = "TestExeInstaller"
            Source = "TestSource"
            Status = 'Ok'
            RebootRequired = 'False'
            InstallerErrorCode = 0
            UninstallerErrorCode = 0
        }

        $expectedPortableInstallerResult = [PSCustomObject]@{
            Id = "AppInstallerTest.TestPortableExe"
            Name = "TestPortableExe"
            Source = "TestSource"
            Status = 'Ok'
            RebootRequired = 'False'
            InstallerErrorCode = 0
            UninstallerErrorCode = 0
        }
    }

    It 'Install by Id' {
        $result = Install-WinGetPackage -Id AppInstallerTest.TestExeInstaller -Version '1.0.0.0'
        Validate-WinGetPackageOperationResult $result $expectedExeInstallerResult 'install'
    }

    It 'Install by exact Name and Version' {
        $result = Install-WinGetPackage -Name TestPortableExe -Version '2.0.0.0' -MatchOption Equals
        Validate-WinGetPackageOperationResult $result $expectedPortableInstallerResult 'install'
    }

    It 'Update by Id' {
        $result = Update-WinGetPackage -Id AppInstallerTest.TestExeInstaller
        Validate-WinGetPackageOperationResult $result $expectedExeInstallerResult 'update'
    }

    It 'Update by Name' {
        $result = Update-WinGetPackage -Name TestPortableExe
        Validate-WinGetPackageOperationResult $result $expectedPortableInstallerResult 'update'
    }

    It 'Uninstall by Id' {
        $result = Uninstall-WinGetPackage -Id AppInstallerTest.TestExeInstaller
        Validate-WinGetPackageOperationResult $result $expectedExeInstallerResult 'uninstall'
    }

    It 'Uninstall by Name' {
        $result = Uninstall-WinGetPackage -Name TestPortableExe
        Validate-WinGetPackageOperationResult $result $expectedPortableInstallerResult 'uninstall'
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

Describe 'Install|Repair|Uninstall-WinGetPackage' {

    BeforeAll {
        AddTestSource
    }

    Context 'MSIX Repair Scenario' {
        BeforeEach {
            $expectedResult = [PSCustomObject]@{
                Id = "AppInstallerTest.TestMsixInstaller"
                Name = "TestMsixInstaller"
                Source = "TestSource"
                Status = 'Ok'
                RebootRequired = 'False'
                InstallerErrorCode = 0
                RepairErrorCode = 0
                UninstallerErrorCode = 0
            }
        }

        It 'Install MSIX By Id' {
            $result = Install-WinGetPackage -Id AppInstallerTest.TestMsixInstaller
            Validate-WinGetPackageOperationResult $result $expectedResult 'install'
        }

        It 'Repair MSIX By Id' {
            $result = Repair-WinGetPackage -Id AppInstallerTest.TestMsixInstaller
            Validate-WinGetPackageOperationResult $result $expectedResult 'repair'
        }

        It 'Uninstall MSIX By Id' {
            $result = Uninstall-WinGetPackage -Id AppInstallerTest.TestMsixInstaller
            Validate-WinGetPackageOperationResult $result $expectedResult 'uninstall'
        }
    }

    Context 'Burn installer "Modify" Repair Scenario' {
        BeforeEach {
            $expectedResult = [PSCustomObject]@{
                Id = "AppInstallerTest.TestModifyRepair"
                Name = "TestModifyRepair"
                Source = "TestSource"
                Status = 'Ok'
                RebootRequired = 'False'
                InstallerErrorCode = 0
                RepairErrorCode = 0
                UninstallerErrorCode = 0
            }
        }

        It 'Install Burn Installer By Id' {
            $result = Install-WinGetPackage -Id AppInstallerTest.TestModifyRepair
            Validate-WinGetPackageOperationResult $result $expectedResult 'install'
        }

        It 'Repair Burn Installer By Id' {
            $result = Repair-WinGetPackage -Id AppInstallerTest.TestModifyRepair
            Validate-WinGetPackageOperationResult $result $expectedResult 'repair'
        }

        It 'Uninstall Burn Installer By Id' {
            $result = Uninstall-WinGetPackage -Id AppInstallerTest.TestModifyRepair
            Validate-WinGetPackageOperationResult $result $expectedResult 'uninstall'
        }
    }

    Context 'Exe Installer "Uninstaller" Repair Scenario' {
        BeforeEach {
            $expectedResult = [PSCustomObject]@{
                Id = "AppInstallerTest.UninstallerRepair"
                Name = "UninstallerRepair"
                Source = "TestSource"
                Status = 'Ok'
                RebootRequired = 'False'
                InstallerErrorCode = 0
                RepairErrorCode = 0
                UninstallerErrorCode = 0
            }
        }

        It 'Install Exe Installer By Id' {
            $result = Install-WinGetPackage -Id AppInstallerTest.UninstallerRepair
            Validate-WinGetPackageOperationResult $result $expectedResult 'install'
        }

        It 'Uninstaller Repair Exe Installer By Id' {
            $result = Repair-WinGetPackage -Id AppInstallerTest.UninstallerRepair
            Validate-WinGetPackageOperationResult $result $expectedResult 'repair'
        }

        It "Uninstall Exe Installer By Id" {
            $result = Uninstall-WinGetPackage -Id AppInstallerTest.UninstallerRepair
            Validate-WinGetPackageOperationResult $result $expectedResult 'uninstall'
        }
    }

    Context 'Inno "Installer" Repair Scenario' {
        BeforeEach {
            $expectedResult = [PSCustomObject]@{
                Id = "AppInstallerTest.TestInstallerRepair"
                Name = "TestInstallerRepair"
                Source = "TestSource"
                Status = 'Ok'
                RebootRequired = 'False'
                InstallerErrorCode = 0
                RepairErrorCode = 0
                UninstallerErrorCode = 0
            }
        }

        It 'Install Exe Installer By Id' {
            $result = Install-WinGetPackage -Id AppInstallerTest.TestInstallerRepair
            Validate-WinGetPackageOperationResult $result $expectedResult 'install'
        }

        It 'Installer Repair Exe Installer By Id' {
            $result = Repair-WinGetPackage -Id AppInstallerTest.TestInstallerRepair
            Validate-WinGetPackageOperationResult $result $expectedResult 'repair'
        }

        It "Uninstall Exe Installer By Id" {
            $result = Uninstall-WinGetPackage -Id AppInstallerTest.TestInstallerRepair
            Validate-WinGetPackageOperationResult $result $expectedResult 'uninstall'
        }
    }

    AfterAll {
        # Uninstall all test packages after each  for proper cleanup.
        $testMsix = Get-WinGetPackage -Id AppInstallerTest.TestMsixInstaller
        if ($testMsix.Count -gt 0)
        {
            Uninstall-WinGetPackage -Id AppInstallerTest.TestMsixInstaller
        }

        $testBurn = Get-WinGetPackage -Id AppInstallerTest.TestModifyRepair
        if ($testBurn.Count -gt 0)
        {
            Uninstall-WinGetPackage -Id AppInstallerTest.TestModifyRepair
        }

        $testExe = Get-WinGetPackage -Id AppInstallerTest.UninstallerRepair
        if ($testExe.Count -gt 0)
        {
            Uninstall-WinGetPackage -Id AppInstallerTest.UninstallerRepair
        }

        $testInno = Get-WinGetPackage -Id AppInstallerTest.TestInstallerRepair
        if ($testInno.Count -gt 0)
        {
            Uninstall-WinGetPackage -Id AppInstallerTest.TestInstallerRepair
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
        $result.Id | Should -Be "AppInstallerTest.TestExeInstaller"
        $result.Name | Should -Be "TestExeInstaller"
        $result.Source | Should -Be "TestSource"
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

Describe 'Export-WinGetPackage' {

    BeforeAll {
        AddTestSource
    }

    It 'Download by Id' {
        $testDirectory = GetRandomTestDirectory
        $result = Export-WinGetPackage -Id AppInstallerTest.TestExeInstaller -Version '1.0.0.0' -DownloadDirectory $testDirectory
        
        $result | Should -Not -BeNullOrEmpty
        $result.Id | Should -Be "AppInstallerTest.TestExeInstaller"
        $result.Name | Should -Be "TestExeInstaller"
        $result.Source | Should -Be "TestSource"
        $result.Status | Should -Be 'Ok'

        # Download directory should be created and have exactly two files (installer and manifest file).
        Test-Path -Path $testDirectory | Should -Be $true
        (Get-ChildItem -Path $testDirectory -Force | Measure-Object).Count | Should -Be 2
    }

    It 'Download by Locale' {
        $testDirectory = GetRandomTestDirectory
        $result = Export-WinGetPackage -Id AppInstallerTest.TestMultipleInstallers -Locale 'zh-CN' -DownloadDirectory $testDirectory

        $result | Should -Not -BeNullOrEmpty
        $result.Id | Should -Be "AppInstallerTest.TestMultipleInstallers"
        $result.Name | Should -Be "TestMultipleInstallers"
        $result.Source | Should -Be "TestSource"
        $result.Status | Should -Be 'Ok'

        Test-Path -Path $testDirectory | Should -Be $true
        (Get-ChildItem -Path $testDirectory -Force | Measure-Object).Count | Should -Be 2
    }

    It 'Download by InstallerType' {
        $testDirectory = GetRandomTestDirectory
        $result = Export-WinGetPackage -Id AppInstallerTest.TestMultipleInstallers -InstallerType 'msi' -DownloadDirectory $testDirectory

        $result | Should -Not -BeNullOrEmpty
        $result.Id | Should -Be "AppInstallerTest.TestMultipleInstallers"
        $result.Name | Should -Be "TestMultipleInstallers"
        $result.Source | Should -Be "TestSource"
        $result.Status | Should -Be 'Ok'

        Test-Path -Path $testDirectory | Should -Be $true
        (Get-ChildItem -Path $testDirectory -Force | Measure-Object).Count | Should -Be 2
    }

    It 'Download by InstallerType that does not exist' {
        $testDirectory = GetRandomTestDirectory
        $result = Export-WinGetPackage -Id AppInstallerTest.TestExeInstaller -Version '1.0.0.0' -InstallerType 'zip' -DownloadDirectory $testDirectory

        $result | Should -Not -BeNullOrEmpty
        $result.Id | Should -Be "AppInstallerTest.TestExeInstaller"
        $result.Name | Should -Be "TestExeInstaller"
        $result.Source | Should -Be "TestSource"
        $result.Status | Should -Be 'NoApplicableInstallers'
        $result.ExtendedErrorCode | Should -Not -BeNullOrEmpty
        Test-Path -Path $testDirectory | Should -Be $false
    }

    AfterEach {
        if (Test-Path $testDirectory) {
            Remove-Item $testDirectory -Force -Recurse
        }
    }
}

Describe 'Get-WinGetUserSetting' {

    It 'Get setting' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $userSettings = Get-WinGetUserSetting
        $userSettings | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $userSettings.Count | Should -Be 2
        $userSettings.visual.progressBar | Should -Be 'rainbow'
        $userSettings.experimentalFeatures.experimentalArg | Should -Be $false
        $userSettings.experimentalFeatures.experimentalCmd | Should -Be $true
    }

    It 'Get settings. Bad json file' {
        Set-Content -Path $settingsFilePath -Value "Hi, im not a json. Thank you, Test."
        { Get-WinGetUserSetting } | Should -Throw
    }
}

Describe 'Test-WinGetUserSetting' {

    It 'Bad json file' {
        Set-Content -Path $settingsFilePath -Value "Hi, im not a json. Thank you, Test."

        $inputSettings = @{ visual= @{ progressBar="retro"} }
        Test-WinGetUserSetting -UserSettings $inputSettings | Should -Be $false
    }

    It 'Equal' {
        $ogSettings = @{ visual= @{ progressBar="retro"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        Test-WinGetUserSetting -UserSettings $ogSettings | Should -Be $true
    }

    It 'Equal. Ignore schema' {
        Set-Content -Path $settingsFilePath -Value '{ "$schema": "https://aka.ms/winget-settings.schema.json", "visual": { "progressBar": "retro" } }'

        $inputSettings = @{ visual= @{ progressBar="retro"} }
        Test-WinGetUserSetting -UserSettings $inputSettings | Should -Be $true
    }

    It 'Not Equal string' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="retro"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSetting -UserSettings $inputSettings | Should -Be $false
    }

    It 'Not Equal bool' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$true ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSetting -UserSettings $inputSettings | Should -Be $false
    }

    It 'Not Equal. More settings' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false }}
        Test-WinGetUserSetting -UserSettings $inputSettings | Should -Be $false
    }

    It 'Not Equal. More settings input' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSetting -UserSettings $inputSettings | Should -Be $false
    }

    It 'Equal IgnoreNotSet' {
        $ogSettings = @{ visual= @{ progressBar="retro"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        Test-WinGetUserSetting -UserSettings $ogSettings -IgnoreNotSet | Should -Be $true
    }

    It 'Equal IgnoreNotSet. More settings' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false }}
        Test-WinGetUserSetting -UserSettings $inputSettings -IgnoreNotSet | Should -Be $true
    }

    It 'Not Equal IgnoreNotSet. More settings input' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSetting -UserSettings $inputSettings -IgnoreNotSet | Should -Be $false
    }

    It 'Not Equal bool IgnoreNotSet' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$true ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$false ; experimentalCmd=$true}}
        Test-WinGetUserSetting -UserSettings $inputSettings -IgnoreNotSet | Should -Be $false
    }

    It 'Not Equal array IgnoreNotSet' {
        $ogSettings = @{ installBehavior= @{ preferences= @{ architectures = @("x86", "x64")} }}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ installBehavior= @{ preferences= @{ architectures = @("x86", "arm64")} }}
        Test-WinGetUserSetting -UserSettings $inputSettings -IgnoreNotSet | Should -Be $false
    }

    It 'Not Equal wrong type IgnoreNotSet' {
        $ogSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$true ; experimentalCmd=$true}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=4 ; experimentalCmd=$true}}
        Test-WinGetUserSetting -UserSettings $inputSettings -IgnoreNotSet | Should -Be $false
    }
    

    AfterAll {
        SetWinGetSettingsHelper @{ debugging= @{ enableSelfInitiatedMinidump=$true ; keepAllLogFiles=$true } }
    }
}

Describe 'Set-WinGetUserSetting' {

    It 'Overwrites' {
        $ogSettings = @{ source= @{ autoUpdateIntervalInMinutes=3}}
        SetWinGetSettingsHelper $ogSettings

        $inputSettings = @{ visual= @{ progressBar="rainbow"} ; experimentalFeatures= @{experimentalArg=$true ; experimentalCmd=$false}}
        $result = Set-WinGetUserSetting -UserSettings $inputSettings

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
        $result = Set-WinGetUserSetting -UserSettings $inputSettings -Merge

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
        $result = Set-WinGetUserSetting -UserSettings $inputSettings

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.'$schema' | Should -Not -BeNullOrEmpty 
        $result.visual.progressBar | Should -Be "retro"
    }

    It 'Overwrites Bad json file' {
        Set-Content -Path $settingsFilePath -Value "Hi, im not a json. Thank you, Test."

        $inputSettings = @{ visual= @{ progressBar="retro"} }
        $result = Set-WinGetUserSetting -UserSettings $inputSettings

        $result | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $result.'$schema' | Should -Not -BeNullOrEmpty 
        $result.visual.progressBar | Should -Be "retro"
    }

    It 'Overwrites Bad json file' {
        Set-Content -Path $settingsFilePath -Value "Hi, im not a json. Thank you, Test."

        $inputSettings = @{ visual= @{ progressBar="retro"} }
        { Set-WinGetUserSetting -UserSettings $inputSettings -Merge } | Should -Throw
    }

    AfterAll {
        SetWinGetSettingsHelper @{ debugging= @{ enableSelfInitiatedMinidump=$true ; keepAllLogFiles=$true } }
    }
}

Describe 'Get|Enable|Disable-WinGetSetting' {

    It 'Get-WinGetSetting' {
        $settings = Get-WinGetSetting
        $settings | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $settings.'$schema' | Should -Not -BeNullOrEmpty 
        $settings.adminSettings | Should -Not -BeNullOrEmpty
        $settings.userSettingsFile | Should -Be $settingsFilePath
    }

    # This tests require admin
    It 'Enable|Disable' {
        $settings = Get-WinGetSetting
        $settings | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $settings.adminSettings | Should -Not -BeNullOrEmpty
        $settings.adminSettings.LocalManifestFiles | Should -Be $false

        Enable-WinGetSetting -Name LocalManifestFiles

        $afterEnable = Get-WinGetSetting
        $afterEnable | Should -Not -BeNullOrEmpty -ErrorAction Stop
        $afterEnable.adminSettings | Should -Not -BeNullOrEmpty
        $afterEnable.adminSettings.LocalManifestFiles | Should -Be $true

        Disable-WingetSetting -Name LocalManifestFiles

        $afterDisable = Get-WinGetSetting
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

    It "Disable WinGetPolicy and run Get-WinGetVersion" {
        $policyKeyValueName =  "EnableAppInstaller"

        Set-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName -Value 0
        $registryKey  =  Get-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName
        $registryKey | Should -Not -BeNullOrEmpty
        $registryKey.EnableAppInstaller | Should -Be 0

        { Get-WinGetVersion } | Should -Throw "This operation is disabled by Group Policy : Enable Windows Package Manager"

        CleanupGroupPolicies
    }

    It "Disable EnableWindowsPackageManagerCommandLineInterfaces Policy and run Get-WinGetVersion" {
       $policyKeyValueName =  "EnableWindowsPackageManagerCommandLineInterfaces"

        Set-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName -Value 0
        $registryKey  =  Get-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName
        $registryKey | Should -Not -BeNullOrEmpty
        $registryKey.EnableWindowsPackageManagerCommandLineInterfaces | Should -Be 0

        { Get-WinGetVersion } | Should -Throw "This operation is disabled by Group Policy : Enable Windows Package Manager command line interfaces"

        CleanupGroupPolicies
    }

    AfterAll {
        CleanupGroupPolicies
        CleanupGroupPolicyKeyIfExists
    }
}

Describe 'WindowsPackageManagerServer' -Skip:($PSEdition -eq "Desktop") {

    BeforeEach {
        AddTestSource
        WaitForWindowsPackageManagerServer $true
    }

    # When WindowsPackageManagerServer dies, we should not fail.
    It 'Forced termination' {
        $source = Get-WinGetSource -Name 'TestSource'
        $source | Should -Not -BeNullOrEmpty
        $source.Name | Should -Be 'TestSource'

        $process = Get-Process -Name "WindowsPackageManagerServer"
        $process | Should -Not -BeNullOrEmpty

        # At least one is running.
        $process | Where-Object { $_.HasExited -eq $false } | Should -Not -BeNullOrEmpty

        WaitForWindowsPackageManagerServer $true

        # From the ones we got, at least one exited
        $process | Where-Object { $_.HasExited -eq $true } | Should -Not -BeNullOrEmpty

        $source2 = Get-WinGetSource -Name 'TestSource'
        $source2 | Should -Not -BeNullOrEmpty
        $source2.Name | Should -Be 'TestSource'

        $process2 = Get-Process -Name "WindowsPackageManagerServer"
        $process2 | Should -Not -BeNullOrEmpty
        $process2.Id | Should -Not -Be $process.Id
    }

    # The Microsoft.WinGet.Client has static proxy objects of WindowsPackageManagerServer
    # This tests does all the Microsoft.WinGet.Client calls in a different pwsh instance.
    It 'Graceful termination' {
        $typeTable = [System.Management.Automation.Runspaces.TypeTable]::LoadDefaultTypeFiles()
        $oopRunspace = [System.Management.Automation.Runspaces.RunspaceFactory]::CreateOutOfProcessRunspace($typeTable)
        $oopRunspace.Open()
        $oopPwsh = [PowerShell]::Create()
        $oopPwsh.Runspace = $oopRunspace
        $oopPwshPid = $oopPwsh.AddScript("`$PID").Invoke()
        $oopPwshProcess = Get-Process -Id $oopPwshPid
        $oopPwshProcess.HasExited | Should -Be $false

        $source = $oopPwsh.AddScript("Get-WinGetSource -Name TestSource").Invoke()
        $source | Should -Not -BeNullOrEmpty
        $source.Name | Should -Be 'TestSource'

        $wingetProcess = Get-Process -Name "WindowsPackageManagerServer"
        $wingetProcess | Should -Not -BeNullOrEmpty

        # At least one is running.
        $wingetProcess | Where-Object { $_.HasExited -eq $false } | Should -Not -BeNullOrEmpty

        $oopRunspace.Close()

        Start-Sleep -Seconds 30
        $oopPwshProcess.HasExited | Should -Be $true

        # From the ones we got, at least one exited
        WaitForWindowsPackageManagerServer
        $wingetProcess | Where-Object { $_.HasExited -eq $true } | Should -Not -BeNullOrEmpty
    }
}

AfterAll {
    RemoveTestSource
}
