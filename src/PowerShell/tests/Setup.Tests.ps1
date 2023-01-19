# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Setup file for all Pester tests related to the Microsoft.WinGet.Client PowerShell module.
#>

BeforeAll {
    wingetdev source add 'TestSource' 'https://localhost:5001/TestKit/'
}

AfterAll {
    # Uninstall all test packages for proper cleanup.
    Uninstall-WinGetPackage -Id AppInstallerTest.TestExeInstaller
    Uninstall-WinGetPackage -Id AppInstallerTest.TestPortableExe

    # Removal of source requires administrator.
    wingetdev source remove 'TestSource'
}