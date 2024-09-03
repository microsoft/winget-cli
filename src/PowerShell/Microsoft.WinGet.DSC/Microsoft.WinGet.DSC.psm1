# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

using namespace System.Collections.Generic

try
{
    # Load all non-test .ps1 files in the script's directory.
    Get-ChildItem -Path $PSScriptRoot\* -Filter *.ps1 -Exclude *.Tests.ps1 -Recurse | ForEach-Object { . $_.FullName }
} catch
{
    $e = $_.Exception
    while ($e.InnerException)
    {
        $e = $e.InnerException
    }

    if (-not [string]::IsNullOrWhiteSpace($e.Message))
    {
        Write-Host $e.Message -ForegroundColor Red -BackgroundColor Black
    }
}

#region enums
enum WinGetAction
{
    Partial
    Full
}

enum Ensure
{
    Absent
    Present
}

enum MatchOption
{
    Equals
    EqualsCaseInsensitive
    StartsWithCaseInsensitive
    ContainsCaseInsensitive
}

enum InstallMode
{
    Default
    Silent
    Interactive
}

enum TrustLevel
{
    Undefined
    None
    Trusted
}

enum OptionalBool
{
    Undefined
    False
    True
}

#endregion enums

#region DscResources
# Author here all DSC Resources.
# DSC Powershell doesn't support binary DSC resources without the MOF schema.
# DSC Powershell classes aren't discoverable if placed outside of the psm1.

# This resource is in charge of managing the settings.json file of winget.
[DSCResource()]
class WinGetUserSettings
{
    # We need a key. Do not set.
    [DscProperty(Key)]
    [string]$SID

    # A hash table with the desired settings.
    [DscProperty(Mandatory)]
    [Hashtable]$Settings

    [DscProperty()]
    [WinGetAction]$Action = [WinGetAction]::Full

    # Gets the current UserSettings by looking at the settings.json file for the current user.
    [WinGetUserSettings] Get()
    {
        Assert-WinGetCommand "Get-WinGetUserSettings"

        $userSettings = Get-WinGetUserSettings
        $result = @{
            SID = ''
            Settings = $userSettings
        }
        return $result
    }

    # Tests if desired properties match.
    [bool] Test()
    {
        Assert-WinGetCommand "Test-WinGetUserSettings"

        $hashArgs = @{
            UserSettings = $this.Settings
        }

        if ($this.Action -eq [WinGetAction]::Partial)
        {
            $hashArgs.Add('IgnoreNotSet', $true)
        }

        return Test-WinGetUserSettings @hashArgs
    }

    # Sets the desired properties.
    [void] Set()
    {
        Assert-WinGetCommand "Set-WinGetUserSettings"

        $hashArgs = @{
            UserSettings = $this.Settings
        }

        if ($this.Action -eq [WinGetAction]::Partial)
        {
            $hashArgs.Add('Merge', $true)
        }

        Set-WinGetUserSettings @hashArgs
    }
}

# Handles configuration of administrator settings.
[DSCResource()]
class WinGetAdminSettings
{
    # We need a key. Do not set.
    [DscProperty(Key)]
    [string]$SID

    # A hash table with the desired admin settings.
    [DscProperty(Mandatory)]
    [Hashtable]$Settings

    # Gets the administrator settings.
    [WinGetAdminSettings] Get()
    {
        Assert-WinGetCommand "Get-WinGetSettings"
        $settingsJson = Get-WinGetSettings
        # Get admin setting values.

        $result = @{
            SID = ''
            Settings = $settingsJson.adminSettings
        }
        return $result
    }

    # Tests if administrator settings given are set as expected.
    # This doesn't do a full comparison to allow users to don't have to update
    # their resource every time a new admin setting is added on winget.
    [bool] Test()
    {
        $adminSettings = $this.Get().Settings
        foreach ($adminSetting in $adminSettings.GetEnumerator())
        {
            if ($this.Settings.ContainsKey($adminSetting.Name))
            {
                if ($this.Settings[$adminSetting.Name] -ne $adminSetting.Value)
                {
                    return $false
                }
            }
        }

        return $true
    }

