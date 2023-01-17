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
class WinGetUserSettingsResource
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
    [WinGetUserSettingsResource] Get()
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
class WinGetSourcesResource
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
    [WinGetSourcesResource] Get()
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

[DSCResource()]
class WinGetPackage
{
    [DscProperty(Key)]
    [string]$Id

    [DscProperty()]
    [string]$Version

    [DscProperty()]
    [string]$Source

    [DscProperty()]
    [Ensure]$Ensure = [Ensure]::Present

    [DscProperty(NotConfigurable)]
    [bool] $Installed = $false

    # Get.
    [WinGetPackage] Get()
    {
        Assert-WinGetCommand "Get-WinGetPackage"

        # DSC only validates keys and mandatories in a Set call.
        if ([string]::IsNullOrWhiteSpace($this.Id))
        {
            throw "Id is required"
        }

        $wingetPackageResource = [WinGetPackage]::new()
        $wingetPackageResource.Id = $this.Id

        # We can't have CatalogPackage as member of this class because it doesn't have a default constructor.
        # A call to Invoke-DscResource will fail with
        # 'ImportClassResourcesFromModule: Exception calling "ImportClassResourcesFromModule" with "4" argument(s): "The DSC resource 'CatalogPackage' has no default constructor."'
        $catalogPackage = Get-WinGetPackage -Id $this.Id -Exact
        if ($null -ne $catalogPackage)
        {
            $wingetPackageResource.Installed = $true
            $wingetPackageResource.Version = $catalogPackage.InstalledVersion.Version
            $wingetPackageResource.Source = $catalogPackage.DefaultInstallVersion.PackageCatalog.Info.Name
        }

        return $wingetPackageResource
    }

    # Test.
    [bool] Test()
    {
        $installedPackage = $this.Get()
        $ensureInstalled = $this.Ensure -eq [Ensure]::Present

        # This will populate info for Set if needed without another call to Get-WinGetPackage
        $this.Installed = $installedPackage.Installed

        # Not installed, doesn't have to.
        if (-not($installedPackage.Installed -or $ensureInstalled))
        {
            return $true
        }

        # Not install, need to ensure installed.
        # Installed, need to ensure not installed.
        if ($installedPackage.Installed -ne $ensureInstalled)
        {
            return $false
        }

        # Different sources.
        if (-not([string]::IsNullOrWhiteSpace($this.Source)))
        {
            if ($this.Source -ne $installedPackage.Source)
            {
                return $false
            }
        }

        $catalogPackage = Get-WinGetPackage -Id $this.Id -Exact

        # If no version is given, see if the latests is installed.
        if ([string]::IsNullOrWhiteSpace($this.Version) -and
            $catalogPackage.IsUpdateAvailable)
        {
            return $false
        }

        # If there is an specific version, compare with the current installed version.
        if (-not ([string]::IsNullOrWhiteSpace($this.Version)))
        {
            $compareResult = $catalogPackage.InstalledVersion.CompareToVersion($this.Version)
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
                Exact = $true
                Mode = 'Silent'
            }
            
            if ($this.Ensure -eq [Ensure]::Present)
            {
                if (-not([string]::IsNullOrWhiteSpace($this.Source)))
                {
                    $hashArgs.Add("Source", $this.Source)
                }

                if ($this.Installed)
                {
                    $catalogPackage = Get-WinGetPackage -Id $this.Id -Exact

                    if ([string]::IsNullOrWhiteSpace($this.Version))
                    {
                        Update-WinGetPackage @hashArgs
                    }
                    else
                    {
                        $hashArgs.Add("Version", $this.Version)

                        $compareResult = $catalogPackage.InstalledVersion.CompareToVersion($this.Version)
                        switch ($compareResult)
                        {
                            'Lesser'
                            {
                                # Installed package has a lower version. Update.
                                # TODO: if update fails we should uninstall and install.
                                Update-WinGetPackage @hashArgs
                                break
                            }
                            {'Greater' -or 'Unknown'}
                            {
                                # The installed package has a greated version or unknown. Uninstall and install.
                                Uninstall-WinGetPackage -Id $this.Id
                                Install-WinGetPackage @hashArgs
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

                    Install-WinGetPackage @hashArgs
                }
            }
            else
            {
                Uninstall-WinGetPackage -Id $this.Id
            }
        }
    }
}

#endregion DscResources
