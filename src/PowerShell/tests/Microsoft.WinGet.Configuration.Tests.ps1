# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Pester tests related to the Microsoft.WinGet.Configuration PowerShell module.
   'Invoke-Pester' should be called in an admin PowerShell window.
   Requires local test repo to be setup.
#>
[CmdletBinding()]
param(
    # The location of the test data
    [string]$ConfigurationTestDataPath
)

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

    if ([System.String]::IsNullOrEmpty($ConfigurationTestDataPath))
    {
        $ConfigurationTestDataPath = (Join-Path $PSScriptRoot "..\..\AppInstallerCLIE2ETests\TestData\Configuration\")
    }

    function GetConfigTestDataPath()
    {
        return $ConfigurationTestDataPath
    }

    function DeleteConfigTxtFiles()
    {
        Get-ChildItem $(GetConfigTestDataPath) -Filter *.txt -Recurse | ForEach-Object { Remove-Item $_ }
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
                try
                {
                    $item = Get-Item $module.Path -ErrorAction Stop
                    while ($item.Name -ne $moduleName)
                    {
                        $item = Get-Item $item.PSParentPath -ErrorAction Stop
                    }

                    if (-not $present)
                    {
                        Get-ChildItem $item.FullName -Recurse | Remove-Item -Force -Recurse -ErrorAction Stop
                        Remove-Item $item -ErrorAction Stop
                    }
                    else
                    {
                        # Must be in the right location
                        $expected = GetExpectedModulePath $testModuleLocation
                        if ($expected -ne $item.Parent.FullName)
                        {
                            Get-ChildItem $item.FullName -Recurse | Remove-Item -Force -Recurse -ErrorAction Stop
                            Remove-Item $item -ErrorAction Stop
                            $isPresent = $false
                        }
                    }
                }
                catch [System.Management.Automation.ItemNotFoundException]
                {
                    Write-Host "Item not found, ignoring..." $_.Exception.Message
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

    It 'Get configuration and details positional' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration $testFile
        $set | Should -Not -BeNullOrEmpty

        $set = Get-WinGetConfigurationDetails $set
        $set | Should -Not -BeNullOrEmpty
    }

    It 'File doesnt exit' {
        $testFile = "c:\dir\fakeFile.txt"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw $testFile
    }

    It 'Invalid file' {
        $testFile = GetConfigTestDataFile "Empty.yml"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw "*0x8A15C002*"
    }

    It 'Missing property' {
        $testFile = GetConfigTestDataFile "NotConfig.yml"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw '*0x8A15C00E*$schema*missing*'
    }

    It 'Missing configurationVersion' {
        $testFile = GetConfigTestDataFile "NoVersion.yml"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw "*0x8A15C00E*configurationVersion*missing*"
    }

    It 'Unknown version' {
        $testFile = GetConfigTestDataFile "UnknownVersion.yml"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw "*0x8A15C004*Configuration file version*is not known.*"
    }

    It 'Resource wrong type' {
        $testFile = GetConfigTestDataFile "ResourcesNotASequence.yml"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw "*0x8A15C003*resources*wrong type*"
    }

    It 'Unit wrong type' {
        $testFile = GetConfigTestDataFile "UnitNotAMap.yml"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw "*0x8A15C003*resources*0*wrong type*"
    }

    It 'No resource name' {
        $testFile = GetConfigTestDataFile "NoResourceName.yml"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw "*0x8A15C00D*resource*invalid value*Module/*"
    }

    It 'Module mismatch' {
        $testFile = GetConfigTestDataFile "ModuleMismatch.yml"
        { Get-WinGetConfiguration -File $testFile } | Should -Throw "*0x8A15C00D*invalid value*DifferentModule*"
    }
}

Describe 'Invoke-WinGetConfiguration' {

    BeforeEach {
        DeleteConfigTxtFiles
    }
    
<# PS Gallery tests are unreliable.
    It 'From Gallery' {
        EnsureModuleState "XmlContentDsc" $false

        $testFile = GetConfigTestDataFile "PSGallery_NoModule_NoSettings.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286075
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be -1978285819
    }
#>

    It 'From TestRepo' {
        EnsureModuleState $e2eTestModule $false

        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"

        $expectedModule = Join-Path $(GetExpectedModulePath DefaultLocation) $e2eTestModule
        Test-Path $expectedModule | Should -Be $true
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

        $result = Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedModule = Join-Path $(GetExpectedModulePath $location) $e2eTestModule
        Test-Path $expectedModule | Should -Be $true
    }

    It 'Piped' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $result = Get-WinGetConfiguration -File $testFile | Invoke-WinGetConfiguration -AcceptConfigurationAgreements
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }

    It 'Positional' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Invoke-WinGetConfiguration -AcceptConfigurationAgreements $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }

    It 'Independent Resource - One Failure' {
        $testFile = GetConfigTestDataFile "IndependentResources_OneFailure.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286075
        $result.UnitResults.Count | Should -Be 2
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be -1978285819
        $result.UnitResults[1].State | Should -Be "Completed"
        $result.UnitResults[1].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "IndependentResources_OneFailure.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }

    It 'Dependent Resource - Failure' {
        $testFile = GetConfigTestDataFile "DependentResources_Failure.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286075
        $result.UnitResults.Count | Should -Be 2
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be -1978285819
        $result.UnitResults[1].State | Should -Be "Skipped"
        $result.UnitResults[1].ResultCode | Should -Be -1978286072

        $expectedFile = Join-Path $(GetConfigTestDataPath) "DependentResources_Failure.txt"
        Test-Path $expectedFile | Should -Be $false
    }

    It 'ResourceCaseInsensitive' {
        $testFile = GetConfigTestDataFile "ResourceCaseInsensitive.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "ResourceCaseInsensitive.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }
}