    # Sets the desired properties.
    [void] Set()
    {
        Assert-IsAdministrator
        Assert-WinGetCommand "Enable-WinGetSetting"
        Assert-WinGetCommand "Disable-WinGetSetting"

        # It might be better to implement an internal Test with one value, or
        # create a new instances with only one setting than calling Enable/Disable
        # for all of them even if only one is different.
        if (-not $this.Test())
        {
            foreach ($adminSetting in $this.Settings.GetEnumerator())
            {
                if ($adminSetting.Value)
                {
                    Enable-WinGetSetting -Name $adminSetting.Name
                }
                else
                {
                    Disable-WinGetSetting -Name $adminSetting.Name
                }
            }
        }
    }
}

[DSCResource()]
class WinGetSource
{
    [DscProperty(Key, Mandatory)]
    [ValidateNotNullOrWhiteSpace()]
    [string]$Name
    
    [DscProperty(Mandatory)]
    [ValidateNotNullOrWhiteSpace()]
    [string]$Argument
    
    [DscProperty()]
    [string]$Type
    
    [DscProperty()]
    [TrustLevel]$TrustLevel = [TrustLevel]::Undefined
    
    [DscProperty()]
    [OptionalBool]$Explicit = [OptionalBool]::Undefined

    [DscProperty()]
    [Ensure]$Ensure = [Ensure]::Present

    [WinGetSource] Get()
    {
        Assert-WinGetCommand "Get-WinGetSource"
        $currentSource = Get-WinGetSource -Name $this.Name
        $result = [WinGetSource]::new()

        if ($currentSource)
        {
            $result.Ensure = [Ensure]::Present
            $result.Name = $currentSource.Name
            $result.Argument = $currentSource.Argument
            $result.Type = $currentSource.Type
            $result.TrustLevel = $currentSource.TrustLevel
            $result.Explicit = $currentSource.Explicit
        }
        else
        {
            $result.Ensure = [Ensure]::Absent
        }

        return $result
    }

    [bool] Test()
    {
        return $this.TestAgainstCurrent($this.Get())
    }

    [void] Set()
    {
        Assert-IsAdministrator
        Assert-WinGetCommand "Add-WinGetSource"
        Assert-WinGetCommand "Remove-WinGetSource"
        Assert-WinGetCommand "Reset-WinGetSource"

        $currentSource = $this.Get()

        $removeSource = $false
        $resetSource = $false
        $addSource = $false

        if ($this.Ensure -eq [Ensure]::Present)
        {
            if ($currentSource.Ensure -eq [Ensure]::Present)
            {
                if (-not $this.TestAgainstCurrent($currentSource))
                {
                    $resetSource = $true
                    $addSource = $true
                }
                # else in desired state
            }
            else
            {
                $addSource = $true
            }
        }
        else
        {
            if ($currentSource.Ensure -eq [Ensure]::Present)
            {
                $removeSource = $true
            }
            else
            {
                # In desired state of Absent
            }
        }

        if ($removeSource)
        {
            Remove-WinGetSource -Name $this.Name
        }
        # Only remove OR reset should be true, not both
        elseif ($resetSource)
        {
            Reset-WinGetSource -Name $this.Name
        }

        if ($addSource)
        {
            $hashArgs = @
            {
                Name = $this.Name
                Argument = $this.Argument
            }

            if (-not [string]::IsNullOrWhiteSpace($this.Type))
            {
                $hashArgs.Add("Type", $this.Type)
            }

            if ($this.TrustLevel -ne [TrustLevel]::Undefined)
            {
                $hashArgs.Add("TrustLevel", $this.TrustLevel)
            }

            if ($this.Explicit -ne [OptionalBool]::Undefined)
            {
                $hashArgs.Add("Explicit", $this.Explicit)
            }

            Add-WinGetSource @hashArgs
        }
    }
    
    hidden [bool] TestAgainstCurrent([WinGetSource]$currentSource)
    {
        if ($this.Ensure -ne $currentSource.Ensure -or
            $this.Argument -ne $currentSource.Argument)
        {
            return $false
        }

        if (-not([string]::IsNullOrWhiteSpace($this.Type)) -and
            $this.Type -ne $currentSource.Type)
        {
            return $false
        }

        if ($this.TrustLevel -ne [TrustLevel]::Undefined -and
            $this.TrustLevel -ne $currentSource.TrustLevel)
        {
            return $false
        }

        if ($this.Explicit -ne [OptionalBool]::Undefined -and
            $this.Explicit -ne $currentSource.Explicit)
        {
            return $false
        }

        return $true
    }
}

