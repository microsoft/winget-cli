---
author: Ruben Guerrero @msftrubengu
created on: 2022-11-14
last updated: 2022-11-14
issue id: 2697
---

# Spec Title

For [#1](https://github.com/microsoft/winget-cli/issues/2697)

## Abstract

This document contains the initial design for WinGet PowerShell DSC resources that help configure WinGet to the users desired state.

## Inspiration

See referenced issues in 2697.

## Solution Design

A new module Microsoft.WinGet.DSC is going to be created where all the WinGet related Powershell DSC resources are going to live. The resources are simple wrappers around functionality that Microsoft.WinGet.Client currently provides, with some additions. We are going to use PSDesiredStateConfiguration 3.0.0 for development, so the new module is going to be also in al alpha release at the beginning.

The proposed DSC resources are (names TBD):
1- UserSettings
2- AdminSettings
3- Sources
4- Install

### UserSettings
This resource will be in charge of editing the settings.json with the user settings. The user can ensure if the desired settings are going to be present or absent and can completely overwrite the file or append new settings.

Enable-WinGetSetting and Disable-WinGetSetting functions already exists in Microsoft.WinGet.Client, but those in reality refer to administrator settings. User settings are not necessarily something that you can enable or disable, because of that we need new cmdlets that are specific to user settings. We should also take in consideration that some settings are temporal, for example experimental features. We cannot add a new/remove new parameters every time one of them is implemented. Instead the input for these cmdlets is going to be a Hashtable with the user settings. Also, there won't be any other validation other than converting the Hashtable into a JSON. This is similar on how `winget settings` will just open the preferred text editor to let the user modify the file and will only inform a malformations on the next winget command.

#### DSC Resource
TODO: add syntax.

#### Get-WinGetUserSettings
Returns the Hashtable representation of winget's settings.json file.

#### Set-WinGetUserSettings
Writes the input settings into winget's settings.json file. Default behavior is additive, use Overwrite to delete previous content.

#### Test-WinGetUserSettings
Returns true is the inputs settings values match the current settings. Default, compares only the settings specified, optionally make a full comparison.
This could live inside the DSC resource, but in order to keep the code contained and easy to manager, is better to expose it.

#### Example of usage
TODO:

### AdminSettings Resource
This resource is a wrapper around Enable-WinGetSetting and Disable-WinGetSetting. The resources takes a Settings Hashtable with the admin setting as key and the desired state as value. At set, it will just go through the keys and perform enable/disable the admin as requested. Requires to be run as administrator.

#### winget-cli changes
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
TODO: add syntax.

#### Example of usage
TODO:

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