Describe 'Start|Complete-WinGetConfiguration' {

    BeforeEach {
        DeleteConfigTxtFiles
    }
    
<# PS Gallery tests are unreliable.
    It 'From Gallery' {
        EnsureModuleState "XmlContentDsc" $false

        $testFile = GetConfigTestDataFile "PSGallery_NoModule_NoSettings.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $job = Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $job | Should -Not -BeNullOrEmpty

        $result = Complete-WinGetConfiguration -ConfigurationJob $job
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286075
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be -1978285819
    }
#>

    It 'From TestRepo' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $job = Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $job | Should -Not -BeNullOrEmpty

        $result = Complete-WinGetConfiguration -ConfigurationJob $job
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"

        # Verify can't be used after.
        { Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set } | Should -Throw "Operation is not valid due to the current state of the object."
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

        $job = Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $job | Should -Not -BeNullOrEmpty

        $result = Complete-WinGetConfiguration -ConfigurationJob $job
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedModule = Join-Path $(GetExpectedModulePath $location) $e2eTestModule
        Test-Path $expectedModule | Should -Be $true
    }

    It 'Piped' {
        DeleteConfigTxtFiles
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $result = Get-WinGetConfiguration -File $testFile | Start-WinGetConfiguration -AcceptConfigurationAgreements | Complete-WinGetConfiguration
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }

    It 'Positional' {
        DeleteConfigTxtFiles
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration $testFile
        $set | Should -Not -BeNullOrEmpty

        $job = Start-WinGetConfiguration -AcceptConfigurationAgreements $set
        $job | Should -Not -BeNullOrEmpty

        $result = Complete-WinGetConfiguration $job
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }

    It 'Independent Resource - One Failure' {
        $testFile = GetConfigTestDataFile "IndependentResources_OneFailure.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $job = Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $job | Should -Not -BeNullOrEmpty

        $result = Complete-WinGetConfiguration -ConfigurationJob $job
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286075
        $result.UnitResults.Count | Should -Be 2
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be -1978285819

        $result.UnitResults[1].State | Should -Be "Completed"
        $result.UnitResults[1].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "IndependentResources_OneFailure.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }

    It 'Dependent Resource - Failure' {
        $testFile = GetConfigTestDataFile "DependentResources_Failure.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $job = Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $job | Should -Not -BeNullOrEmpty

        $result = Complete-WinGetConfiguration -ConfigurationJob $job
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286075
        $result.UnitResults.Count | Should -Be 2
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be -1978285819
        $result.UnitResults[1].State | Should -Be "Skipped"
        $result.UnitResults[1].ResultCode | Should -Be -1978286072

        $expectedFile = Join-Path $(GetConfigTestDataPath) "DependentResources_Failure.txt"
        Test-Path $expectedFile | Should -Be $false
    }

    It 'ResourceCaseInsensitive' {
        $testFile = GetConfigTestDataFile "ResourceCaseInsensitive.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $job = Start-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $job | Should -Not -BeNullOrEmpty

        $result = Complete-WinGetConfiguration -ConfigurationJob $job
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $expectedFile = Join-Path $(GetConfigTestDataPath) "ResourceCaseInsensitive.txt"
        Test-Path $expectedFile | Should -Be $true
        Get-Content $expectedFile -Raw | Should -Be "Contents!"
    }
}

Describe 'Test-WinGetConfiguration' {

    BeforeEach {
        DeleteConfigTxtFiles
    }

    It 'Negative' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Test-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.TestResult | Should -Be "Negative"
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].TestResult | Should -Be "Negative"
        $result.UnitResults[0].ResultCode | Should -Be 0
    }

    It 'Positive' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $expectedFile = Join-Path $(GetConfigTestDataPath) "Configure_TestRepo.txt"
        Set-Content -Path $expectedFile -Value "Contents!" -NoNewline

        $result = Test-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.TestResult | Should -Be "Positive"
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].TestResult | Should -Be "Positive"
        $result.UnitResults[0].ResultCode | Should -Be 0
    }

    It 'Piped' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $result = Get-WinGetConfiguration -File $testFile | Test-WinGetConfiguration -AcceptConfigurationAgreements
        $result | Should -Not -BeNullOrEmpty
        $result.TestResult | Should -Be "Negative"
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].TestResult | Should -Be "Negative"
        $result.UnitResults[0].ResultCode | Should -Be 0
    }

    It 'Positional' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Test-WinGetConfiguration -AcceptConfigurationAgreements $set
        $result | Should -Not -BeNullOrEmpty
        $result.TestResult | Should -Be "Negative"
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].TestResult | Should -Be "Negative"
        $result.UnitResults[0].ResultCode | Should -Be 0
    }

    It "Failed" {
        $testFile = GetConfigTestDataFile "IndependentResources_OneFailure.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Test-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.TestResult | Should -Be "Failed"
        $result.UnitResults.Count | Should -Be 2
        $result.UnitResults[0].TestResult | Should -Be "Failed"
        $result.UnitResults[0].ResultCode | Should -Be -1978285819
        $result.UnitResults[1].TestResult | Should -Be "Negative"
        $result.UnitResults[1].ResultCode | Should -Be 0
    }
}