# TODO: It would be nice if these resource has a non configurable property that has extra information that comes from
# GitHub. We could implement it here or add more cmdlets in Microsoft.WinGet.Client.
[DSCResource()]
class WinGetPackageManager
{
    # We need a key. Do not set.
    [DscProperty(Key)]
    [string]$SID

    [DscProperty()]
    [string]$Version = ""

    [DscProperty()]
    [bool]$UseLatest

    [DscProperty()]
    [bool]$UseLatestPreRelease

    # If winget is not installed the version will be empty.
    [WinGetPackageManager] Get()
    {
        $integrityResource = [WinGetPackageManager]::new()
        if ($integrityResource.Test())
        {
            $integrityResource.Version = Get-WinGetVersion
        }

        return $integrityResource
    }

    # Tests winget is installed.
    [bool] Test()
    {
        Assert-WinGetCommand "Assert-WinGetPackageManager"
        Assert-WinGetCommand "Get-WinGetVersion"

        try
        {
            $hashArgs = @{}

            if ($this.UseLatest)
            {
                $hashArgs.Add("Latest", $true)
            } elseif ($this.UseLatestPreRelease)
            {
                $hashArgs.Add("Latest", $true)
                $hashArgs.Add("IncludePrerelease", $true)
            } elseif (-not [string]::IsNullOrWhiteSpace($this.Version))
            {
                $hashArgs.Add("Version", $this.Version)
            }

            Assert-WinGetPackageManager @hashArgs
        }
        catch
        {
            return $false
        }

        return $true
    }

    # Repairs Winget.
    [void] Set()
    {
        Assert-WinGetCommand "Repair-WinGetPackageManager"

        if (-not $this.Test())
        {
            $result = -1
            $hashArgs = @{}

            if ($this.UseLatest)
            {
                $hashArgs.Add("Latest", $true)
            } elseif ($this.UseLatestPreRelease)
            {
                $hashArgs.Add("Latest", $true)
                $hashArgs.Add("IncludePrerelease", $true)
            } elseif (-not [string]::IsNullOrWhiteSpace($this.Version))
            {
                $hashArgs.Add("Version", $this.Version)
            }

            $result = Repair-WinGetPackageManager @hashArgs

            if ($result -ne 0)
            {
                # TODO: Localize.
                throw "Failed to repair winget. Result $result"
            }
        }
    }
}

[DSCResource()]
class WinGetPackage
{
    [DscProperty(Key, Mandatory)]
    [ValidateNotNullOrWhiteSpace()]
    [string]$Id

    [DscProperty(Key)]
    [string]$Source

    [DscProperty()]
    [string]$Version

    [DscProperty()]
    [Ensure]$Ensure = [Ensure]::Present

    [DscProperty()]
    [MatchOption]$MatchOption = [MatchOption]::EqualsCaseInsensitive

    [DscProperty()]
    [bool]$UseLatest = $false

    [DSCProperty()]
    [InstallMode]$InstallMode = [InstallMode]::Silent

    [DscProperty(NotConfigurable)]
    [string]$InstalledVersion

    [DscProperty(NotConfigurable)]
    [bool]$IsInstalled = $false

    [DscProperty(NotConfigurable)]
    [bool]$IsUpdateAvailable = $false

    [PSObject] hidden $CatalogPackage = $null

    hidden Initialize()
    {
        # DSC only validates keys and mandatories in a Set call.
        if ([string]::IsNullOrWhiteSpace($this.Id))
        {
            # TODO: Localize.
            throw "WinGetPackage: Id is required"
        }

        if (($this.UseLatest -eq $true) -and (-not[string]::IsNullOrWhiteSpace($this.Version)))
        {
            # TODO: Localize.
            throw "WinGetPackage: Version and UseLatest cannot be set at the same time"
        }

        # This has to use MatchOption equals. Otherwise, it might find other package where the
        # id starts with.
        $hashArgs = @{
            Id = $this.Id
            MatchOption = $this.MatchOption
        }

        if (-not([string]::IsNullOrWhiteSpace($this.Source)))
        {
            $hashArgs.Add("Source", $this.Source)
        }

        $this.CatalogPackage = Get-WinGetPackage @hashArgs
        if ($null -ne $this.CatalogPackage)
        {
            $this.InstalledVersion = $this.CatalogPackage.InstalledVersion
            $this.IsInstalled = $true
            $this.IsUpdateAvailable = $this.CatalogPackage.IsUpdateAvailable
        }
    }

