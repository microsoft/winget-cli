# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Tests for the Install-WinGetPackage cmdlet in the Microsoft.WinGet.Client PowerShell module.
#>

Describe 'Install-WinGetPackage' {

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
}

Describe 'Update-WinGetPackage' {

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
}

Describe 'Uninstall-WinGetPackage' {
    
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
}