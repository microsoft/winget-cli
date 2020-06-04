---
author: Ruben Guerrero @msftrubengu
created on: 2020-06-03
last updated: 2020-06-03
issue id: 396
---

# Settings command

For [#396](https://github.com/microsoft/winget-cli/issues/396)

## Abstract

The winget.exe client must support having a feature that allows users to specify settings for the application. This will be in the form of an editable file. The file must be in a location where a user can easily get it and modify it.

## Inspiration

Add ability for user to set their own preferences.

## Solution Design

### Settings File

#### Format

The WinGet settings file needs to be in a readable format for users and it must also be consider that WinGet already has code to handle YAML files. This design propose YAML as the format. 

Another options would be using JSON which, in theory, should be able to be parsed via yamlcpp (YAML is a superset of JSON) but it just doesn't seem natural. Any other format will require a new parser which is a hard no.

#### Location

WinGet can either run in package context or not. That means the location of the settings file will be determine depending on the context. 

Package Context: %LOCALAPPDATA%\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\settings.yaml

Non-package context: %LOCALAPPDATA%\Microsoft\Winget\settings.yaml

### Accessing WinGet Settings

The user must be able to open the settings file easily. This can be either via arguments `winget --settings` or command `winget settings`. The later is preferred as it allows future scenarios for settings, such as specifying a settings via commands. For example, `winget settings set SOME_SETTING on` is more desirable than `winget --settings-set SOME_SETTING on` or something similar.

When the user specified the command, WinGet will open the file via `ShellExecute`.

### Documentation

All setting must be documented in the winget-cli repository in doc\Settings.md

### Progress Bar Visual Style

For the sake of the discussion and to not provide a setting feature without a setting, the progress bar color setting will be implemented. This will only support the current visual style that WinGet already support and no new ones will be added. See [VisualStyle enum in AppInstallerRuntime.h](https://github.com/microsoft/winget-cli/blob/ed545f996acd36e9b4b277949abc7f62e259ad68/src/AppInstallerCLICore/ExecutionProgress.h#L20)

#### Setting

Setting name: progressBar

Possible values:
1. accent (default)
1. no-vt
1. rainbow
1. plain

Example:

```
progressBar: plain
```

#### Override

The user must be able to keep using the preferred visual style via arguments as it is now. This has more priority than the style defined in the settings file, because is saying it want that style for the current run.

## UI/UX Design

Executing `winget settings` will open settings file in the default text editor the user has for the file extension.

## Capabilities

It will allow winget to set and define any new settings in the future.

### Accessibility

Any description or output text that is added by consequence of this feature will need to be localized. Moreover, this feature allows a mechanism to add accessibility settings in the future.

### Security

This should not introduce any _new_ security concerns.

### Reliability

This is not expected to impact reliability.

Settings will not be transfer if the user changes the context in which WinGet runs. That is, if the user has the Microsoft provided WinGet package and uninstall it and runs a non-packaged WinGet, the settings are gone. This is by design.

### Compatibility

Previous versions winget will not be able to read a setting file. An update to winget is required.

### Performance, Power, and Efficiency

## Potential Issues

If the file isn't intuitive users will experience difficulties setting what they need.

## Future considerations

This feature allows the ability to expand the customization of winget for any user. It is also design to be expand with future settings commands that doesn't fit the scope of this future. For example, one could see `winget settings set SOME_SETTING on` to modify the settings without the need of editing a file.

