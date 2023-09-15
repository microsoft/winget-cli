# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Pester tests related to the Microsoft.WinGet.Client PowerShell module.
   The tests require the localhost web server to be running and serving the test data.
   'Invoke-Pester' should be called in an admin PowerShell window.
#>

BeforeAll {
    $settingsFilePath = (ConvertFrom-Json (wingetdev.exe settings export)).userSettingsFile

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
            Add-WinGetSource -Name 'TestSource' -Arg 'https://localhost:5001/TestKit/'
        }
    }

    function RemoveTestSource {
        try {
            Get-WinGetSource -Name 'TestSource'
        }
        catch {
            # Source Remove requires admin privileges, this will only execute successfully in an elevated PowerShell.
            # This is a workaround to an issue where the server takes longer than expected to terminate when
            # running from PowerShell. This can cause other E2E tests to fail when attempting to reset the test source.
            Start-Process -FilePath "wingetdev" -ArgumentList "source remove TestSource"
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

    function KillWindowsPackageManagerServer()
    {
        $processes = Get-Process | Where-Object { $_.Name -eq "WindowsPackageManagerServer" }
        foreach ($p in $processes)
        {
            Stop-Process $p

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
}

Describe 'WindowsPackageManagerServer' {

    BeforeEach {
        KillWindowsPackageManagerServer
    }

    # When WindowsPackageManagerServer dies, we should not fail.
    It 'Forced termination' {
        $source = Get-WinGetSource -Name 'TestSource'
        $source | Should -Not -BeNullOrEmpty
        $source.Name | Should -Be 'TestSource'

        $process = Get-Process -Name "WindowsPackageManagerServer"
        $process | Should -Not -BeNullOrEmpty
        $process.HasExited | Should -Be $false

        KillWindowsPackageManagerServer

        $process.HasExited | Should -Be $true

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
        $oopRunspace = [System.Management.Automation.Runspaces.RunspaceFactory]::CreateOutOfProcessRunspace($typetable)
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
        $wingetProcess.HasExited | Should -Be $false

        $oopRunspace.Close()

        Start-Sleep -Seconds 30
        $oopPwshProcess.HasExited | Should -Be $true
        $wingetProcess.HasExited | Should -Be $true
    }
}

AfterAll {
    #RemoveTestSource
}