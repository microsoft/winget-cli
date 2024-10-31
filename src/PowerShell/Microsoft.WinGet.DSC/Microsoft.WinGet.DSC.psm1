# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

using namespace System.Collections.Generic

# Check that we are running as an administrator
function Assert-IsAdministrator
{
    $windowsIdentity = [System.Security.Principal.WindowsIdentity]::GetCurrent()
    $windowsPrincipal = New-Object -TypeName 'System.Security.Principal.WindowsPrincipal' -ArgumentList @( $windowsIdentity )

    $adminRole = [System.Security.Principal.WindowsBuiltInRole]::Administrator

    if (-not $windowsPrincipal.IsInRole($adminRole))
    {
        New-InvalidOperationException -Message "This resource must run as an Administrator."
    }
}

#region enums
enum WinGetAction
{
    Partial
    Full
}

enum WinGetEnsure
{
    Absent
    Present
}

enum WinGetMatchOption
{
    Equals
    EqualsCaseInsensitive
    StartsWithCaseInsensitive
    ContainsCaseInsensitive
}

enum WinGetInstallMode
{
    Default
    Silent
    Interactive
}

enum WinGetTrustLevel
{
    Undefined
    None
    Trusted
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
        $userSettings = Get-WinGetUserSetting
        $result = @{
            SID = ''
            Settings = $userSettings
        }
        return $result
    }

    # Tests if desired properties match.
    [bool] Test()
    {
        $hashArgs = @{
            UserSettings = $this.Settings
        }

        if ($this.Action -eq [WinGetAction]::Partial)
        {
            $hashArgs.Add('IgnoreNotSet', $true)
        }

        return Test-WinGetUserSetting @hashArgs
    }

    # Sets the desired properties.
    [void] Set()
    {
        $hashArgs = @{
            UserSettings = $this.Settings
        }

        if ($this.Action -eq [WinGetAction]::Partial)
        {
            $hashArgs.Add('Merge', $true)
        }

        Set-WinGetUserSetting @hashArgs
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
        $settingsJson = Get-WinGetSetting
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
    [string]$Name
    
    [DscProperty(Mandatory)]
    [string]$Argument
    
    [DscProperty()]
    [string]$Type
    
    [DscProperty()]
    [WinGetTrustLevel]$TrustLevel = [WinGetTrustLevel]::Undefined
    
    [DscProperty()]
    [nullable[bool]]$Explicit = $null

    [DscProperty()]
    [WinGetEnsure]$Ensure = [WinGetEnsure]::Present

    [WinGetSource] Get()
    {
        if ([String]::IsNullOrWhiteSpace($this.Name))
        {
            throw "A value must be provided for WinGetSource::Name"
        }

        $currentSource = $null

        try {
            $currentSource = Get-WinGetSource -Name $this.Name
        }
        catch {
        }

        $result = [WinGetSource]::new()

        if ($currentSource)
        {
            $result.Ensure = [WinGetEnsure]::Present
            $result.Name = $currentSource.Name
            $result.Argument = $currentSource.Argument
            $result.Type = $currentSource.Type
            $result.TrustLevel = $currentSource.TrustLevel
            $result.Explicit = $currentSource.Explicit
        }
        else
        {
            $result.Ensure = [WinGetEnsure]::Absent
            $result.Name = $this.Name
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

        $currentSource = $this.Get()

        $removeSource = $false
        $resetSource = $false
        $addSource = $false

        if ($this.Ensure -eq [WinGetEnsure]::Present)
        {
            if ($currentSource.Ensure -eq [WinGetEnsure]::Present)
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
            if ($currentSource.Ensure -eq [WinGetEnsure]::Present)
            {
                $removeSource = $true
            }
            # else in desired state (Absent)
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
            $hashArgs = @{
                Name = $this.Name
                Argument = $this.Argument
            }

            if (-not [string]::IsNullOrWhiteSpace($this.Type))
            {
                $hashArgs.Add("Type", $this.Type)
            }

            if ($this.TrustLevel -ne [WinGetTrustLevel]::Undefined)
            {
                $hashArgs.Add("TrustLevel", $this.TrustLevel)
            }

            if ($null -ne $this.Explicit)
            {
                $hashArgs.Add("Explicit", $this.Explicit)
            }

            Add-WinGetSource @hashArgs
        }
    }
    
    # Test $this against a value retrieved from Get
    # We don't need to check Name because it is the Key for Get
    [bool] hidden TestAgainstCurrent([WinGetSource]$currentSource)
    {
        if ($this.Ensure -eq [WinGetEnsure]::Absent -and
            $currentSource.Ensure -eq [WinGetEnsure]::Absent)
        {
            return $true
        }

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

        if ($this.TrustLevel -ne [WinGetTrustLevel]::Undefined -and
            $this.TrustLevel -ne $currentSource.TrustLevel)
        {
            return $false
        }

        if ($null -ne $this.Explicit -and
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
    [string]$Id

    [DscProperty(Key)]
    [string]$Source

    [DscProperty()]
    [string]$Version

    [DscProperty()]
    [WinGetEnsure]$Ensure = [WinGetEnsure]::Present

    [DscProperty()]
    [WinGetMatchOption]$MatchOption = [WinGetMatchOption]::EqualsCaseInsensitive

    [DscProperty()]
    [bool]$UseLatest = $false

    [DSCProperty()]
    [WinGetInstallMode]$InstallMode = [WinGetInstallMode]::Silent

    [PSObject] hidden $CatalogPackage = $null

    [WinGetPackage] Get()
    {
        if ([String]::IsNullOrWhiteSpace($this.Id))
        {
            throw "A value must be provided for WinGetPackage::Id"
        }

        $result = [WinGetPackage]::new()

        $hashArgs = @{
            Id = $this.Id
            MatchOption = $this.MatchOption
        }

        if (-not([string]::IsNullOrWhiteSpace($this.Source)))
        {
            $hashArgs.Add("Source", $this.Source)
        }

        $result.CatalogPackage = Get-WinGetPackage @hashArgs
        if ($null -ne $result.CatalogPackage)
        {
            $result.Ensure = [WinGetEnsure]::Present
            $result.Id = $result.CatalogPackage.Id
            $result.Source = $result.CatalogPackage.Source
            $result.Version = $result.CatalogPackage.InstalledVersion
            $result.UseLatest = -not $result.CatalogPackage.IsUpdateAvailable
        }
        else
        {
            $result.Ensure = [WinGetEnsure]::Absent
            $result.Id = $this.Id
            $result.MatchOption = $this.MatchOption
            $result.Source = $this.Source
        }

        return $result
    }

    [bool] Test()
    {
        return $this.TestAgainstCurrent($this.Get())
    }

    [void] Set()
    {
        $currentPackage = $this.Get()

        if (-not $this.TestAgainstCurrent($currentPackage))
        {
            $hashArgs = @{
                Id = $this.Id
                MatchOption = $this.MatchOption
                Mode = $this.InstallMode
            }
            
            if ($this.Ensure -eq [WinGetEnsure]::Present)
            {
                if (-not([string]::IsNullOrWhiteSpace($this.Source)))
                {
                    $hashArgs.Add("Source", $this.Source)
                }

                if ($currentPackage.Ensure -eq [WinGetEnsure]::Present)
                {
                    if ($this.UseLatest)
                    {
                        $this.TryUpdate($hashArgs)
                    }
                    elseif (-not([string]::IsNullOrWhiteSpace($this.Version)))
                    {
                        $hashArgs.Add("Version", $this.Version)

                        $compareResult = $currentPackage.CatalogPackage.CompareToVersion($this.Version)
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
    
    [bool] hidden TestAgainstCurrent([WinGetPackage]$currentPackage)
    {
        if ($this.Ensure -eq [WinGetEnsure]::Absent -and
            $currentPackage.Ensure -eq [WinGetEnsure]::Absent)
        {
            return $true
        }

        $this.CatalogPackage = $currentPackage.CatalogPackage

        if ($this.Ensure -ne $currentPackage.Ensure)
        {
            return $false
        }

        # At this point we know is installed.
        # If asked for latest, but there are updates available.
        if ($this.UseLatest)
        {
            if (-not $currentPackage.UseLatest)
            {
                return $false
            }
        }
        # If there is an specific version, compare with the current installed version.
        elseif (-not ([string]::IsNullOrWhiteSpace($this.Version)))
        {
            $compareResult = $currentPackage.CatalogPackage.CompareToVersion($this.Version)
            if ($compareResult -ne 'Equal')
            {
                return $false
            }
        }

        return $true
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
