# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Pester tests related to the Microsoft.WinGet.Configuration PowerShell module.
   'Invoke-Pester' should be called in an admin PowerShell window.
#>

BeforeAll {
    $deviceGroupPolicyRoot = "HKLM:\Software\Policies\Microsoft\Windows"
    $wingetPolicyKeyName = "AppInstaller"
    $wingetGroupPolicyRegistryRoot = $deviceGroupPolicyRoot + "\" + $wingetPolicyKeyName

    Import-Module Microsoft.WinGet.Configuration

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

Describe 'Test-GroupPolicies' {
    BeforeAll {
        CleanupGroupPolicies
        CreatePolicyKeyIfNotExists
    }

    It "Disable WinGetPolicy and run Get-WinGetConfiguration" {

        $policyKeyValueName =  "EnableAppInstaller"

        Set-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName -Value 0
        $registryKey  =  Get-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName
        $registryKey | Should -Not -BeNullOrEmpty
        $registryKey.EnableAppInstaller | Should -Be 0

        # [NOTE:] We don't need a valid yml file path to test Group Policy blocking scenario as it is the earliest check, so just using some random non existing path for this test.
        { Get-WinGetConfiguration -File "Z:\NonExisting_SettingsFile.yml" } | Should -Throw "This operation is disabled by Group Policy : Enable Windows Package Manager"

        CleanupGroupPolicies
    }

    It "Disable EnableWindowsPackageManagerCommandLineInterfaces Policy and run Get-WinGetConfiguration" {
       $policyKeyValueName =  "EnableWindowsPackageManagerCommandLineInterfaces"

        Set-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName -Value 0
        $registryKey  =  Get-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName
        $registryKey | Should -Not -BeNullOrEmpty
        $registryKey.EnableWindowsPackageManagerCommandLineInterfaces | Should -Be 0

        # [NOTE:] We don't  need a valid yml file path to test Group Policy blocking scenario as it is the earliest check, so just using some random non existing path for this test.
        { Get-WinGetConfiguration -File "Z:\NonExisting_SettingsFile.yml" } | Should -Throw "This operation is disabled by Group Policy : Enable Windows Package Manager command line interfaces"

        CleanupGroupPolicies
    }

    AfterAll {
        CleanupGroupPolicies
        CleanupGroupPolicyKeyIfExists
    }
}

AfterAll {
    CleanupGroupPolicies
    CleanupGroupPolicyKeyIfExists
}