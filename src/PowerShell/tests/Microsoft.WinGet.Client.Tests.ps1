# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Pester tests related to the Microsoft.WinGet.Client PowerShell module. The tests require the
   localhostwebserver to be running. 'Invoke-Pester' should be called in an admin powershell window with the
   PowerShell module already imported. 'Import-Module <Path to .psd1 file>'
#>

BeforeAll {
    wingetdev source add 'TestSource' 'https://localhost:5001/TestKit/'
}

Describe 'Get-WinGetSource' {
    
    It 'Get Test Source' {
        $source = Get-WinGetSource -Name TestSource
        $source.Name | Should -Be 'TestSource'
        $source.Argument | Should -Be 'https://localhost:5001/TestKit/'
        $source.Type | Should -Be 'Microsoft.PreIndexed.Package'
    }
}

Describe 'Find-WinGetPackage' {
    It 'Given no parameters, lists all available packages' {
        $allPackages = Find-WinGetPackage -Source TestSource
        $allPackages.Count | Should -BeGreaterThan 0
    }

    It 'Find by Id' {
        $package = Find-WinGetPackage -Source 'TestSource' -Id 'AppInstallerTest.TestExeInstaller'
        $package.Name | Should -Be 'TestExeInstaller'
        $package.Id | Should -Be 'AppInstallerTest.TestExeInstaller'
        $package.Version | Should -Be '2.0.0.0'
        $package.Source | Should -Be 'TestSource'
    }

    It 'Find by Name' {
        $package = Find-WinGetPackage -Source 'TestSource' -Name 'TestPortableExe'
        $package[0].Name | Should -Be 'TestPortableExe'
        $package[1].Name | Should -Be 'TestPortableExeWithCommand'
    }

    It 'Find by Name sort by Version' {
        $package = Find-WinGetPackage -Source 'TestSource' -Name 'TestPortableExe' | Sort-Object 'Version'
        $package[0].Name | Should -Be 'TestPortableExeWithCommand'
        $package[1].Name | Should -Be 'TestPortableExe'
    }

    It 'Find package and verify PackageVersionInfo' {
        $package = Find-WinGetPackage -Source 'TestSource' -Id 'AppInstallerTest.TestPortableExe' -Exact   
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

    It 'Install by Id' {
        $result = Install-WinGetPackage -Id AppInstallerTest.TestExeInstaller -Version '1.0.0.0'
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Install by exact Name and Version' {
        $result = Install-WinGetPackage -Name TestPortableExe -Version '2.0.0.0' -Exact
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Update by Id' {
        $result = Update-WinGetPackage -Id AppInstallerTest.TestExeInstaller
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Update by Name and Version' {
        $result = Update-WinGetPackage -Name TestPortableExe
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Uninstall by Id' {
        $result = Uninstall-WinGetPackage -Id AppInstallerTest.TestExeInstaller
        $result.UninstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Uninstall by Name' {
        $result = Uninstall-WinGetPackage -Name TestPortableExe
        $result.UninstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    AfterAll {
        # Uninstall all test packages after each  for proper cleanup.
        $testExe = Get-WinGetPackage -Id AppInstallerTest.TestExeInstaller
        if ($testExe.Count -gt 0)
        {
            Uninstall-WinGetPackage -Id AppInstallerTest.TestExeInstaller 
        } 

        $testPortable = Get-WinGetPackage -Id AppInstallerTest.TestPortableExe
        if ($testPortable.Count -gt 0)
        {
            Uninstall-WinGetPackage -Id AppInstallerTest.TestPortableExe
        }
   }
}

Describe 'Get-WinGetPackage' {
    
    It 'Install by Id' {
        $result = Install-WinGetPackage -Id AppInstallerTest.TestExeInstaller -Version '1.0.0.0'
        $result.InstallerErrorCode | Should -Be 0
        $result.Status | Should -Be 'Ok'
        $result.RebootRequired | Should -Be 'False'
    }

    It 'Get package by Id' {
        $result = Get-WinGetPackage -Id AppInstallerTest.TestExeInstaller
        $result.Name | Should -Be 'TestExeInstaller'
        $result.Id | Should -Be 'AppInstallerTest.TestExeInstaller'
        $result.Version | Should -Be '1.0.0.0'
        $result.AvailableVersions[0] | Should -Be '2.0.0.0'
        $result.Source | Should -Be 'TestSource'
    }

    It 'Get package by Name' {
        $result = Get-WinGetPackage -Name TestExeInstaller
        $result.Name | Should -Be 'TestExeInstaller'
        $result.Id | Should -Be 'AppInstallerTest.TestExeInstaller'
        $result.Version | Should -Be '1.0.0.0'
        $result.AvailableVersions[0] | Should -Be '2.0.0.0'
        $result.Source | Should -Be 'TestSource'
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

AfterAll {
     # Removal of source requires administrator.
     wingetdev source remove 'TestSource'   
}