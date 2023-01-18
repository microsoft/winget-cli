# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Tests for the Get-WinGetPackage cmdlet in the Microsoft.WinGet.Client PowerShell module.
#>

Describe 'Install-WinGetPackage' {
    
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
}