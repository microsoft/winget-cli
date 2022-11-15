---
author: Ruben Guerrero @msftrubengu
created on: 2022-11-14
last updated: 2022-11-14
issue id: 2697
---

# Spec Title

For [#2697](https://github.com/microsoft/winget-cli/issues/2697)

## Abstract

This document contains the initial design for WinGet PowerShell DSC resources that help configure WinGet to the users desired state.

## Inspiration

See referenced issues in [#2697](https://github.com/microsoft/winget-cli/issues/2697).

## Solution Design

A new module Microsoft.WinGet.DSC is going to be created where all the WinGet related Powershell DSC resources are going to live. The resources are simple wrappers around functionality that Microsoft.WinGet.Client currently provides, with some additions. We are going to use PSDesiredStateConfiguration 3.0.0 for development, so the new module is going to be also in al alpha release at the beginning.

The proposed DSC resources are (names TBD):
1. WinGetUserSettings
2. WinGetAdminSettings
3. WinGetSources
4. WinGetInstallation

### UserSettings
This resource will be in charge of editing the settings.json with the user settings.

Enable-WinGetSetting and Disable-WinGetSetting functions already exists in Microsoft.WinGet.Client, but those in reality refer to administrator settings. User settings are not necessarily something that you can just enable or disable, because of that we need new cmdlets that are specific to user settings. We should also take in consideration that some settings are temporal, for example experimental features. We cannot add a new/remove new parameters every time one of them is implemented. Also, there won't be any other validation other than converting the Hashtable into a JSON. This is similar on how `winget settings` will just open the preferred text editor to let the user modify the file and will only inform a malformations on the next winget command.

#### Microsoft.WinGet.Client

*Get-WinGetUserSettings*

Returns the Hashtable representation of winget's settings.json file.

*Set-WinGetUserSettings*

Writes the input settings into winget's settings.json file. Default behavior must be additive and there must be an option to delete overwrite content.

*Test-WinGetUserSettings*

Returns true is the inputs settings values match the current settings. Default, compares only the settings specified, optionally make a full comparison.
This could live inside the DSC resource, but in order to keep the code contained and easy to manager, is better to expose it.

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

DSC resources require a key that is a string, a signed/unsigned integer, or Enum types. The key must be unique per resource, but we only expect to have one winget user settings resource in a session. To resolve this, we can use the SID of the current user as the key. This will also allow a future implementation where an administrator sets different user settings for different users. For now, we won't expect people to set this, we will do it internally.

*Action*

Action is an enum with two values: Partial, Full.
Partial - For Set, this means the changes are additive, but overwriting existing settings if already present. For Test, this means "validate all the settings in the input Settings are present and have the same value".
Full - For Set, this means overwrite the settings.json file with the input ettings. For Test, this means "validate all the settings in the input Settings are present and have the same value and no other settings are explicitly set".

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

### AdminSettings Resource
This resource is a wrapper around Enable-WinGetSetting and Disable-WinGetSetting.

#### winget-cli
Currently, there's no way to expose if the administrator settings are enabled or not. This is a requirement for both Test and Get. We need to expose this information with a new `--export` option in the `winget settings` command. The export option will print a json with the information required.

```
{
    "LocalManifestFiles": true,
    "BypassCertificatePinningForMicrosoftStore": false,
    "FutureAdminSetting": true
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

#### Alternative
Since admin settings don't change that often, we could also make an enum as the key that represents an individual admin setting. Users would now have to declare one DSC resource per admin settings and we also lose the future ability to set them specifically for different users.

### Source Resource
This resource will be in charge of adding or removing winget sources. It wraps calls to Add-WinGetSource, Remove-WinGetSource and Get-WinGetSource.

Its input is an array of WinGetSource, which is going to be a new PowerShell class that contains the name, type and argument of a source. Think about it as the deserialized form of the result of `winget sources export` without the Identifier and Data properties.

Existing PowerShell DSC property Ensure will tell if the sources need to be added or deleted. Test will verify that all the sources are present.

#### DSC Resource
TODO: add syntax.

#### Example of usage
TODO:

### Install Resource
This resource checks the general integrity of winget. It usage is to fix the current state of winget but performing common solutions (see main issue) as well as updating/downgrading to different versions of winget.

#### First usage
TODO:

#### Update
TODO:

#### Revert
TODO:

## Potential Issues
We are taking a dependency on PowerShell DSC 3.0.0 which is still in an alpha release. I expect us to find problems on the way, but hopefully nothing that will block us.
