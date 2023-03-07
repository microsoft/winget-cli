# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

using namespace System.Collections.Generic

try
{
    # Load all non-test .ps1 files in the script's directory.
    Get-ChildItem -Path $PSScriptRoot\* -Filter *.ps1 -Exclude *.Tests.ps1 -Recurse | ForEach-Object { Import-Module $_.FullName }
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

        if ($this.Action -eq [WinGetAction]::Partial)
        {
            return Test-WinGetUserSettings -UserSettings $this.Settings -IgnoreNotSet
        }

        return Test-WinGetUserSettings -UserSettings $this.Settings
    }

    # Sets the desired properties.
    [void] Set()
    {
        Assert-WinGetCommand "Set-WinGetUserSettings"

        if ($this.Action -eq [WinGetAction]::Partial)
        {
            Set-WinGetUserSettings -UserSettings $this.Settings -Merge | Out-Null
        }
        else
        {
            Set-WinGetUserSettings -UserSettings $this.Settings | Out-Null
        }
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
        $settingsJson = Get-WinGetSettings | ConvertFrom-Json -AsHashtable
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
class WinGetSources
{
    # We need a key. Do not set.
    [DscProperty(Key)]
    [string]$SID

    # An array of Hashtable with the key value properties that follows the source's group policy schema.
    [DscProperty(Mandatory)]
    [Hashtable[]]$Sources

    [DscProperty()]
    [Ensure]$Ensure = [Ensure]::Present

    [DscProperty()]
    [bool]$Reset = $false

    [DscProperty()]
    [WinGetAction]$Action = [WinGetAction]::Full

    # Gets the current sources on winget.
    [WinGetSources] Get()
    {
        Assert-WinGetCommand "Get-WinGetSource"
        $packageCatalogReferences = Get-WinGetSource
        $wingetSources = [List[Hashtable]]::new()
        foreach ($packageCatalogReference in $packageCatalogReferences)
        {
            $source = @{
                Arg = $packageCatalogReference.Info.Argument
                Identifier = $packageCatalogReference.Info.Id
                Name = $packageCatalogReference.Info.Name
                Type = $packageCatalogReference.Info.Type
            }
            $wingetSources.Add($source)
        }

        $result = @{
            SID = ''
            Sources = $wingetSources
        }
        return $result
    }

    # Tests if desired properties match.
    [bool] Test()
    {
        $currentSources = $this.Get().Sources

        # If this is a full match and the counts are different give up.
        if (($this.Action -eq [WinGetAction]::Full) -and ($this.Sources.Count -ne $currentSources.Count))
        {
            return $false
        }

        # There's no need to differentiate between Partial and Full anymore.
        foreach ($source in $this.Sources)
        {
            # Require Name and Arg.
            if ((-not $source.ContainsKey("Name")) -or [string]::IsNullOrWhiteSpace($source.Name))
            {
                throw "Invalid source input. Name is required."
            }

            if ((-not $source.ContainsKey("Arg")) -or [string]::IsNullOrWhiteSpace($source.Arg))
            {
                throw "Invalid source input. Arg is required."
            }

            # Type has a default value.
            $sourceType = "Microsoft.PreIndexed.Package"
            if ($source.ContainsKey("Type") -and (-not([string]::IsNullOrWhiteSpace($source.Type))))
            {
                $sourceType = $source.Type
            }

            $result = $currentSources | Where-Object { $_.Name -eq $source.Name -and $_.Arg -eq $source.Arg -and $_.Type -eq $sourceType }

            # Source not found.
            if ($null -eq $result)
            {
                return $false
            }
        }

        return $true
    }

    # Sets the desired properties.
    [void] Set()
    {
        Assert-IsAdministrator
        Assert-WinGetCommand "Add-WinGetSource"
        Assert-WinGetCommand "Reset-WinGetSource"
        Assert-WinGetCommand "Remove-WinGetSource"

        foreach ($source in $this.Sources)
        {
            $sourceType = "Microsoft.PreIndexed.Package"

            # Require Name and Arg.
            if ((-not $source.ContainsKey("Name")) -or [string]::IsNullOrWhiteSpace($source.Name))
            {
                throw "Invalid source input. Name is required."
            }

            if ((-not $source.ContainsKey("Arg")) -or [string]::IsNullOrWhiteSpace($source.Arg))
            {
                throw "Invalid source input. Arg is required."
            }

            if ($source.ContainsKey("Type") -and (-not([string]::IsNullOrWhiteSpace($source.Type))))
            {
                $sourceType = $source.Type
            }

            if ($this.Ensure -eq [Ensure]::Present)
            {
                Add-WinGetSource -Name $source.Name -Argument $source.Argument -Type $source.Type

                if ($this.Reset)
                {
                    Reset-WinGetSource -Name $source.Name
                }
            }
            else
            {
                Remove-WinGetSource -Name $source.Name
            }
        }
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
                $hashArgs.Add("IncludePreRelease", $true)
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
                $hashArgs.Add("IncludePreRelease", $true)
            } elseif (-not [string]::IsNullOrWhiteSpace($this.Version))
            {
                $hashArgs.Add("Version", $this.Version)
            }

            $result = Repair-WinGetPackageManager @hashArgs

            if ($result -ne 0)
            {
                throw "Failed to repair winget. Result $result"
            }
        }
    }
}

#endregion DscResources
