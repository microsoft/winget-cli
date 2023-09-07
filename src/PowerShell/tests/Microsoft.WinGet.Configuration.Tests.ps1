# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Pester tests related to the Microsoft.WinGet.Configuration PowerShell module.
   'Invoke-Pester' should be called in an admin PowerShell window.
   Requires local test repo to be setup.
#>

BeforeAll {
    $env:POWERSHELL_TELEMETRY_OPTOUT = "true"
    $deviceGroupPolicyRoot = "HKLM:\Software\Policies\Microsoft\Windows"
    $wingetPolicyKeyName = "AppInstaller"
    $wingetGroupPolicyRegistryRoot = $deviceGroupPolicyRoot + "\" + $wingetPolicyKeyName
    $e2eTestModule = "xE2ETestResource"

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

    function GetConfigTestDataPath()
    {
        return Join-Path $PSScriptRoot "..\..\AppInstallerCLIE2ETests\TestData\Configuration\"
    }

    function DeleteConfigTxtFiles()
    {
        Get-ChildItem $(GetConfigTestDataPath) -Filter Configure*.txt -Recurse | ForEach-Object { Remove-Item $_ }
    }

    function GetConfigTestDataFile([string] $fileName)
    {
        $path = Join-Path $(GetConfigTestDataPath) $fileName

        if (-not (Test-Path $path))
        {
            throw "$path does not exists"
        }

        return $path
    }

    enum TestModuleLocation
    {
        CurrentUser
        AllUsers
        Custom
        DefaultLocation
    }

    function GetExpectedModulePath([TestModuleLocation]$testModuleLocation)
    {
        switch ($testModuleLocation)
        {
            ([TestModuleLocation]::CurrentUser)
            {
                $path = [Environment]::GetFolderPath([Environment+SpecialFolder]::MyDocuments)
                return Join-Path $path "PowerShell\Modules"
            }
            ([TestModuleLocation]::AllUsers)
            {
                $path = [Environment]::GetFolderPath([Environment+SpecialFolder]::ProgramFiles)
                return Join-Path $path "PowerShell\Modules"
            }
            ([TestModuleLocation]::DefaultLocation)
            {
                $path = [Environment]::GetFolderPath([Environment+SpecialFolder]::LocalApplicationData)
                return Join-Path $path "Microsoft\WinGet\Configuration\Modules"
            }
            ([TestModuleLocation]::Custom)
            {
                return Join-Path $env:TEMP "E2EPesterCustomModules"
            }
            default
            {
                throw $testModuleLocation
            }
        }
    }

    function CleanupPsModulePath()
    {
        $wingetPath = GetExpectedModulePath DefaultLocation
        $customPath = GetExpectedModulePath Custom
        $modulePath = $env:PsModulePath
        $newModulePath = ($modulePath.Split(';') | Where-Object { $_ -ne $wingetPath } | Where-Object { $_ -ne $customPath }) -join ';'
        $env:PsModulePath = $newModulePath
    }

    function EnsureModuleState([string]$moduleName, [bool]$present, [string]$repository = $null, [TestModuleLocation]$testModuleLocation = [TestModuleLocation]::CurrentUser)
    {
        CleanupPsModulePath
        $wingetPath = GetExpectedModulePath DefaultLocation
        $customPath = GetExpectedModulePath Custom
        $env:PsModulePath += ";$wingetPath;$customPath"

        $availableModules = Get-Module $moduleName -ListAvailable
        $isPresent = $null -ne $availableModules

        if ($isPresent)
        {
            foreach ($module in $availableModules)
            {
                $item = Get-Item $module.Path
                while ($item.Name -ne $moduleName)
                {
                    $item = Get-Item $item.PSParentPath
                }

                if (-not $present)
                {
                    Get-ChildItem $item.FullName -Recurse | Remove-Item -Force -Recurse
                    Remove-Item $item
                }
                else
                {
                    # Must be in the right location
                    $expected = GetExpectedModulePath $testModuleLocation
                    if ($expected -ne $item.Parent.FullName)
                    {
                        Get-ChildItem $item.FullName -Recurse | Remove-Item -Force -Recurse
                        Remove-Item $item
                        $isPresent = $false
                    }
                }
            }
        }

        if ((-not $isPresent) -and $present)
        {
            $params = @{
                Name = $moduleName
                Force = $true
            }

            if (-not [string]::IsNullOrEmpty($repository))
            {
                $params.Add('Repository', $repository)
            }

            if (($testModuleLocation -eq [TestModuleLocation]::CurrentUser) -or
                ($testModuleLocation -eq [TestModuleLocation]::AllUsers))
            {
                if ($testModuleLocation -eq [TestModuleLocation]::AllUsers)
                {
                    $params.Add('Scope', 'AllUsers')
                }

                Install-Module @params
            }
            else
            {
                $path = $customPath
                if (($testModuleLocation -eq [TestModuleLocation]::WinGetModulePath) -or
                    ($testModuleLocation -eq [TestModuleLocation]::DefaultLocation))
                {
                    $path = $wingetPath
                }
                $params.Add('Path', $path)

                Save-Module @params
            }
        }

        CleanupPsModulePath
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

        # [NOTE:] We don't need a valid yml file path to test Group Policy blocking scenario as it is the earliest check, 
        # so just using some random file path for this test.
        { Get-WinGetConfiguration -File "Z:\NonExisting_SettingsFile.yml" } | Should -Throw "This operation is disabled by Group Policy : Enable Windows Package Manager"

        CleanupGroupPolicies
    }

    It "Disable EnableWindowsPackageManagerCommandLineInterfaces Policy and run Get-WinGetConfiguration" {
       $policyKeyValueName =  "EnableWindowsPackageManagerCommandLineInterfaces"

        Set-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName -Value 0
        $registryKey  =  Get-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName
        $registryKey | Should -Not -BeNullOrEmpty
        $registryKey.EnableWindowsPackageManagerCommandLineInterfaces | Should -Be 0

        # [NOTE:] We don't  need a valid yml file path to test Group Policy blocking scenario as it is the earliest check, 
        # so just using some random file path for this test.
        { Get-WinGetConfiguration -File "Z:\NonExisting_SettingsFile.yml" } | Should -Throw "This operation is disabled by Group Policy : Enable Windows Package Manager command line interfaces"

        CleanupGroupPolicies
    }

    It "Disable EnableWindowsPackageManagerConfiguration Policy and run Get-WinGetConfiguration" {
       $policyKeyValueName =  "EnableWindowsPackageManagerConfiguration"

        Set-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName -Value 0
        $registryKey  =  Get-ItemProperty -Path $wingetGroupPolicyRegistryRoot -Name $policyKeyValueName
        $registryKey | Should -Not -BeNullOrEmpty
        $registryKey.EnableWindowsPackageManagerConfiguration | Should -Be 0

        # [NOTE:] We don't  need a valid yml file path to test Group Policy blocking scenario as it is the earliest check, 
        # so just using some random file path for this test.
        { Get-WinGetConfiguration -File "Z:\NonExisting_SettingsFile.yml" } | Should -Throw "This operation is disabled by Group Policy : Enable Windows Package Manager Configuration"

        CleanupGroupPolicies
    }

    AfterAll {
        CleanupGroupPolicies
        CleanupGroupPolicyKeyIfExists
    }
}