Describe 'Confirm-WinGetConfiguration' {

    It 'Duplicate Identifiers' {
        $testFile = GetConfigTestDataFile "DuplicateIdentifiers.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Confirm-WinGetConfiguration -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286074
        $result.UnitResults.Count | Should -Be 3
        $result.UnitResults[0].ResultCode | Should -Be -1978286074
        $result.UnitResults[1].ResultCode | Should -Be -1978286074
        $result.UnitResults[2].ResultCode | Should -Be 0
    }

    It 'Missing dependency' {
        $testFile = GetConfigTestDataFile "MissingDependency.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Confirm-WinGetConfiguration -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286073
        $result.UnitResults.Count | Should -Be 3
        $result.UnitResults[0].ResultCode | Should -Be 0
        $result.UnitResults[1].ResultCode | Should -Be 0
        $result.UnitResults[2].ResultCode | Should -Be -1978286073
    }

    It 'Dependency cycle' {
        $testFile = GetConfigTestDataFile "DependencyCycle.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Confirm-WinGetConfiguration -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286068
        $result.UnitResults.Count | Should -Be 3
        $result.UnitResults[0].ResultCode | Should -Be -1978286072
        $result.UnitResults[1].ResultCode | Should -Be -1978286072
        $result.UnitResults[2].ResultCode | Should -Be 0
    }

    It 'No issue' {
        $testFile = GetConfigTestDataFile "PSGallery_NoSettings.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Confirm-WinGetConfiguration -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
    }

    It 'Piped' {
        $testFile = GetConfigTestDataFile "DuplicateIdentifiers.yml"
        $result = Get-WinGetConfiguration -File $testFile | Confirm-WinGetConfiguration
        $result.UnitResults.Count | Should -Be 3
        $result.UnitResults[0].ResultCode | Should -Be -1978286074
        $result.UnitResults[1].ResultCode | Should -Be -1978286074
        $result.UnitResults[2].ResultCode | Should -Be 0
    }

    It 'Positional' {
        $testFile = GetConfigTestDataFile "DuplicateIdentifiers.yml"
        $set = Get-WinGetConfiguration $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Confirm-WinGetConfiguration $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be -1978286074
        $result.UnitResults.Count | Should -Be 3
        $result.UnitResults[0].ResultCode | Should -Be -1978286074
        $result.UnitResults[1].ResultCode | Should -Be -1978286074
        $result.UnitResults[2].ResultCode | Should -Be 0
    }
}

Describe 'Configuration History' {

    BeforeEach {
        DeleteConfigTxtFiles
    }

    It 'History Lifecycle' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = Invoke-WinGetConfiguration -AcceptConfigurationAgreements -Set $set
        $result | Should -Not -BeNullOrEmpty
        $result.ResultCode | Should -Be 0
        $result.UnitResults.Count | Should -Be 1
        $result.UnitResults[0].State | Should -Be "Completed"
        $result.UnitResults[0].ResultCode | Should -Be 0

        $historySet = Get-WinGetConfiguration -InstanceIdentifier $set.InstanceIdentifier
        $historySet | Should -Not -BeNullOrEmpty
        $historySet.InstanceIdentifier | Should -Be $set.InstanceIdentifier

        $allHistory = Get-WinGetConfiguration -All
        $allHistory | Should -Not -BeNullOrEmpty

        $historySet | Remove-WinGetConfigurationHistory

        $historySetAfterRemove = Get-WinGetConfiguration -InstanceIdentifier $set.InstanceIdentifier
        $historySetAfterRemove | Should -BeNullOrEmpty
    }
}

Describe 'Configuration Serialization' {

    It 'Basic Serialization' {
        $testFile = GetConfigTestDataFile "Configure_TestRepo.yml"
        $set = Get-WinGetConfiguration -File $testFile
        $set | Should -Not -BeNullOrEmpty

        $result = ConvertTo-WinGetConfigurationYaml -Set $set
        $result | Should -Not -BeNullOrEmpty

        $tempFile = New-TemporaryFile
        Set-Content -Path $tempFile -Value $result

        $roundTripSet = Get-WinGetConfiguration -File $tempFile.VersionInfo.FileName
        $roundTripSet | Should -Not -BeNullOrEmpty
    }
}

AfterAll {
    CleanupGroupPolicies
    CleanupGroupPolicyKeyIfExists
    CleanupPsModulePath
    DeleteConfigTxtFiles
}
