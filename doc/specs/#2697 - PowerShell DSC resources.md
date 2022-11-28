---
author: Ruben Guerrero @msftrubengu
created on: 2022-11-14
last updated: 2022-11-14
issue id: 2697
---

# PowerShell DSC Resources for WinGet Spec

For [#2697](https://github.com/microsoft/winget-cli/issues/2697)

## Abstract

This document contains the initial proposal for WinGet PowerShell DSC resources that help configure WinGet to the users desired state.

## Inspiration

See referenced [#2697](https://github.com/microsoft/winget-cli/issues/2697).

## Solution Design

A new module Microsoft.WinGet.DSC is going to be created where all the WinGet related Powershell DSC resources are going to live. The resources are simple wrappers around functionality that Microsoft.WinGet.Client currently provides, with some additions. We are going to use PSDesiredStateConfiguration 3.0.0 for development, so the new module is going to initially be an alpha release.

The proposed DSC resources are (names TBD):
1. WinGetUserSettings
2. WinGetAdminSettings
3. WinGetSources
4. WinGetIntegrity
5. WinGetPackages
6. WinGetPackage

### 1. WinGetUserSettings
This resource will be in charge of editing the settings.json with the specified settings.

Enable-WinGetSetting and Disable-WinGetSetting functions already exists in Microsoft.WinGet.Client, but those in reality refer to administrator settings. User settings are not necessarily something that you can just enable or disable. Because of that we need new cmdlets that are specific to user settings. We should also take in consideration that some settings are temporal, for example experimental features. We cannot add a new/remove new parameters every time one of them is implemented. Also, there won't be any validation other than converting the Hashtable into a JSON. This is similar on how `winget settings` will just open the preferred text editor to let the user modify the file and will only inform of malformations on the next winget command.

#### Microsoft.WinGet.Client

*Get-WinGetUserSettings*

Returns the Hashtable representation of winget's settings.json file.

*Set-WinGetUserSettings*

Writes the input settings into winget's settings.json file. Default behavior is overwrite. Optionally do an additive set.

*Test-WinGetUserSettings*

Returns true is the inputs settings values match the current settings. By default, makes a full comparison, optionally compares only the settings specified.
This could live inside the DSC resource, but in order to keep the code contained and easy to manage and test is better to expose it.

#### DSC Resource

```
WinGetUserSettings [String] #ResourceName
{
    Settings = [string[]]
    SID = [string]
    [Action = [string]]
    [DependsOn = [string[]]]
    [PsDscRunAsCredential = [PSCredential]]
}
```

*Settings*

A Hashtable with the user settings to be set.

*SID (Key)*

DSC resources require a key that is a string, a signed/unsigned integer, or Enum types. The key must be unique per resource, but we only expect to have one winget user settings resource in a session. To resolve this, we can use the SID of the current user as the key. This will also allow a future implementation where an administrator sets different user settings for different users. For now, we won't expect people to set this and we will use an empty string as the current user's SID.

*Action*

Action is an enum with two values: Partial, Full.
- Partial: For Set, this means the changes are additive and overwrite existing settings if already present. For Test, this means validate all the settings in the input Settings are present and have the same value.
- Full: For Set, this means overwrite the settings.json file with the input settings. For Test, this means validate all the settings in the input Settings are present and have the same value and no other settings are explicitly set.

The default value is Partial.

#### Example of usage
```
$resource = @{
    Name = 'WinGetUserSettings'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        Settings = @{
            telemetry = @{
                disable = $true
            }
        }
    }
}

$testResult = Invoke-DscResource @resource -Method Test
if (-not $testResult.InDesiredState)
{
    Invoke-DscResource @resource -Method Set
}

```

### 2. WinGetAdminSettings
This resource will be in charge of enabling and disabling winget administrator settings and is a wrapper around Enable-WinGetSetting and Disable-WinGetSetting.

#### winget-cli
Currently, there's no way to expose if the administrator settings are enabled or not. This is a requirement for both Test and Get. We need to expose this information with a new `--export` option in the `winget settings` command. The export option will print a json with the information required.

```
{
    "adminSettings": {
        "BypassCertificatePinningForMicrosoftStore": false,
        "LocalManifestFiles": true,
        "FutureAdminSetting": "setting"
    }
}
```
We could technically read the registry and avoid this, but that doesn't sound like the right thing to do.

#### DSC Resource

```
WinGetAdminSettings [String] #ResourceName
{
    Settings = [string[]]
    SID = [string]
    [DependsOn = [string[]]]
    [PsDscRunAsCredential = [PSCredential]]
}
```

*Settings*

A Hashtable with the admin settings to be set.

*SID (Key)*

See SID property of UserSettings

#### Example of usage
```
$resource = @{
    Name = 'WinGetAdminSettings'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        Settings = @{
            LocalManifestFiles = $true,
        }
    }
}

$testResult = Invoke-DscResource @resource -Method Test
if (-not $testResult.InDesiredState)
{
    Invoke-DscResource @resource -Method Set
}

```

### 3. WinGetSources
This resource will be in charge of adding or removing winget sources. It wraps calls to Add-WinGetSource, Remove-WinGetSource and Get-WinGetSource.

#### DSC Resource
```
WinGetSources [String] #ResourceName
{
    SID = [string]
    Sources = [WinGetSource[]]
    [DependsOn = [string[]]]
    [Ensure = [string]]
    [Reset = [bool]]
    [PsDscRunAsCredential = [PSCredential]]
}
```

*SID (Key)*

See SID property of UserSettings

*Sources*
This is an array of a new PowerShell WinGetSource class. It contains the name, type and argument of the source. Type is optional as it default as "Microsoft.PreIndexed.Package". Think about it as the deserialized form of the result of `winget sources export` without the Identifier and Data properties.

It might seem natural to use the type "Microsoft.Management.Deployment.PackageCatalogReference" returned by Get-WinGetSource, but PackageCatalogReference is not what we really want.

Existing PowerShell DSC property Ensure will tell if the sources need to be added or deleted. Test will verify that all the sources are present.

*Ensure*

This is a enum with two values: Absent and Present. Defaults to present. For Set, Absent means call Remove-WinGetSource and Ensure means if not there call Add-WinGetSource.

*Reset*

Calls Reset-WinGetSource to repair the sources.

#### Example of usage
```
$wingetSources = [List[WinGetSource]]::new()
$wingetSources.Add([WinGetSource]::new(
    "winget",
    "https://cdn.winget.microsoft.com/cache"))

$resource = @{
    Name = 'WinGetSources'
    ModuleName = 'Microsoft.WinGet.DSC'
    Property = @{
        Sources = $wingetSources
        Ensure = Absent
    }
}

# Delete winget source :)
Invoke-DscResource @resource -Method Set
```

### 4. WinGetIntegrity
This resource checks the general integrity of winget. It will be in charge of installing/updating winget as well as fixing the common issues described in [Troubleshooting](https://github.com/microsoft/winget-cli/tree/master/doc/troubleshooting). This name is the the most TBD of the resources names, please help proposing new ones.

#### Repair
We will create a new function in Microsoft.WinGet.Client `Repair-WinGet` that will try to repair winget in the machine. It won't do any installations. This is basically run winget and see what happens and if it fails try to repair it. Is important to say that we won't repair or try to repair all of the issues. When we run `winget` it will be without any commands, so specific failure related with other commands are more complicated and can't sadly be easily automated.

#### Update
One can only dream to call `Install-WinGetPackage -id "Microsoft.WinGet"`. For now we will use the GitHub release URL with the version provided, download it and call `Add-AppxPackage`.

#### Downgrade
Same as update but using the `ForceUpdateFromAnyVersion` option.

#### Constraints
*Windows version*

Make sure Windows versions is equal or newer than 10.0.17763.0

*Appx PowerShell module*

There was a [bug](https://github.com/PowerShell/PowerShell/issues/13138) in the Appx Powershell module fails on PowerShell Core. The fix is available starting 22453 and it won't get back ported. This means that anything before that can't just simple use the Add-AppxPackage cmdlet. Luckily there's a remediation which is importing the module with the `UseWindowsPowerShell` option. We need to check the OS version and import Appx accordingly.

To get information about App Installer, we could also use `Get-AppxProvisionedPackage` from the DISM module. The problem is that the version returned is the bundle version. We will need to do some gymnastics or parse the bundle version and assumes date to get the package version.

*App Installer*

The first App Installer GA version that contained winget was 1.11.13404. It the version is lower than that (and OS requirements are fulfilled) we need to download winget and installed it before even trying to repair it.


#### DSC Resource
```
WinGetIntegrity [String] #ResourceName
{
    SID = [string]
    [DependsOn = [string[]]]
    [PsDscRunAsCredential = [PSCredential]]
    [Version = [string]]
}
```
*SID (Key)*

See SID property of UserSettings

*Version*

If the version is empty, then we will assume that the current version installed needs to get repaired. If winget is no in the box (really old App Installer package), then we will install the latest stable release. If a version is present and is not the same as the one installed, we will download it and install it. A user can specify a preview version.

#### Notes
I'm kind of hesitant on leaving this as one DSC resource. We might need to split it into the integrity and the installation resources. Feedback is appreciated.

### 5. WinGetPackages
This resource is basically a wrapper around `winget import`. It takes an input file that describes the packages to install.

#### DSC Resource
```
WinGetPackages [String] #ResourceName
{
    SID = [string]
    PackagesFile = [string]
    [IgnoreUnavailable = [bool]]
    [IgnoreVersions = [bool]]
    [NoUpgrade = [bool]]
    [DependsOn = [string[]]]
    [PsDscRunAsCredential = [PSCredential]]
    [Version = [string]]
}
```
*SID (Key)*

See SID property of UserSettings.

*PackagesFile*

A local file with the packages to install. Follows the [winget install schema](https://raw.githubusercontent.com/microsoft/winget-cli/master/schemas/JSON/packages/packages.schema.1.0.json).

*IgnoreUnavailable*

Uses the `--ignore-unavailable` option to ignore unavailable packages. Default false.

*IgnoreVersions*

Uses the `--ignore-versions` option to ignore package versions in the impo[NoUpgrade = [bool]]rt file. Default false.

*NoUpgrade*

Uses the `--no-upgrade` option to skip upgrade if an installed version already exists.

### 6. WinGetPackage
This resource installs a single package via winget with the specified information. Acts as a wrapper around `winget install`. We will always use the silent option.

#### DSC Resource
```
WinGetPackages [String] #ResourceName
{
    PackageIdentifier = [string]
    [Version = [string]]
    [Source = [string]]
    [WinGetOverride = [string]]
    [InstallerOverride = [string]]
    [DependsOn = [string[]]]
    [PsDscRunAsCredential = [PSCredential]]
    [Version = [string]]
}
```
*PackageIdentifier (Key)*

The package identifier of the package to be installed. Required.

*Version*

Optional version of the package to be installer. If empty it will use latests.

*Source*

Optional source of the package. If empty, use winget.

*WinGetOverride*

Optional parameter to allow any other winget options that will be passed to `winget install`. Allow us to keep this resource open for different configurations without adding new parameters.

*InstallerOverride*

Optional value for the `--override` option. We can make this a parameter in the resource or remove it and make users use WinGetOverride with a value of `--override something`.

## Potential Issues
We are taking a dependency on PowerShell DSC 3.0.0 which is still in an alpha release. I expect us to find problems on the way, but hopefully nothing that will block us.
