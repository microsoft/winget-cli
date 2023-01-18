# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Tests for the Find-WinGetPackage cmdlet in the Microsoft.WinGet.Client PowerShell module.
#>

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
}