Describe 'Get configuration' {

    It 'Get configuration and details' {
        EnsureModuleState $e2eTestModule $false

        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $set = Get-WinGetConfigurationDetails -Set $set
        $set | Should -Not -BeNullOrEmpty
    }

    It 'Get details piped' {
        EnsureModuleState $e2eTestModule $false

        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile | Get-WinGetConfigurationDetails
        $set | Should -Not -BeNullOrEmpty
    }

    It 'File doesnt exit' {
        $testFile = "c:\dir\fakefile.txt"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw $testFile
    }
}

Describe 'Invoke winget configuration' {

    BeforeAll {
        DeleteConfigTxtFiles
    }

    It 'From Gallery' {
        EnsureModuleState "XmlContentDsc" $false

        $testFile = GetConfigTestDataFile "PSGallery_NoModule_NoSettings.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        { Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set } | Should -Throw "One or more errors occurred. (Some of the configuration was not applied successfully.)"
    }

    It 'From TestRepo' {
        EnsureModuleState $e2eTestModule $false

        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $set = Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $set | Should -Not -BeNullOrEmpty

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"

        $expectedModule = Join-Path $(GetExpectedModulePath DefaultLocation) $e2eTestModule
        Test-Path $expectedModule | Should -Be $true
    }

    It 'From TestRepo piped' {
        DeleteConfigTxtFiles
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile | Invoke-WinGetConfiguration -AcceptConfigurationAgreements
        $set | Should -Not -BeNullOrEmpty

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }

    It 'From TestRepo Location' -ForEach @(
        @{ Location = "CurrentUser"; }
        @{ Location = "AllUsers"; }
        @{ Location = "DefaultLocation"; }
        @{ Location = "Custom"; }) {
        $modulePath = "'"
        switch ($location)
        {
            ([TestModuleLocation]::CurrentUser)
            {
                $modulePath = "currentuser"
                break
            }
            ([TestModuleLocation]::AllUsers)
            {
                $modulePath = "allusers"
                break
            }
            ([TestModuleLocation]::DefaultLocation)
            {
                $modulePath = "default"
                break
            }
            ([TestModuleLocation]::Custom)
            {
                $modulePath = GetExpectedModulePath Custom
                break
            }
            default {
                throw $location
            }
        }
    
        EnsureModuleState $e2eTestModule $false

        $testFile = GetConfigTestDataFile "Configure_TestRepo_Location.yml"
        $set = Get-WinGetConfiguration -File $testFile -ModulePath $modulePath
        $set | Should -Not -BeNullOrEmpty

        $set = Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $set | Should -Not -BeNullOrEmpty

        $expectedModule = Join-Path $(GetExpectedModulePath $location) $e2eTestModule
        Test-Path $expectedModule | Should -Be $true
    }

    It 'Independent Resource - One Failure' {
        $testFile = GetConfigTestDataFile "IndependentResources_OneFailure.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        { Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set } | Should -Throw "One or more errors occurred. (Some of the configuration was not applied successfully.)"

        $expectedFile = Join-Path $(GetConfigTestDataPath) "IndependentResources_OneFailure.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }

    It 'Dependent Resource - Failure' {
        $testFile = GetConfigTestDataFile "DependentResources_Failure.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        { Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set } | Should -Throw "One or more errors occurred. (Some of the configuration was not applied successfully.)"

        $expectedFile = Join-Path $(GetConfigTestDataPath) "DependentResources_Failure.txt"
        Test-Path $expectedFile | Should -Be $false
    }
}

Describe 'Start and complete configuration' {

    BeforeAll {
        DeleteConfigTxtFiles
    }

    It 'From TestRepo' {
        DeleteConfigTxtFiles
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $job = Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $job | Should -Not -BeNullOrEmpty

        $set = Complete-WinGetConfiguration -ConfigurationJob $job
        $set | Should -Not -BeNullOrEmpty

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"

        { Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set } | Should -Throw "Operation is not valid due to the current state of the object."
    }

    It 'From TestRepo piped' {
        DeleteConfigTxtFiles
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile | Start-WinGetConfiguration -AcceptConfigurationAgreements | Complete-WinGetConfiguration
        $set | Should -Not -BeNullOrEmpty

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }

    It 'Dependent Resource - Failure' {
        $testFile = GetConfigTestDataFile "DependentResources_Failure.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        # This should not throw
        $job = Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $job | Should -Not -BeNullOrEmpty

        { Complete-WinGetConfiguration -ConfigurationJob $job } | Should -Throw "One or more errors occurred. (Some of the configuration was not applied successfully.)"

        $expectedFile = Join-Path $(GetConfigTestDataPath) "DependentResources_Failure.txt"
        Test-Path $expectedFile | Should -Be $false
    }
}

AfterAll {
    CleanupGroupPolicies
    CleanupGroupPolicyKeyIfExists
    CleanupPsModulePath
    DeleteConfigTxtFiles
}