    # Get.
    [WinGetPackage] Get()
    {
        Assert-WinGetCommand "Get-WinGetPackage"
        $this.Initialize()
        return $this
    }

    # Test.
    [bool] Test()
    {
        $this.Initialize()
        $ensureInstalled = $this.Ensure -eq [Ensure]::Present

        # Not installed, doesn't have to.
        if (-not($this.IsInstalled -or $ensureInstalled))
        {
            return $true
        }

        # Not install, need to ensure installed.
        # Installed, need to ensure not installed.
        if ($this.IsInstalled -ne $ensureInstalled)
        {
            return $false
        }

        # At this point we know is installed.
        # If asked for latests, but there are updates available.
        if ($this.UseLatest -and
            $this.IsUpdateAvailable)
        {
            return $false
        }

        # If there is an specific version, compare with the current installed version.
        if (-not ([string]::IsNullOrWhiteSpace($this.Version)))
        {
            $compareResult = $this.CatalogPackage.CompareToVersion($this.Version)
            if ($compareResult -ne 'Equal')
            {
                return $false
            }
        }

        # For now this is all.
        return $true
    }

    # Set.
    [void] Set()
    {
        Assert-WinGetCommand "Install-WinGetPackage"
        Assert-WinGetCommand "Uninstall-WinGetPackage"

        if (-not $this.Test())
        {
            $hashArgs = @{
                Id = $this.Id
                MatchOption = $this.MatchOption
                Mode = $this.InstallMode
            }
            
            if ($this.Ensure -eq [Ensure]::Present)
            {
                if (-not([string]::IsNullOrWhiteSpace($this.Source)))
                {
                    $hashArgs.Add("Source", $this.Source)
                }

                if ($this.IsInstalled)
                {
                    if ($this.UseLatest)
                    {
                        $this.TryUpdate($hashArgs)
                    }
                    elseif (-not([string]::IsNullOrWhiteSpace($this.Version)))
                    {
                        $hashArgs.Add("Version", $this.Version)

                        $compareResult = $this.CatalogPackage.CompareToVersion($this.Version)
                        switch ($compareResult)
                        {
                            'Lesser'
                            {
                                $this.TryUpdate($hashArgs)
                                break
                            }
                            {'Greater' -or 'Unknown'}
                            {
                                # The installed package has a greater version or unknown. Uninstall and install.
                                $this.Uninstall()
                                $this.Install($hashArgs)
                                break
                            }
                        }
                    }
                }
                else
                {
                    if (-not([string]::IsNullOrWhiteSpace($this.Version)))
                    {
                        $hashArgs.Add("Version", $this.Version)
                    }

                    $this.Install($hashArgs)
                }
            }
            else
            {
                $this.Uninstall()
            }
        }
    }

    hidden Install([Hashtable]$hashArgs)
    {
        $installResult = Install-WinGetPackage @hashArgs
        if (-not $installResult.Succeeded())
        {
            # TODO: Localize.
            throw "WinGetPackage Failed installing $($this.Id). $($installResult.ErrorMessage())"
        }
    }

    hidden Uninstall()
    {
        $uninstallResult = Uninstall-WinGetPackage -PSCatalogPackage $this.CatalogPackage
        if (-not $uninstallResult.Succeeded())
        {
            # TODO: Localize.
            throw "WinGetPackage Failed uninstalling $($this.Id). $($uninstallResult.ErrorMessage())"
        }
    }

    hidden Update([Hashtable]$hashArgs)
    {
        $updateResult = Update-WinGetPackage @hashArgs
        if (-not $updateResult.Succeeded())
        {
            # TODO: Localize.
            throw "WinGetPackage Failed updating $($this.Id). $($updateResult.ErrorMessage())"
        }
    }

    # Tries to update, if not, uninstall and install.
    hidden TryUpdate([Hashtable]$hashArgs)
    {
        try
        {
            $this.Update($hashArgs)
        }
        catch
        {
            $this.Uninstall()
            $this.Install($hashArgs)
        }
    }
}

#endregion DscResources
