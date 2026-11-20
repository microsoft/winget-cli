# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

param([switch]$TargetProduction)

Describe 'Assert-WinGetPackageManager' -Skip:(-not $TargetProduction -or [Security.Principal.WindowsIdentity]::GetCurrent().IsSystem) {
    BeforeAll {
        Import-Module Microsoft.WinGet.Client
    }

    It 'Reports <Expected> using only the Store-signed package version' -ForEach @(
        @{ StoreVersion = $null; Expected = 'AppInstallerNotInstalled' }
        @{ StoreVersion = '1.0.0.0'; Expected = 'AppInstallerNotSupported' }
        @{ StoreVersion = '1.31.0.0'; Expected = 'AppInstallerNotRegistered' }
    ) {
        Mock Get-AppxPackage {
            [pscustomobject]@{ PublisherId = '8wekyb3d8bbwe'; SignatureKind = 'Developer'; Version = '1.31.0.0' }
            if ($StoreVersion) {
                [pscustomobject]@{ PublisherId = '8wekyb3d8bbwe'; SignatureKind = 'Store'; Version = $StoreVersion }
            }
        } -ParameterFilter { $Name -eq 'Microsoft.DesktopAppInstaller' }

        $originalLocalAppData = $env:LOCALAPPDATA
        try {
            $env:LOCALAPPDATA = $TestDrive
            $failure = { Assert-WinGetPackageManager -ErrorAction Stop } | Should -Throw -PassThru
            $failure.Exception.Category | Should -Be $Expected
        }
        finally {
            $env:LOCALAPPDATA = $originalLocalAppData
        }
    }
}
